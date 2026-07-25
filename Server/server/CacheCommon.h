#pragma once
#include "server.h"

// -------------------------------------------------------------------
// Shared cache utility for offline startup optimization.
// All code is gated behind CACHE_OLD_ITEM_DEFS (see server.h).
// -------------------------------------------------------------------

// Magic constants for cache file identification (little-endian ASCII).
constexpr DWORD CACHE_MAGIC_ITEMS = 0x454D4950; // "PIME" -> PT Item Cache

// Current cache format version. Bump this when serialized structs change.
constexpr int CACHE_VERSION = 1;

/// <summary>
/// Resolve the cache directory path from server.ini.
/// Uses the same NetFolder as items.dat, falling back to ".\\Cache\\" if not configured.
/// </summary>
inline void GetCacheDirectory(char* szOut, size_t iOutLen)
{
    INI::CReader cReader("server.ini");
    std::string netFolder = cReader.ReadString("Database", "NetFolder");
    if (!netFolder.empty())
    {
        StringCbPrintfA(szOut, (DWORD)iOutLen, "%s\\cache", netFolder.c_str());
    }
    else
    {
        StringCbCopyA(szOut, (DWORD)iOutLen, ".\\cache");
    }
}

/// <summary>
/// Ensure the cache directory exists (best-effort).
/// </summary>
inline void EnsureCacheDirectoryExists(const char* szCacheDir)
{
    CreateDirectoryA(szCacheDir, NULL);
}

/// <summary>
/// Build full path for a cache file inside the cache directory.
/// </summary>
inline void GetCacheFilePath(char* szOut, size_t iOutLen, const char* szCacheDir, const char* szFileName)
{
    StringCbPrintfA(szOut, (DWORD)iOutLen, "%s\\%s", szCacheDir, szFileName);
}
