#pragma once

#include <utility>
#include <string>
#include <vector>

#include <tier0/logging.h>
#include <const.h>

#include <KeyValues.h>

#include <ISmmPlugin.h>

#include <cs2s/plugin/service/library.h>
#include <cs2s/plugin/service/detour.h>
#include <cs2s/plugin/service/event.h>

#include "hooks.h"

struct Player
{
    // Intrinsic
    std::string name;
    uint64_t steam_id{0};
    bool bot{false};

    // Plugin
    int32_t rating{0};

    struct
    {
        uint32_t kills{0};
    } life;

    struct
    {
        int32_t rating_initial{0};
        uint32_t kills{0};
        uint32_t kills_headshot{0};
        uint32_t deaths{0};
        uint32_t bullets_fired_rifle{0};
        uint32_t bullets_fired_sniper{0};
        uint32_t bullets_fired_smg{0};
        uint32_t bullets_fired_deagle{0};
        uint32_t bullets_fired_pistol{0};
        uint32_t bullets_hit_rifle{0};
        uint32_t bullets_hit_sniper{0};
        uint32_t bullets_hit_smg{0};
        uint32_t bullets_hit_deagle{0};
        uint32_t bullets_hit_pistol{0};
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

    // Tracking (connected, Player)
    std::vector<std::pair<bool, Player>> players;

    // Config
    struct {
        bool chat{false};

        uint32_t points_kill_player{5};
        uint32_t points_kill_bot{2};
        uint32_t points_headshot_bonus{1};
        float points_knife_multiplier{3.0f};
        float points_death_multiplier{1.0f};
        uint32_t points_minimum{2};
        uint32_t points_maximum{20};
        uint32_t points_initial{1337};
        uint32_t points_suicide_deduction{0};
        uint32_t points_reduced_minimum_player_count{1};
        float points_reduced_multiplier{0.4f};
        uint32_t points_normal_minimum_player_count{4};

        uint32_t rank_unranked_kills{20};
        float rank_unranked_boost{2.0};
        bool rank_show_all{true};
        int rank_allow_reset{1};  // 2 VIP, 1 any, 0 none

        struct
        {
            uint32_t points;
            const char* message;
        } streaks[7] {
            {3, "Dominating"},
            {5, "Killingspree"},
            {7, "Rampage"},
            {9, "Monsterkill"},
            {11, "Unstoppable"},
            {13, "Ludicrous"},
            {15, "Godlike"},
        };
    } config;

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
