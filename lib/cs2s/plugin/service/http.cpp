#include "http.h"

#include "cs2s/common/macro.h"

#define LOG_PREFIX "[" STR(PLUGIN_NAME) ":http] "

PLUGIN_GLOBALVARS()

namespace cs2s::plugin::service
{

bool PluginHttpService::Load(SourceMM::PluginId id, SourceMM::ISmmAPI* ismm, bool late)
{
    return true;
}

bool PluginHttpService::Unload()
{
    return true;
}

void PluginHttpService::GameServerSteamAPIActivated()
{
    Log_Msg(this->log, LOG_PREFIX "Steam API activating...\n");

    this->steam_api.Init();
    this->steam_http = this->steam_api.SteamHTTP();

    Log_Msg(this->log, LOG_PREFIX "Steam API activated!\n");
    RETURN_META(MRES_IGNORED);
}

void PluginHttpService::GameServerSteamAPIDeactivated()
{
    this->steam_http = nullptr;

    Log_Msg(this->log, LOG_PREFIX "Steam API deactivated!\n");
    RETURN_META(MRES_IGNORED);
}

ISteamHTTP* PluginHttpService::Get()
{
    if (!this->steam_http)
    {
        Log_Warning(this->log, LOG_PREFIX "Steam HTTP service is not available yet!\n");
        return nullptr;
    }

    return this->steam_http;
}

}
