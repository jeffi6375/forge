#include "forge/hook.h"
#include "forge/mem.h"
#include "forge/plugin.h"
#include "forge/proc.h"
#include <stdio.h>
#include <string.h>

typedef void (*AdjustSharpness)(void*, s32, bool);
typedef void (*CanonicalizePath)(const char* src, char* dst);
AdjustSharpness originalAdjustSharpness = NULL;

void (*original_sApp_run)(void*) = NULL;

bool (*original_MtFile_open)(void*, const char*, u32) = NULL;

CanonicalizePath canonicalizePath = NULL;

void adjustSharpness(void* unknown, s32 amount, bool ignoreSkills)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Sharpness adjusted by %ld", amount);
    svcOutputDebugString(buffer, strlen(buffer));
    originalAdjustSharpness(unknown, amount * 150, ignoreSkills);
}

void sApp_run(void* app)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "sApp_run called with app=%p", app);
    svcOutputDebugString(buffer, strlen(buffer));

    return original_sApp_run(app);
}

bool g_first = true;
bool MtFile_open(void* this, const char* path, u32 mode)
{
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer), "MtFile_open called with path=%s", path);
    svcOutputDebugString(buffer, len);

    bool result = original_MtFile_open(this, path, mode);

    len = snprintf(buffer, sizeof(buffer), "MtFile_open result: %d", result);
    svcOutputDebugString(buffer, len);

    if (g_first) {
        g_first = false;
        canonicalizePath = (CanonicalizePath)(g_mainTextAddr + 0x8292EC);

        char canonicalPath[512];
        canonicalizePath(path, canonicalPath);

        len = snprintf(buffer, sizeof(buffer), "Canonical path: %s", canonicalPath);
        svcOutputDebugString(buffer, len);

        forge_plugin_loadPlugins();
    }

    return result;
}

void forgeMain()
{
    // hookCreate((void*)(g_mainTextAddr + 0x2AD288), (void*)(adjustSharpness), (void**)(&originalAdjustSharpness));
    // hookCreate((void*)(g_mainTextAddr + 0xB8692C), (void*)(sApp_run), (void**)(&original_sApp_run));
    hookCreate((void*)(g_mainTextAddr + 0x82B1EC), (void*)(MtFile_open), (void**)(&original_MtFile_open));
}

void virtmemSetup(void);

void forgeInit()
{
    envSetup(NULL, procGetHandle(), NULL);
    virtmemSetup();
    memSetup();
    forgeMain();
}
