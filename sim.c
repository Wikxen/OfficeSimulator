#include "sim.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static const Money cash_limit = 999999999;
enum { WEEK_LIMIT = 999999, LEGACY_AMENITY_LIMIT = 4,
    QUIT_THRESHOLD = 35, QUIT_MIN_CHANCE = 10, QUIT_MAX_CHANCE = 65 };
static const uint32_t default_seed = UINT32_C(0xe71c8d41);

const Office offices[TIER_COUNT] = {
    {"Starter Suite", 2, 1, 180, 0},
    {"Studio Office", 5, 4, 550, 5200},
    {"Team Loft", 9, 7, 1250, 14500},
    {"Headquarters", STAFF_LIMIT, AMENITY_COUNT, 2750, 36000},
};

const Amenity amenities[AMENITY_COUNT] = {
    {"Coffee station", 9, 450, 40},
    {"Indoor plants", 9, 1020, 40},
    {"Lounge sofa", 9, 1590, 40},
    {"Arcade cabinet", 9, 2160, 40},
    {"Snack bar", 6, 2500, 65},
    {"Quiet nook", 6, 2950, 60},
    {"Game table", 5, 3400, 75},
    {"Fitness corner", 5, 3900, 85},
    {"Reading library", 4, 4400, 90},
    {"Wellness pod", 4, 4900, 105},
};

const char *const action_names[ACTION_COUNT] = {
    "Improve furniture", "Upgrade technology", "Add amenities", "Hire a teammate", "Move office",
};

Sim sim_new(uint32_t seed)
{
    return (Sim){.cash = 2400, .week = 1, .staff = 1, .rng = seed ? seed : default_seed};
}

Forecast sim_forecast(const Sim *s)
{
    const Office *office = &offices[s->tier];
    int mood = 59 + 8*s->furniture + 5*s->tech
        - 23*(s->staff - 1)/office->capacity;
    Money upkeep = 0;
    for (int i = 0; i < s->amenities; ++i) {
        mood += amenities[i].satisfaction;
        upkeep += amenities[i].upkeep;
    }
    if (mood > 100) mood = 100;
    int mood_loss = s->disruption < mood ? s->disruption : mood;
    mood -= mood_loss;
    Money revenue = (Money)s->staff * (160 + 7*mood + 25*s->tech)
        * (100 + 2*s->reputation) / 100;
    Money costs = office->rent + 385*(s->staff - 1)
        + 20*s->furniture + 35*s->tech + upkeep;
    return (Forecast){mood, mood_loss, revenue, costs, revenue - costs};
}

Money sim_price(const Sim *s, Action action)
{
    switch (action) {
    case FURNITURE: return s->furniture < UPGRADE_LIMIT ? 500 + 620*s->furniture : 0;
    case TECH: return s->tech < UPGRADE_LIMIT ? 650 + 730*s->tech : 0;
    case AMENITIES: return s->amenities < offices[s->tier].amenity_slots ? amenities[s->amenities].price : 0;
    case HIRE: return 500 + 140*(s->staff - 1);
    case MOVE: return s->tier + 1 < TIER_COUNT ? offices[s->tier + 1].move : 0;
    default: return 0;
    }
}

const char *sim_block(const Sim *s, Action action)
{
    Money price = sim_price(s, action);
    if (s->closed) return "Office closed";
    if (action == AMENITIES && !price)
        return s->amenities == AMENITY_COUNT ? "All perks installed" : "No room for more";
    if (!price) return "Fully upgraded";
    if (action == HIRE && s->staff >= offices[s->tier].capacity) return "No vacancy";
    if (action == MOVE && s->staff < offices[s->tier].capacity) return "Fill this office first";
    if (price > s->cash) return "Need more cash";
    return NULL;
}

static uint32_t roll(Sim *s, uint32_t limit)
{
    uint32_t next = s->rng;
    next ^= next << 13;
    next ^= next >> 17;
    next ^= next << 5;
    s->rng = next;
    return next % limit;
}

static void disrupt(Sim *s, int low, int high)
{
    s->disruption += low + (int)roll(s, (uint32_t)(high - low + 1));
}

int sim_quit_chance(const Sim *s)
{
    if (s->closed || s->staff <= 1 || s->disruption < QUIT_THRESHOLD) return 0;
    int chance = QUIT_MIN_CHANCE + s->disruption - QUIT_THRESHOLD;
    return chance < QUIT_MAX_CHANCE ? chance : QUIT_MAX_CHANCE;
}

bool sim_buy(Sim *s, Action action)
{
    if (sim_block(s, action)) return false;
    s->cash -= sim_price(s, action);
    switch (action) {
    case FURNITURE: ++s->furniture; break;
    case TECH: ++s->tech; break;
    case AMENITIES: ++s->amenities; break;
    case HIRE:
        ++s->staff;
        disrupt(s, HIRE_DISRUPTION_MIN, HIRE_DISRUPTION_MAX);
        break;
    case MOVE:
        ++s->tier;
        disrupt(s, MOVE_DISRUPTION_MIN, MOVE_DISRUPTION_MAX);
        break;
    default: return false;
    }
    return true;
}

