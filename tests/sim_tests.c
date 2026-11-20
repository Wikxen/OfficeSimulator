#include "sim.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void check_round_trip(const Sim *s)
{
    char source[128], restored[128];
    Sim loaded;
    int size = sim_encode(s, source, sizeof source);
    assert(size > 0 && (size_t)size < sizeof source);
    assert(sim_decode(source, &loaded));
    sim_encode(&loaded, restored, sizeof restored);
    assert(strcmp(source, restored) == 0);
}

static void check_start(void)
{
    Sim s = sim_new(1);
    assert(s.staff == 1 && s.cash == 2400 && s.week == 1 && s.tier == 0);
    assert(s.furniture == 0 && s.tech == 0 && s.amenities == 0);
    Forecast bare = sim_forecast(&s);
    assert(bare.net > 0 && bare.mood < 75);
    assert(!sim_buy(&s, MOVE) && s.cash == 2400);
    assert(sim_buy(&s, FURNITURE) && sim_buy(&s, AMENITIES));
    Forecast furnished = sim_forecast(&s);
    assert(furnished.mood >= 75 && furnished.revenue > bare.revenue && furnished.net > bare.net);
    Money balance = s.cash;
    assert(sim_advance(&s) && s.reputation == 1 && s.week == 2);
    assert(s.last_net == furnished.net && s.cash == balance + furnished.net);
    assert(sim_forecast(&s).revenue > furnished.revenue);
    assert(sim_buy(&s, HIRE) && s.staff == 2 && !sim_buy(&s, HIRE));
    assert(sim_forecast(&s).mood < furnished.mood);
    assert(!sim_buy(&s, MOVE));
    check_round_trip(&s);
}

static void check_growth(void)
{
    int fastest = 100, slowest = 0;
    for (uint32_t seed = 1; seed <= 64; ++seed) {
        Sim s = sim_new(seed);
        bool visited[TIER_COUNT] = {true};
        while (!s.won && s.week <= 100) {
            if (s.furniture < 3) sim_buy(&s, FURNITURE);
            if (s.tech < 3) sim_buy(&s, TECH);
            if (s.amenities < 3) sim_buy(&s, AMENITIES);
            if (s.disruption <= 6) sim_buy(&s, HIRE);
            if (!s.disruption && s.cash >= sim_price(&s, MOVE) + 1500 && sim_buy(&s, MOVE))
                visited[s.tier] = true;
            assert(sim_forecast(&s).net > 0);
            assert(sim_advance(&s) == WEEK_FINISHED);
            check_round_trip(&s);
        }
        for (int i = 0; i < TIER_COUNT; ++i) assert(visited[i]);
        assert(s.won && s.streak == THRIVE_WEEKS && s.tier == TIER_COUNT - 1);
        assert(s.staff == offices[s.tier].capacity && sim_forecast(&s).mood >= THRIVE_MOOD);
        if (s.week - 1 < fastest) fastest = s.week - 1;
        if (s.week - 1 > slowest) slowest = s.week - 1;
        assert(sim_advance(&s) && s.won && s.streak == THRIVE_WEEKS);
    }
    printf("Paced growth won in %d-%d weeks across 64 seeds.\n", fastest, slowest);
}

