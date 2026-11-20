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
    Sim s = sim_new();
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
    Sim s = sim_new();
    bool visited[TIER_COUNT] = {true};
    while (!s.won && s.week <= 60) {
        int levels[] = {s.furniture, s.tech, s.amenities};
        for (int i = 0; i <= AMENITIES; ++i) {
            if (levels[i] < (s.tier == 0 ? 2 : 3)) sim_buy(&s, i);
        }
        while (sim_buy(&s, HIRE)) {}
        if (sim_buy(&s, MOVE)) visited[s.tier] = true;
        assert(sim_forecast(&s).net > 0);
        assert(sim_advance(&s));
        check_round_trip(&s);
    }
    for (int i = 0; i < TIER_COUNT; ++i) assert(visited[i]);
    assert(s.won && s.streak == THRIVE_WEEKS && s.tier == TIER_COUNT - 1);
    assert(s.staff == offices[s.tier].capacity && sim_forecast(&s).mood >= THRIVE_MOOD);
    printf("Campaign reached a thriving headquarters in %d weeks.\n", s.week - 1);
    assert(sim_advance(&s) && s.won && s.streak == THRIVE_WEEKS);
}

static void check_limits(void)
{
    Sim s = sim_new();
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

    Sim untouched = sim_new();
    assert(!sim_decode("OFFICE1 2400 0 1 4 1 0 0 0 0 0 0 0", &untouched));
    assert(!sim_decode("OFFICE1 2400 0 1 0 16 0 0 0 0 0 0 0", &untouched));
    assert(!sim_decode("OFFICE1 2400 0 1 0 1 0 0 0 0 0 0 0 junk", &untouched));
    assert(!sim_decode("OFFICE1 2400 0 1", &untouched));
    assert(untouched.cash == 2400 && untouched.tier == 0 && untouched.staff == 1);
}

int main(void)
{
    check_start();
    check_growth();
    check_limits();
    puts("Simulation tests passed.");
    return 0;
}
