#pragma once

#include <cstdint>

#include <engine/igameeventsystem.h>
#include <igameevents.h>

// vendor/hl2sdk/game/shared/shareddefs.h
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)

extern GameEventKeySymbol_t attacker_symbol;
extern GameEventKeySymbol_t userid_symbol;
extern GameEventKeySymbol_t name_symbol;
extern GameEventKeySymbol_t steamid_symbol;
extern GameEventKeySymbol_t bot_symbol;
extern GameEventKeySymbol_t hitgroup_symbol;
extern GameEventKeySymbol_t weapon_symbol;
