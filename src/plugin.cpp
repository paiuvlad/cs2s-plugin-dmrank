#include "plugin.h"

#include <string>
#include <algorithm>

#include <Color.h>

#define LOG_PREFIX "[" STR(PLUGIN_NAME) "] "

// Set up Source 2 logging. Provides macros like `Log_Msg`, `Log_Warning`, and
// `Log_Error`. See tier0/logging.h for full documentation. You can change the
// color of this channel by adding your own RGB values to the `Color()`.
DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_CS2S, STR(CS2S_PLUGIN_NAME), 0, LV_MAX, Color(0, 0, 255));

// A static instance of the plugin loaded by Metamod.
Plugin g_Plugin(LOG_CS2S);
PLUGIN_EXPOSE(Plugin, g_Plugin);

Plugin::Plugin(LoggingChannelID_t log)
    : log(log)
    , libraries(log)
    , events(log)
    , http(log)
    , print(log, &this->libraries)
{
}

SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIDeactivated, SH_NOATTRIB, 0);

// Called when the plugin is loaded by Metamod. You can test this by running
// `meta load addons/cs2s-plugin` (interpolating the actual generated plugin
// path and name) in your server console.
bool Plugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
    this->metamod = ismm;

    if (!this->libraries.Load(id, ismm, late))
    {
        ismm->Format(error, maxlen, "failed to load library service");
        return false;
    }

    if (!this->events.Load(id, ismm, late))
    {
        ismm->Format(error, maxlen, "failed to load events service");
        return false;
    }

    if (!this->print.Load(id, ismm, late))
    {
        ismm->Format(error, maxlen, "failed to load print service");
        return false;
    }

    if (!this->http.Load(id, ismm, late))
    {
        ismm->Format(error, maxlen, "failed to load http service");
        return false;
    }

    PLUGIN_SAVEVARS();
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, g_pSource2Server, &this->http, &cs2s::plugin::service::PluginHttpService::GameServerSteamAPIActivated, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIDeactivated, g_pSource2Server, &this->http, &cs2s::plugin::service::PluginHttpService::GameServerSteamAPIDeactivated, false);

    void* game_resource_service_server;
    GET_V_IFACE_ANY(GetEngineFactory, game_resource_service_server, void*, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
    if (!game_resource_service_server)
    {
        ismm->Format(error, maxlen, "failed to resolve " GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
        return false;
    }


//    cs2s::plugin::service::Library engine_library;
//    if (!this->libraries.Resolve(ROOT_BIN_DIRECTORY, "engine2", &engine_library))
//    {
//        ismm->Format(error, maxlen, "failed to resolve engine2 module");
//        return false;
//    }
//
//    auto create_interface = engine_library.Resolve(cs2s::plugin::service::Symbol<decltype(CreateInterface)>{"CreateInterface"});
//    if (!create_interface)
//    {
//        ismm->Format(error, maxlen, "failed to resolve engine2::CreateInterface");
//        return false;
//    }
//
//    game_resource_service_server = create_interface(GAMERESOURCESERVICESERVER_INTERFACE_VERSION, nullptr);
//    if (!game_resource_service_server)
//    {
//        ismm->Format(error, maxlen, "failed to resolve " GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
//        return false;
//    }

#ifdef WIN32
#define GAME_ENTITY_SYSTEM_OFFSET 88
#else
#define GAME_ENTITY_SYSTEM_OFFSET 80
#endif
    this->game_entity_system = *reinterpret_cast<CGameEntitySystem**>(
        reinterpret_cast<uintptr_t>(game_resource_service_server) + GAME_ENTITY_SYSTEM_OFFSET
    );
    if (!this->game_entity_system)
    {
        ismm->Format(error, maxlen, "failed to resolve a CGameEntitySystem");
        return false;
    }
    Log_Msg(this->log, LOG_PREFIX "Got game entity system %p\n", this->game_entity_system);

    this->events.Subscribe("player_connect", this);
    this->events.Subscribe("player_disconnect", this);
    this->events.Subscribe("player_info", this);
    this->events.Subscribe("player_death", this);
    this->events.Subscribe("weapon_fire", this);

    return true;
}

// Called when the plugin is unloaded by Metamod. You can manually unload a
// plugin with `meta unload addons/cs2s-plugin`.
bool Plugin::Unload(char* error, size_t maxlen)
{
    this->libraries.Unload();
    this->events.Unload();
    this->http.Unload();
    this->print.Unload();

    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, g_pSource2Server, &this->http, &cs2s::plugin::service::PluginHttpService::GameServerSteamAPIActivated, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIDeactivated, g_pSource2Server, &this->http, &cs2s::plugin::service::PluginHttpService::GameServerSteamAPIDeactivated, false);

    Log_Msg(this->log, LOG_PREFIX "Finished unloading\n");
    return true;
}

