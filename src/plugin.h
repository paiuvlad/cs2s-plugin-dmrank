#pragma once

#include <utility>
#include <string>
#include <vector>

#include <tier0/logging.h>
#include <const.h>
#include <engine/igameeventsystem.h>
#include <igameevents.h>

#include <ISmmPlugin.h>

#include <cs2s/plugin/service/library.h>
#include <cs2s/plugin/service/event.h>

#include "hooks.h"

// This type must be signed or else 0 clamp checks won't work, e.g. std::max(points - x, 0)
using Points = int32_t;

struct Player
{
    // Intrinsic
    std::string name;
    uint64_t steam_id{0};
    bool bot{false};

    // Plugin
    bool vip{false};  // TODO
    Points points{0};

    struct
    {
        uint32_t kills{0};
    } life;

    struct
    {
        Points points_initial{0};  // TODO
        uint32_t kills{0};
        uint32_t kills_headshot{0};
        uint32_t deaths{0};
        uint32_t bullets_fired_rifle{0};  // TODO
        uint32_t bullets_fired_sniper{0};  // TODO
        uint32_t bullets_fired_smg{0};  // TODO
        uint32_t bullets_fired_deagle{0};  // TODO
        uint32_t bullets_fired_pistol{0};  // TODO
        uint32_t bullets_hit_rifle{0};  // TODO
        uint32_t bullets_hit_sniper{0};  // TODO
        uint32_t bullets_hit_smg{0};  // TODO
        uint32_t bullets_hit_deagle{0};  // TODO
        uint32_t bullets_hit_pistol{0};  // TODO
    } session;
};

class Plugin final : public ISmmPlugin, public IMetamodListener, public IGameEventListener2
{
private:
    LoggingChannelID_t log;
    cs2s::plugin::service::PluginLibraryService libraries;
    cs2s::plugin::service::PluginEventService events;

    // Derived
    ISmmAPI* metamod{nullptr};

    // Reversed
    decltype(UTIL_ClientPrintAll)* client_print_all{nullptr};

    // Config
    struct {
        bool enabled{true};

        bool chat_points{false};  // TODO
        bool chat_rank{false};  // TODO

        float points_kill_player{5.0f};
        float points_kill_bot{2.0f};
        float points_headshot_bonus{1.0f};
        float points_knife_multiplier{3.0f};
        float points_death_multiplier{1.0f};
        float points_gain_minimum{2.0f};
        float points_gain_maximum{20.0f};
        Points points_initial{1337};  // TODO
        Points points_suicide_deduction{0};
        uint32_t points_low_player_count{4};
        float points_low_player_count_multiplier{0.4f};  // TODO
        float points_unranked_multiplier{2.0f};

        uint32_t rank_unranked_kills{20};  // TODO
        bool rank_show_all{true};  // TODO
        int rank_allow_reset{1};  // 2 VIP, 1 any, 0 none  // TODO

        struct
        {
            uint32_t points;
            const char* message;
        } streaks[7] {  // TODO
            {3, "Dominating"},
            {5, "Killingspree"},
            {7, "Rampage"},
            {9, "Monsterkill"},
            {11, "Unstoppable"},
            {13, "Ludicrous"},
            {15, "Godlike"},
        };
    } config;

    // Tracking (connected, Player)
    size_t players_count{0};
    std::pair<bool, Player> players[ABSOLUTE_PLAYER_LIMIT]{};  // Goes at the end since it's relatively large

public:
    explicit Plugin(LoggingChannelID_t log);

    // We frequently pass references to `self`; ensure we're never copied or moved
    Plugin(Plugin&&) = delete;
    Plugin(const Plugin&) = delete;

public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;

    const char* GetAuthor() override { return "Noah Kim"; }
    const char* GetName() override { return "CS2S Plugin"; }
    const char* GetDescription() override { return "A CS2 server plugin template."; }
    const char* GetURL() override { return "https://github.com/noahbkim/cs2s-plugin"; }
    const char* GetLicense() override { return "MIT"; };
    const char* GetVersion() override { return "0.1"; };
    const char* GetDate() override { return "2023-10-18"; };
    const char* GetLogTag() override { return "cs2s-plugin"; };

public:
    void FireGameEvent(IGameEvent *event) override;

private:
    void GetPlayer(Player& player);
    void PostPlayer(Player& player);

    void PlayerConnect(IGameEvent* event);
    void PlayerInfo(IGameEvent* event);
    void PlayerDisconnect(IGameEvent* event);
    void PlayerDeath(IGameEvent* event);
    void WeaponFire(IGameEvent* event);
};