int sim_cohesion_recovery(const Sim *s)
{
    if (s->staff <= 3) return 3;
    if (s->staff <= 8) return 2;
    return 1;
}

WeekResult sim_advance(Sim *s)
{
    if (s->closed || s->week == WEEK_LIMIT) return WEEK_BLOCKED;
    Forecast forecast = sim_forecast(s);
    s->last_net = forecast.net;
    s->cash += forecast.net;
    if (s->cash > cash_limit) s->cash = cash_limit;
    s->closed = s->cash < 0;
    if (forecast.mood >= 75 && forecast.net > 0) {
        if (s->reputation < 25) ++s->reputation;
    } else if (s->reputation > 0) {
        --s->reputation;
    }
    if (s->tier == TIER_COUNT - 1 && s->staff == offices[s->tier].capacity
        && forecast.mood >= THRIVE_MOOD && forecast.net > 0) {
        if (s->streak < THRIVE_WEEKS) ++s->streak;
        if (s->streak == THRIVE_WEEKS) s->won = true;
    } else {
        s->streak = 0;
    }
    int chance = sim_quit_chance(s);
    bool quit = chance > 0 && roll(s, 100) < (uint32_t)chance;
    if (!s->closed) {
        int recovery = sim_cohesion_recovery(s);
        s->disruption = s->disruption > recovery ? s->disruption - recovery : 0;
        if (quit) {
            --s->staff;
            s->disruption += QUIT_DISRUPTION;
            s->streak = 0;
        }
    }
    ++s->week;
    return quit ? WEEK_TEAMMATE_QUIT : WEEK_FINISHED;
}

int sim_encode(const Sim *s, char *out, size_t size)
{
    return snprintf(out, size, "OFFICE2 %lld %lld %d %d %d %d %d %d %d %d %d %d %d %" PRIu32 "\n",
        s->cash, s->last_net, s->week, s->tier, s->staff, s->furniture, s->tech,
        s->amenities, s->reputation, s->streak, s->won, s->closed, s->disruption, s->rng);
}

bool sim_decode(const char *text, Sim *out)
{
    Sim s = {0};
    int won, closed, consumed = 0;
    char extra;
    bool legacy = strncmp(text, "OFFICE1 ", 8) == 0;
    if (!legacy && strncmp(text, "OFFICE2 ", 8) != 0) return false;
    if (sscanf(text + 8, "%10lld %10lld %6d %2d %2d %2d %2d %2d %2d %2d %1d %1d%n",
            &s.cash, &s.last_net, &s.week, &s.tier, &s.staff, &s.furniture, &s.tech,
            &s.amenities, &s.reputation, &s.streak, &won, &closed, &consumed) != 12)
        return false;
    if (!consumed) return false;
    const char *tail = text + 8 + consumed;
    if (legacy) {
        if (sscanf(tail, " %c", &extra) == 1) return false;
        s.rng = default_seed ^ ((uint32_t)s.week * UINT32_C(2654435761)) ^ (uint32_t)s.cash;
        if (!s.rng) s.rng = default_seed;
    } else {
        unsigned long long rng;
        if (*tail != ' ' || sscanf(tail, "%8d %10llu %c", &s.disruption, &rng, &extra) != 2
            || rng == 0 || rng > UINT32_MAX)
            return false;
        s.rng = (uint32_t)rng;
    }
    if (s.tier < 0 || s.tier >= TIER_COUNT) return false;
    int slots = offices[s.tier].amenity_slots;
    // Let older offices retain amenities they already paid for.
    int saved_slots = legacy || slots < LEGACY_AMENITY_LIMIT ? LEGACY_AMENITY_LIMIT : slots;
    // At most one resignation and one replacement per completed week.
    int disruption_limit = (STAFF_LIMIT - 1)*HIRE_DISRUPTION_MAX + (TIER_COUNT - 1)*MOVE_DISRUPTION_MAX
        + (s.week - 1)*(HIRE_DISRUPTION_MAX + QUIT_DISRUPTION);
    if (s.cash < -25000 || s.cash > cash_limit || s.last_net < -25000 || s.last_net > 25000
        || s.week < 1 || s.week > WEEK_LIMIT
        || s.staff < 1 || s.staff > offices[s.tier].capacity
        || s.furniture < 0 || s.furniture > UPGRADE_LIMIT
        || s.tech < 0 || s.tech > UPGRADE_LIMIT || s.amenities < 0 || s.amenities > saved_slots
        || s.disruption < 0 || s.disruption > disruption_limit
        || s.reputation < 0 || s.reputation > 25 || s.streak < 0 || s.streak > THRIVE_WEEKS
        || (won != 0 && won != 1) || (closed != 0 && closed != 1) || closed != (s.cash < 0)
        || (s.streak == THRIVE_WEEKS && !won))
        return false;
    s.won = won != 0;
    s.closed = closed != 0;
    *out = s;
    return true;
}