void Plugin::GetPlayer(Player& player)
{
//    if (player.bot)
//    {
//        return;
//    }
//
//    ISteamHTTP* steam_http = this->http.Get();
//    if (steam_http == nullptr) {
//        Log_Error(this->log, LOG_PREFIX "Cannot request player, Steam API not activated!\n");
//        return;
//    }
//
//    HTTPRequestHandle request = steam_http->CreateHTTPRequest(k_EHTTPMethodGET, "https://google.com");
//    SteamAPICall_t call;
//    steam_http->SendHTTPRequest(request, &call);
//    auto callback = std::make_unique<CCallResult<Plugin, HTTPRequestCompleted_t>>();
//    callback->Set(call, this, &Plugin::HandleGetPlayer);
//    this->requests.emplace_back(std::move(callback));
}

void Plugin::HandleGetPlayer(HTTPRequestCompleted_t* response, bool failed)
{
//    Log_Msg(this->log, LOG_PREFIX "Got response! %s\n", failed ? "failed" : "succeeded");
}

void Plugin::PostPlayer(Player& player)
{
    // TODO
}

void Plugin::FireGameEvent(IGameEvent *event)
{
    if (!this->config.enabled)
    {
        return;
    }

    switch (event->GetID())
    {
    case 8:
        this->PlayerConnect(event);
        break;
    case 9:
        this->PlayerDisconnect(event);
        break;
    case 10:  // TODO: verify
        this->PlayerInfo(event);
        break;
    case 53:
        this->PlayerDeath(event);
        break;
    case 158:
        this->WeaponFire(event);
        break;
    default:
        Log_Warning(this->log, LOG_PREFIX "Received unexpected event %d\n", event->GetID());
    }
}

void Plugin::PlayerConnect(IGameEvent* event)
{
    int player_slot = event->GetPlayerSlot(userid_symbol).Get();
    auto& [player_connected, player] = this->players[player_slot];
    if (player_connected)
    {
        this->PostPlayer(player);
        Log_Warning(this->log, LOG_PREFIX "player_connect: slot %d still connected, removing\n", player_slot);
    }
    else
    {
        player_connected = true;
        this->players_count += 1;
    }

    // Overwrite player information
    player.name = event->GetString(name_symbol);
    player.steam_id = event->GetUint64(steamid_symbol);
    player.bot = event->GetBool(bot_symbol);
//    player.controller = this->game_entity_system->GetBaseEntity(CEntityIndex(player_slot + 1));
    this->GetPlayer(player);

    Log_Msg(
        this->log,
        LOG_PREFIX "Connected player %d as %s (controller %p)\n",
        player_slot,
        player.name.c_str(),
        player.controller
    );
}

void Plugin::PlayerInfo(IGameEvent* event)
{
    int player_slot = event->GetPlayerSlot(userid_symbol).Get();
    auto& [player_connected, player] = this->players[player_slot];
    if (!player_connected)
    {
        Log_Warning(this->log, LOG_PREFIX "player_info: slot %d disconnected, updating anyway\n", player_slot);
    }

    // Update with new data
    player.name = event->GetString(name_symbol);
    player.steam_id = event->GetUint64(steamid_symbol);
    player.bot = event->GetBool(bot_symbol);

    Log_Msg(this->log, LOG_PREFIX "Updated player %d as %s\n", player_slot, player.name.c_str());
}

void Plugin::PlayerDisconnect(IGameEvent* event)
{
    int player_slot = event->GetPlayerSlot(userid_symbol).Get();
    auto& [player_connected, player] = this->players[player_slot];
    if (!player_connected)
    {
        Log_Warning(this->log, LOG_PREFIX "player_disconnect: slot %d already disconnected, skipping\n", player_slot);
        return;
    }
    else
    {
        this->players_count -= 1;
    }

    // Reset values for debugging and to keep memory profile low (probably
    // unnecessary), but better safe than sorry.
    player = {};
}

