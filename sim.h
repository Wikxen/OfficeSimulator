#ifndef OFFICE_SIM_H
#define OFFICE_SIM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { TIER_COUNT = 4, UPGRADE_LIMIT = 4, THRIVE_WEEKS = 3, THRIVE_MOOD = 85 };
enum { AMENITY_COUNT = 10, STAFF_LIMIT = 16,
    HIRE_DISRUPTION_MIN = 8, HIRE_DISRUPTION_MAX = 12,
    MOVE_DISRUPTION_MIN = 22, MOVE_DISRUPTION_MAX = 28, QUIT_DISRUPTION = 8 };
typedef long long Money;

typedef struct {
    const char *name;
    int capacity, amenity_slots;
    Money rent, move;
} Office;

typedef enum { FURNITURE, TECH, AMENITIES, HIRE, MOVE, ACTION_COUNT } Action;
typedef enum { WEEK_BLOCKED, WEEK_FINISHED, WEEK_TEAMMATE_QUIT } WeekResult;

typedef struct {
    Money cash, last_net;
    int week, tier, staff, furniture, tech, amenities, reputation, streak;
    int disruption;
    uint32_t rng;
    bool won, closed;
} Sim;

typedef struct {
    int mood, mood_loss;
    Money revenue, costs, net;
} Forecast;

typedef struct {
    const char *name;
    int satisfaction;
    Money price, upkeep;
} Amenity;

extern const Office offices[TIER_COUNT];
extern const Amenity amenities[AMENITY_COUNT];
extern const char *const action_names[ACTION_COUNT];

Sim sim_new(uint32_t seed);
Forecast sim_forecast(const Sim *s);
Money sim_price(const Sim *s, Action action);
const char *sim_block(const Sim *s, Action action);
bool sim_buy(Sim *s, Action action);
int sim_quit_chance(const Sim *s);
int sim_cohesion_recovery(const Sim *s);
WeekResult sim_advance(Sim *s);
int sim_encode(const Sim *s, char *out, size_t size);
bool sim_decode(const char *text, Sim *out);

#endif
