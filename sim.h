#ifndef OFFICE_SIM_H
#define OFFICE_SIM_H

#include <stdbool.h>
#include <stddef.h>

enum { TIER_COUNT = 4, UPGRADE_LIMIT = 4, THRIVE_WEEKS = 3, THRIVE_MOOD = 85 };
typedef long long Money;

typedef struct {
    const char *name;
    int capacity;
    Money rent, move;
} Office;

typedef enum { FURNITURE, TECH, AMENITIES, HIRE, MOVE, ACTION_COUNT } Action;

typedef struct {
    Money cash, last_net;
    int week, tier, staff, furniture, tech, amenities, reputation, streak;
    bool won, closed;
} Sim;

typedef struct {
    int mood;
    Money revenue, costs, net;
} Forecast;

extern const Office offices[TIER_COUNT];
extern const char *const action_names[ACTION_COUNT];

Sim sim_new(void);
Forecast sim_forecast(const Sim *s);
Money sim_price(const Sim *s, Action action);
const char *sim_block(const Sim *s, Action action);
bool sim_buy(Sim *s, Action action);
bool sim_advance(Sim *s);
int sim_encode(const Sim *s, char *out, size_t size);
bool sim_decode(const char *text, Sim *out);

#endif