static void check_cohesion(void)
{
    unsigned hire_rolls = 0, move_rolls = 0;
    for (uint32_t seed = 1; seed <= 64; ++seed) {
        Sim s = sim_new(seed);
        s.cash = 100000;
        assert(!sim_buy(&s, MOVE) && s.rng == seed && !s.disruption);
        assert(sim_buy(&s, HIRE));
        int hire = s.disruption;
        assert(hire >= HIRE_DISRUPTION_MIN && hire <= HIRE_DISRUPTION_MAX);
        hire_rolls |= 1u << (hire - HIRE_DISRUPTION_MIN);
        int mood = sim_forecast(&s).mood;

        Sim loaded;
        char save[128];
        sim_encode(&s, save, sizeof save);
        assert(sim_decode(save, &loaded));
        assert(loaded.disruption == hire && loaded.rng == s.rng);
        assert(sim_buy(&s, MOVE) && sim_buy(&loaded, MOVE));
        assert(loaded.disruption == s.disruption && loaded.rng == s.rng);
        int move = s.disruption - hire;
        assert(move >= MOVE_DISRUPTION_MIN && move <= MOVE_DISRUPTION_MAX);
        move_rolls |= 1u << (move - MOVE_DISRUPTION_MIN);
        assert(sim_forecast(&s).mood < mood);

        assert(sim_buy(&s, HIRE) && s.disruption >= hire + move + HIRE_DISRUPTION_MIN);
        Forecast disrupted = sim_forecast(&s);
        Sim settled = s;
        settled.disruption = 0;
        assert(disrupted.mood < sim_forecast(&settled).mood);
        assert(disrupted.revenue < sim_forecast(&settled).revenue);
        while (s.disruption > 0) {
            int before = s.disruption;
            int staff = s.staff;
            Money net = sim_forecast(&s).net;
            WeekResult result = sim_advance(&s);
            assert(result != WEEK_BLOCKED && s.last_net == net);
            int expected = before > COHESION_RECOVERY ? before - COHESION_RECOVERY : 0;
            if (result == WEEK_TEAMMATE_QUIT) {
                assert(staff > 1 && s.staff == staff - 1);
                expected += QUIT_DISRUPTION;
            } else assert(s.staff == staff);
            assert(s.disruption == expected);
        }
    }
    assert(hire_rolls == (1u << (HIRE_DISRUPTION_MAX - HIRE_DISRUPTION_MIN + 1)) - 1);
    assert(move_rolls == (1u << (MOVE_DISRUPTION_MAX - MOVE_DISRUPTION_MIN + 1)) - 1);

    Sim rushed = sim_new(73);
    rushed.cash = sim_price(&rushed, HIRE) + offices[1].move;
    assert(sim_buy(&rushed, HIRE) && sim_buy(&rushed, MOVE) && rushed.cash == 0);
    Sim settled = rushed;
    settled.disruption = 0;
    assert(sim_forecast(&settled).net > 0 && sim_forecast(&rushed).net < 0);
    assert(sim_advance(&rushed) == WEEK_FINISHED && rushed.closed && rushed.staff == 2);
}

static void check_resignations(void)
{
    int quit_count = 0;
    for (uint32_t seed = 1; seed <= 100; ++seed) {
        Sim s = sim_new(seed);
        s.cash = 100000;
        s.staff = 2;
        s.disruption = 34;
        assert(sim_quit_chance(&s) == 0);
        Sim safe = s;
        assert(sim_advance(&safe) == WEEK_FINISHED && safe.staff == 2 && safe.rng == seed);
        s.disruption = 35;
        assert(sim_quit_chance(&s) == 10);
        s.disruption = 50;
        assert(sim_quit_chance(&s) == 25);
        s.disruption = 100;
        assert(sim_quit_chance(&s) == 65);

        Sim loaded;
        char save[128];
        sim_encode(&s, save, sizeof save);
        assert(sim_decode(save, &loaded));
        Forecast before = sim_forecast(&s);
        Money cash = s.cash;
        WeekResult result = sim_advance(&s);
        assert(sim_advance(&loaded) == result);
        assert(loaded.disruption == s.disruption && loaded.staff == s.staff && loaded.rng == s.rng);
        assert(s.last_net == before.net && s.cash == cash + before.net);
        if (result == WEEK_TEAMMATE_QUIT) {
            ++quit_count;
            assert(s.staff == 1 && s.disruption == 105 && s.streak == 0);
            assert(sim_forecast(&s).costs == before.costs - 385);
            assert(sim_quit_chance(&s) == 0);
            uint32_t state = s.rng;
            assert(sim_advance(&s) == WEEK_FINISHED && s.staff == 1 && s.rng == state);
        } else {
            assert(result == WEEK_FINISHED && s.staff == 2 && s.disruption == 97);
        }
        check_round_trip(&s);
    }
    assert(quit_count > 0 && quit_count < 100);

    Sim chained = sim_new(1);
    chained.cash = 100000;
    chained.week = 100;
    chained.disruption = 1000;
    check_round_trip(&chained);
}

