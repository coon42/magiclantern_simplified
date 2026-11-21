#ifndef __PLATFORM_MMU_PATCHES_H__
#define __PLATFORM_MMU_PATCHES_H__

#include "patch.h"

#if CONFIG_FW_VERSION == 160 // ensure our hard-coded patch addresses are not broken
                             // by a FW upgrade

struct patch early_data_patches[] =
{
};

static uint8_t prompt_old[] = "[%d]>";
static uint8_t prompt_new[] = "{%d}>";

struct patch normal_data_patches[] =
{
    {
        .addr = (uint8_t *)0xE004500E,   // in "%s[%d]>" at 0xE004500C
        .old_values = prompt_old,
        .new_values = prompt_new,
        .size = 5,                        // > 4, so the pointers are used
        .description = "drysh prompt test"
    }
};

struct function_hook_patch early_code_patches[] =
{
};

struct function_hook_patch normal_code_patches[] =
{
};

#endif // RP FW_VERSION 160
#endif // __PLATFORM_MMU_PATCHES_H__