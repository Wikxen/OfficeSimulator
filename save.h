#ifndef OFFICE_SAVE_H
#define OFFICE_SAVE_H

#include "sim.h"

typedef enum { SAVE_OK, SAVE_MISSING, SAVE_INVALID, SAVE_IO_ERROR } SaveResult;
SaveResult load_game(Sim *s);
bool save_game(const Sim *s);

#endif
