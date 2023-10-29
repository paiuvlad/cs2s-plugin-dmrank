#pragma once

#include <cstdint>
#include <cstdio>
#include <utility>

#include <tier0/logging.h>

#include <cs2s/common/macro.h>
#include <cs2s/plugin/service.h>
#include <cs2s/plugin/service/library.h>

// Avoid creating LOG_PREFIX in global namespace
#define PRINTER_LOG_PREFIX "[" STR(PLUGIN_NAME) "] "

// vendor/hl2sdk/game/shared/shareddefs.h
#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

// vendor/hl2sdk/game/server/util.h
void ClientPrint(
    CEntityInstance* player,
    int msg_dest,
    const char* msg_name,
    const char* param1 = nullptr,
    const char* param2 = nullptr,
    const char* param3 = nullptr,
    const char* param4 = nullptr
);

void UTIL_ClientPrintAll(
    int msg_dest,
    const char* msg_name,
    const char* param1 = nullptr,
    const char* param2 = nullptr,
    const char* param3 = nullptr,
    const char* param4 = nullptr
);

template<typename... Args>
bool format(std::string& into, const char* fmt, Args&&... args)
{
    int size = snprintf(nullptr, 0, fmt, args...);
    if (size <= 0)
    {
        return false;
    }

    char* buffer = new char[size];
    snprintf(buffer, size, fmt, args...);
    into.assign(buffer, size);
    return true;
}

class PluginPrinterService : public cs2s::plugin::PluginService
{
protected:
    cs2s::plugin::service::PluginLibraryService* libraries;

    cs2s::plugin::service::Library server{};

public:
    decltype(ClientPrint)* client_print{nullptr};
    decltype(UTIL_ClientPrintAll)* client_print_all{nullptr};

public:
    PluginPrinterService(LoggingChannelID_t log, cs2s::plugin::service::PluginLibraryService* libraries);

public:
    bool Load(PluginId id, ISmmAPI* ismm, bool late) override;
    bool Unload() override;

public:
    template<typename... Args>
    void Print(CEntityInstance* player, int hud, const char* fmt, Args&&... args) const
    {
        std::string buffer;
        if (!format(buffer, fmt, std::forward<Args>(args)...))
        {
            Log_Error(this->log, PRINTER_LOG_PREFIX "Failed to interpolate format: %s\n", fmt);
            return;
        }

        this->client_print(player, hud, buffer.c_str(), nullptr, nullptr, nullptr, nullptr);
    }

    void Print(CEntityInstance* player, int hud, const char* fmt) const
    {
        this->client_print(player, hud, fmt, nullptr, nullptr, nullptr, nullptr);
    }

    template<typename... Args>
    void Print(int hud, const char* fmt, Args&&... args) const
    {
        std::string buffer;
        if (!format(buffer, fmt, std::forward<Args>(args)...))
        {
            Log_Error(this->log, PRINTER_LOG_PREFIX "Failed to interpolate format: %s\n", fmt);
            return;
        }

        this->client_print_all(hud, buffer.c_str(), nullptr, nullptr, nullptr, nullptr);
    }

    void Print(int hud, const char* fmt) const
    {
        this->client_print_all(hud, fmt, nullptr, nullptr, nullptr, nullptr);
    }
};
