#include "printer.h"

#include <cs2s/common/macro.h>

#define LOG_PREFIX "[" STR(PLUGIN_NAME) ":print] "

#ifdef WIN32
const uint8_t CLIENT_PRINT_PATTERN[] = {0x48, 0x85, 0xc9, 0xf, 0x84, 0x2a, 0x2a, 0x2a, 0x2a, 0x48, 0x8b, 0xc4, 0x48, 0x89, 0x58, 0x18};
const uint8_t UTIL_CLIENT_PRINT_ALL_PATTERN[] = {0x48, 0x89, 0x5c, 0x24, 0x8, 0x48, 0x89, 0x6c, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48, 0x81, 0xec, 0x70, 0x1, 0x2a, 0x2a, 0x8b, 0xe9};
#else
const uint8_t CLIENT_PRINT_PATTERN[] = {0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x49, 0x89, 0xcf, 0x41, 0x56, 0x49, 0x89, 0xd6, 0x41, 0x55, 0x41, 0x89, 0xf5, 0x41, 0x54, 0x4c, 0x8d, 0xa5, 0xa0, 0xfe, 0xff, 0xff};
const uint8_t UTIL_CLIENT_PRINT_ALL_PATTERN[] = {0x55, 0x48, 0x89, 0xe5, 0x41, 0x57, 0x49, 0x89, 0xd7, 0x41, 0x56, 0x49, 0x89, 0xf6, 0x41, 0x55, 0x41, 0x89, 0xfd};
#endif

cs2s::plugin::service::Pattern<decltype(ClientPrint)> ClientPrintPattern{
    CLIENT_PRINT_PATTERN,
    sizeof(CLIENT_PRINT_PATTERN),
};

cs2s::plugin::service::Pattern<decltype(UTIL_ClientPrintAll)> UTIL_ClientPrintAllPattern{
    UTIL_CLIENT_PRINT_ALL_PATTERN,
    sizeof(UTIL_CLIENT_PRINT_ALL_PATTERN),
};

PluginPrinterService::PluginPrinterService(LoggingChannelID_t log, cs2s::plugin::service::PluginLibraryService* libraries)
    : PluginService(log)
    , libraries(libraries)
{
}

bool PluginPrinterService::Load(PluginId id, ISmmAPI* ismm, bool late)
{
    cs2s::plugin::service::Library server_library;
    if (!this->libraries->Resolve(GAME_BIN_DIRECTORY, "server", &server_library))
    {
        Log_Error(this->log, LOG_PREFIX "Could not determine game bin directory\n");
        return false;
    }

    this->client_print = server_library.Match(UTIL_ClientPrintAllPattern);
    if (!this->client_print)
    {
        Log_Error(this->log, LOG_PREFIX "failed to locate ClientPrint\n");
        return false;
    }

    this->client_print_all = server_library.Match(UTIL_ClientPrintAllPattern);
    if (!this->client_print_all)
    {
        Log_Error(this->log, LOG_PREFIX "failed to locate UTIL_ClientPrintAll\n");
        return false;
    }

    return true;
}

bool PluginPrinterService::Unload()
{
    this->server = {};  // Invoke destructor to release handle on shared library
    this->client_print = nullptr;
    this->client_print_all = nullptr;

    return true;
}