static void check_amenities(void)
{
    Sim s = sim_new(1);
    s.cash = 1000000;
    const int slots[] = {1, 4, 7, 10};
    for (int tier = 0; tier < TIER_COUNT; ++tier) {
        assert(s.tier == tier && offices[tier].amenity_slots == slots[tier]);
        while (s.amenities < slots[tier]) {
            Forecast before = sim_forecast(&s);
            const Amenity *amenity = &amenities[s.amenities];
            Money cash = s.cash;
            assert(sim_price(&s, AMENITIES) == amenity->price && sim_buy(&s, AMENITIES));
            assert(s.cash == cash - amenity->price);
            assert(sim_forecast(&s).costs == before.costs + amenity->upkeep);
        }
        assert(!sim_buy(&s, AMENITIES));
        while (sim_buy(&s, HIRE)) {}
        if (tier + 1 < TIER_COUNT) assert(sim_buy(&s, MOVE) && s.amenities == slots[tier]);
    }
    assert(s.disruption >= (STAFF_LIMIT - 1)*HIRE_DISRUPTION_MIN + (TIER_COUNT - 1)*MOVE_DISRUPTION_MIN);
    assert(sim_forecast(&s).mood == 0);
    check_round_trip(&s);

    Sim restored;
    assert(sim_decode("OFFICE1 2400 0 1 0 1 0 0 4 0 0 0 0\n", &restored));
    assert(restored.amenities == 4 && !restored.disruption && restored.rng);
    assert(!sim_buy(&restored, AMENITIES));
    check_round_trip(&restored);
}

static void check_limits(void)
{
    Sim s = sim_new(1);
    s.cash = 100000;
    for (int i = 0; i < UPGRADE_LIMIT; ++i) assert(sim_buy(&s, FURNITURE));
    assert(!sim_buy(&s, FURNITURE));
    s.tier = TIER_COUNT - 1;
    assert(!sim_buy(&s, MOVE));
    s.cash = 1;
    s.reputation = 1;
    assert(sim_forecast(&s).net < 0 && sim_advance(&s));
    assert(s.closed && s.cash < 0 && s.reputation == 0);
    assert(!sim_advance(&s) && !sim_buy(&s, TECH));
    check_round_trip(&s);

    Sim untouched = sim_new(1);
    assert(!sim_decode("OFFICE1 2400 0 1 4 1 0 0 0 0 0 0 0", &untouched));
    assert(!sim_decode("OFFICE1 2400 0 1 0 16 0 0 0 0 0 0 0", &untouched));
    assert(!sim_decode("OFFICE1 2400 0 1 0 1 0 0 0 0 0 0 0 junk", &untouched));
    assert(!sim_decode("OFFICE1 2400 0 1", &untouched));
    assert(!sim_decode("OFFICE2 2400 0 1 0 1 0 0 5 0 0 0 0 0 1", &untouched));
    assert(!sim_decode("OFFICE2 2400 0 1 0 1 0 0 0 0 0 0 0 265 1", &untouched));
    assert(!sim_decode("OFFICE2 2400 0 1 0 1 0 0 0 0 0 0 0 0 0", &untouched));
    assert(!sim_decode("OFFICE2 2400 0 1 0 1 0 0 0 0 0 0 0 0 4294967296", &untouched));
    assert(untouched.cash == 2400 && untouched.tier == 0 && untouched.staff == 1);
}

int main(void)
{
    check_start();
    check_cohesion();
    check_resignations();
    check_amenities();
    check_growth();
    check_limits();
    puts("Simulation tests passed.");
    return 0;
}
