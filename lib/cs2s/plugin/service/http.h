#pragma once

#include <vector>

#include <steam/steam_gameserver.h>

#include <ISmmPlugin.h>

#include "cs2s/plugin/service.h"

namespace cs2s::plugin::service
{

class PluginHttpService : public PluginService
{
protected:
    ISmmAPI* metamod{nullptr};

    // Must be late-initialized once the Steam API library is loaded.
    CSteamGameServerAPIContext steam_api{};
    ISteamHTTP* steam_http{nullptr};

public:
    using PluginService::PluginService;

public:
    bool Load(PluginId id, ISmmAPI* ismm, bool late) override;
    bool Unload() override;

public:
    void GameServerSteamAPIActivated();
    void GameServerSteamAPIDeactivated();

public:
    ISteamHTTP* Get();
};

}