void Plugin::PlayerDeath(IGameEvent* event)
{
    int target_slot = event->GetPlayerSlot(userid_symbol).Get();
    auto& [target_connect, target] = this->players[target_slot];
    if (!target_connect)
    {
        Log_Warning(this->log, LOG_PREFIX "player_death: target slot %d disconnected, continuing\n", target_slot);
    }

    int attacker_slot = event->GetPlayerSlot(attacker_symbol).Get();
    if (attacker_slot == 63)  // Suicide
    {
        target.session.deaths += 1;
        target.points = std::max(target.points - this->config.points_suicide_deduction, 0);
        return;
    }

    auto& [attacker_connected, attacker] = this->players[attacker_slot];
    if (!attacker_connected)
    {
        Log_Warning(this->log, LOG_PREFIX "player_death: attacker slot %d disconnected, continuing\n", attacker_slot);
    }

    // Completely ignore when bots kill each other
    if (target.bot && attacker.bot)
    {
        return;
    }

    // Reused checks
    bool headshot = event->GetInt(hitgroup_symbol) == HITGROUP_HEAD;
    bool unranked = attacker.session.kills < this->config.rank_unranked_kills;

    // Reset life stats for player who died and add death
    target.life = {};
    target.session.deaths += 1;

    // Add kill stats to attacker
    attacker.life.kills += 1;
    attacker.session.kills += 1;
    if (headshot)
    {
        attacker.session.kills_headshot += 1;
    }

    float points;

    // Number of points is fixed if killing or killed by a bot
    if (target.bot || attacker.bot)
    {
        points = this->config.points_kill_bot;
    }

    // If they kill another player, use the rating system
    else
    {
        // Attacker receives player.points/attacker_player.points * points_to_kill_player
        float factor = static_cast<float>(target.points) / static_cast<float>(std::max(attacker.points, 1));
        points = static_cast<float>(factor * this->config.points_kill_player);

        // Bonuses and multiplier for kill types
        if (headshot)
        {
            points += this->config.points_headshot_bonus;
        }
        bool knife = strncmp(event->GetString(weapon_symbol), "knife", 5) == 0;  // "knife" or "knife_t"
        if (knife)
        {
            points *= this->config.points_knife_multiplier;
        }

        // Adjust points for player count
        if (this->players_count < this->config.points_low_player_count)
        {
            points *= this->config.points_low_player_count_multiplier;
        }

        // Clamp the total number of points gained within the bounds
        points = std::clamp(points, this->config.points_gain_minimum, this->config.points_gain_maximum);
    }

    // Apply the points to the target
    if (!target.bot)
    {
        float points_lost_exact = points * this->config.points_death_multiplier;
        Points points_lost = std::ceil(points_lost_exact);  // Avoids an additional static_cast
        target.points = std::max(target.points - points_lost, 0);

        // Send target message
        this->print.Print(
            event->GetPlayerController(userid_symbol),
            HUD_PRINTTALK,
            CHAT_DEFAULT "[rankWS] " CHAT_RED "-%u points (%u)" CHAT_DEFAULT " - attacker: %s\n",
            points_lost,
            target.points,
            attacker.name.c_str()
        );
    }

    // Apply points to attacker
    if (!attacker.bot)
    {
        float points_gained_exact = points;
        if (unranked)
        {
            points_gained_exact *= this->config.points_unranked_multiplier;
        }
        Points points_gained = std::ceil(points_gained_exact);
        attacker.points += points_gained;

        // Send attacker message
        const char* killstreak_message = this->config.get_kill_streak(attacker.life.kills);
        if (killstreak_message)
        {
            this->print.Print(
                event->GetPlayerController(attacker_symbol),
                HUD_PRINTTALK,
                CHAT_DEFAULT "[rankWS] " CHAT_GREEN "+%u points (%u)" CHAT_DEFAULT " - %s\n",
                points_gained,
                attacker.points,
                killstreak_message
            );
        }
        else if (headshot)
        {
            this->print.Print(
                event->GetPlayerController(attacker_symbol),
                HUD_PRINTTALK,
                CHAT_DEFAULT "[rankWS] " CHAT_GREEN "+%u points (%u)" CHAT_DEFAULT " - " CHAT_BLUE "headshot" CHAT_DEFAULT ": %s\n",
                points_gained,
                attacker.points,
                target.name.c_str()
            );
        }
        else
        {
            this->print.Print(
                event->GetPlayerController(attacker_symbol),
                HUD_PRINTTALK,
                CHAT_DEFAULT "[rankWS] " CHAT_GREEN "+%u points (%u)" CHAT_DEFAULT " - killed: %s\n",
                points_gained,
                attacker.points,
                target.name.c_str()
            );
        }
    }
}

void Plugin::WeaponFire(IGameEvent* event)
{
    int player_slot = event->GetPlayerSlot(userid_symbol).Get();
    auto& [player_connected, player] = this->players[player_slot];
}
