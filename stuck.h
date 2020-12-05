#ifndef STUCK_H
#define STUCK_H

#include "raylib.h"

enum ModulePhase {SPLASH, MENU, GAMEPLAY};

void SetModule(enum ModulePhase);

#endif
