#include "plugin.h"

#include <string>
#include <algorithm>

#include <Color.h>

#include <cs2s/common/macro.h>

#define LOG_PREFIX "[" STR(PLUGIN_NAME) "] "

// Set up Source 2 logging. Provides macros like `Log_Msg`, `Log_Warning`, and
// `Log_Error`. See tier0/logging.h for full documentation. You can change the
// color of this channel by adding your own RGB values to the `Color()`.
DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_CS2S, STR(CS2S_PLUGIN_NAME), 0, LV_MAX, Color(0, 0, 255));

// A static instance of the plugin loaded by Metamod.
Plugin g_Plugin(LOG_CS2S);
PL_EXPOSURE_FUNC(Plugin, g_Plugin);

Plugin::Plugin(LoggingChannelID_t log)
    : log(log)
    , libraries(log)
    , events(log)
{
}

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

    cs2s::plugin::service::Library server_library;
    if (!this->libraries.Resolve(GAME_BIN_DIRECTORY, "server", &server_library))
    {
        ismm->Format(error, maxlen, "failed to resolve server library");
        return false;
    }

    this->client_print_all = server_library.Match(UTIL_ClientPrintAllPattern);
    if (!this->client_print_all)
    {
        ismm->Format(error, maxlen, "failed to locate UTIL_ClientPrintAllPattern");
        return false;
    }

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

    return true;
}

void Plugin::GetPlayer(Player& player)
{
    // TODO
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
    int player_slot = event->GetInt(userid_symbol) % ABSOLUTE_PLAYER_LIMIT;
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
    this->GetPlayer(player);

    Log_Msg(this->log, LOG_PREFIX "Connected player %d as %s\n", player_slot, player.name.c_str());
}

void Plugin::PlayerInfo(IGameEvent* event)
{
    int player_slot = event->GetInt(userid_symbol) % ABSOLUTE_PLAYER_LIMIT;
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
    int player_slot = event->GetInt(userid_symbol) % ABSOLUTE_PLAYER_LIMIT;
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
    int target_slot = event->GetInt(userid_symbol) % ABSOLUTE_PLAYER_LIMIT;
    auto& [target_connect, target] = this->players[target_slot];
    if (!target_connect)
    {
        Log_Warning(this->log, LOG_PREFIX "player_death: target slot %d disconnected, continuing\n", target_slot);
    }

    int attacker_slot = event->GetInt(attacker_symbol) % ABSOLUTE_PLAYER_LIMIT;
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

    // If the player killed a bot, they should receive a fixed number of points
    if (target.bot)
    {
        attacker.points += std::ceil(this->config.points_kill_bot);
        return;
    }

    // If the player was killed by a bot, they should lose a fixed number of points
    if (attacker.bot)
    {
        attacker.points += std::ceil(this->config.points_kill_bot);
        return;
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
    }

    // Adjust points for player count
    if (this->players_count < this->config.points_low_player_count)
    {
        points *= this->config.points_low_player_count_multiplier;
    }

    // Clamp the total number of points gained within the bounds
    points = std::clamp(points, this->config.points_gain_minimum, this->config.points_gain_maximum);

    // Apply the points to the target
    float points_lost_exact = points * this->config.points_death_multiplier;
    Points points_lost = std::ceil(points_lost_exact);  // Avoids an additional static_cast
    target.points = std::max(target.points - points_lost, 0);

    // Apply points to attacker
    float points_gained_exact = points;
    if (unranked)
    {
        points_gained_exact *= this->config.points_unranked_multiplier;
    }
    Points points_gained = std::ceil(points_gained);
    attacker.points += points_gained;

    std::string message = (
        LOG_PREFIX " " +
        attacker.name + " +" + std::to_string(points_gained) +
        target.name + " -" + std::to_string(points_lost)
    );
    this->client_print_all(HUD_PRINTTALK, message.c_str(), nullptr, nullptr, nullptr, nullptr);
}

void Plugin::WeaponFire(IGameEvent* event)
{
    int player_slot = event->GetInt(userid_symbol) % ABSOLUTE_PLAYER_LIMIT;
    auto& [player_connected, player] = this->players[player_slot];
}
