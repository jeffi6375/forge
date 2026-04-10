#include "forge/hook.h"
#include "forge/mem.h"
#include "forge/patch.h"
#include "forge/proc.h"
#include <stdio.h>
#include <string.h>

void (*originalAdjustSharpness)(void*, s32, bool) = NULL;

Patch g_hasSkillPatch1;
Patch g_hasSkillPatch2;

void adjustSharpness(void* unknown, s32 amount, bool ignoreSkills)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Sharpness adjusted by %ld", amount);
    svcOutputDebugString(buffer, strlen(buffer));
    originalAdjustSharpness(unknown, amount * 150, ignoreSkills);
}

void forge_main()
{
    forge_hook_create((void*)(g_mainTextAddr + 0x2AD288), (void*)(adjustSharpness), (void**)(&originalAdjustSharpness));

    const u8 bytes1[] = { 0x01, 0x00, 0xA0, 0x03 }; // moveq r0, 1
    g_hasSkillPatch1 = forge_patch_create(g_mainTextAddr + 0xE0DA8, bytes1, sizeof(bytes1), true);

    const u8 bytes2[] = { 0x01, 0x00, 0xA0, 0xE3 }; // mov r0, 1
    g_hasSkillPatch2 = forge_patch_create(g_mainTextAddr + 0xE0DD4, bytes2, sizeof(bytes2), true);
}
