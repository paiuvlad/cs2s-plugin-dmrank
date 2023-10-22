#pragma once

#include <cstdint>

#include <engine/igameeventsystem.h>
#include <igameevents.h>

#include <cs2s/plugin/service/library.h>

// vendor/hl2sdk/game/shared/shareddefs.h
#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

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

// vendor/hl2sdk/game/server/util.h
void UTIL_ClientPrintAll(
    int msg_dest,
    const char *msg_name,
    const char *param1 = nullptr,
    const char *param2 = nullptr,
    const char *param3 = nullptr,
    const char *param4 = nullptr
);

extern cs2s::plugin::service::Pattern<decltype(UTIL_ClientPrintAll)> UTIL_ClientPrintAllPattern;

extern GameEventKeySymbol_t attacker_symbol;
extern GameEventKeySymbol_t userid_symbol;
extern GameEventKeySymbol_t name_symbol;
extern GameEventKeySymbol_t steamid_symbol;
extern GameEventKeySymbol_t bot_symbol;
extern GameEventKeySymbol_t hitgroup_symbol;
extern GameEventKeySymbol_t weapon_symbol;
