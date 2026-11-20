#include "sim.h"
#include <stdio.h>

static const Money cash_limit = 999999999;
enum { WEEK_LIMIT = 999999 };

const Office offices[TIER_COUNT] = {
    {"Starter Suite", 2, 180, 0},
    {"Studio Office", 5, 550, 5200},
    {"Team Loft", 9, 1250, 14500},
    {"Headquarters", 16, 2750, 36000},
};

const char *const action_names[ACTION_COUNT] = {
    "Improve furniture", "Upgrade technology", "Add amenities", "Hire a teammate", "Move office",
};

Sim sim_new(void)
{
    return (Sim){.cash = 2400, .week = 1, .staff = 1};
}

Forecast sim_forecast(const Sim *s)
{
    const Office *office = &offices[s->tier];
    int mood = 59 + 8*s->furniture + 5*s->tech + 9*s->amenities
        - 23*(s->staff - 1)/office->capacity;
    if (mood > 100) mood = 100;
    Money revenue = (Money)s->staff * (160 + 7*mood + 25*s->tech)
        * (100 + 2*s->reputation) / 100;
    Money costs = office->rent + 385*(s->staff - 1)
        + 20*s->furniture + 35*s->tech + 40*s->amenities;
    return (Forecast){mood, revenue, costs, revenue - costs};
}

Money sim_price(const Sim *s, Action action)
{
    switch (action) {
    case FURNITURE: return s->furniture < UPGRADE_LIMIT ? 500 + 620*s->furniture : 0;
    case TECH: return s->tech < UPGRADE_LIMIT ? 650 + 730*s->tech : 0;
    case AMENITIES: return s->amenities < UPGRADE_LIMIT ? 450 + 570*s->amenities : 0;
    case HIRE: return 500 + 140*(s->staff - 1);
    case MOVE: return s->tier + 1 < TIER_COUNT ? offices[s->tier + 1].move : 0;
    default: return 0;
    }
}

const char *sim_block(const Sim *s, Action action)
{
    Money price = sim_price(s, action);
    if (s->closed) return "Office closed";
    if (!price) return "Fully upgraded";
    if (action == HIRE && s->staff >= offices[s->tier].capacity) return "No vacancy";
    if (action == MOVE && s->staff < offices[s->tier].capacity) return "Fill this office first";
    if (price > s->cash) return "Need more cash";
    return NULL;
}

bool sim_buy(Sim *s, Action action)
{
    if (sim_block(s, action)) return false;
    s->cash -= sim_price(s, action);
    switch (action) {
    case FURNITURE: ++s->furniture; break;
    case TECH: ++s->tech; break;
    case AMENITIES: ++s->amenities; break;
    case HIRE: ++s->staff; break;
    case MOVE: ++s->tier; break;
    default: return false;
    }
    return true;
}

bool sim_advance(Sim *s)
{
    if (s->closed || s->week == WEEK_LIMIT) return false;
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
    ++s->week;
    return true;
}

int sim_encode(const Sim *s, char *out, size_t size)
{
    return snprintf(out, size, "OFFICE1 %lld %lld %d %d %d %d %d %d %d %d %d %d\n",
        s->cash, s->last_net, s->week, s->tier, s->staff, s->furniture, s->tech,
        s->amenities, s->reputation, s->streak, s->won, s->closed);
}

bool sim_decode(const char *text, Sim *out)
{
    Sim s = {0};
    int won, closed;
    char extra;
    if (sscanf(text, "OFFICE1 %10lld %10lld %6d %2d %2d %2d %2d %2d %2d %2d %1d %1d %c",
            &s.cash, &s.last_net, &s.week, &s.tier, &s.staff, &s.furniture, &s.tech,
            &s.amenities, &s.reputation, &s.streak, &won, &closed, &extra) != 12)
        return false;
    if (s.cash < -25000 || s.cash > cash_limit || s.last_net < -25000 || s.last_net > 25000
        || s.week < 1 || s.week > WEEK_LIMIT || s.tier < 0 || s.tier >= TIER_COUNT
        || s.staff < 1 || s.staff > offices[s.tier].capacity
        || s.furniture < 0 || s.furniture > UPGRADE_LIMIT
        || s.tech < 0 || s.tech > UPGRADE_LIMIT || s.amenities < 0 || s.amenities > UPGRADE_LIMIT
        || s.reputation < 0 || s.reputation > 25 || s.streak < 0 || s.streak > THRIVE_WEEKS
        || (won != 0 && won != 1) || (closed != 0 && closed != 1) || closed != (s.cash < 0)
        || (s.streak == THRIVE_WEEKS && !won))
        return false;
    s.won = won != 0;
    s.closed = closed != 0;
    *out = s;
    return true;
}
