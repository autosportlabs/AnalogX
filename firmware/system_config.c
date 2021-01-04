/*
 * RaceCapture/Pro Stimulator firmware
 *
 * Copyright (C) 2019 Autosport Labs
 *
 * This file is part of the Race Capture firmware suite
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details. You should
 * have received a copy of the GNU General Public License along with
 * this code. If not, see <http://www.gnu.org/licenses/>.
 */
#include "system_config.h"
#include "analogx_api.h"
#include "logging.h"
#include "settings.h"
#include "ch.h"
#include "hal.h"
#include "string.h"
#include "memory_util.h"

#define _LOG_PFX "[CONFIG]       "

#define DEFAULT_CONFIG {                \
         { MAJOR_VER,                   \
           MINOR_VER,                   \
           PATCH_VER                    \
         },                             \
         { DEFAULT_SAMPLE_RATE          \
         }                              \

static Configuration g_config = DEFAULT_CONFIG;
static bool g_initialized = false;

static const volatile Configuration g_saved_config  __attribute__((section(".config\n\t#")));

static const VersionInfo* get_current_version_info(void)
{
        static const VersionInfo vi = {
                .major = MAJOR_VER,
                .minor = MINOR_VER,
                .patch = PATCH_VER,
        };
        return &vi;
}

static enum memory_flash_result_t flash_config(void)
{
        log_info(_LOG_PFX "flash, %x, %x, %d\r\n", (void *)&g_saved_config, (void *)&g_config, sizeof(Configuration));
        enum memory_flash_result_t rc = memory_flash_region((void *)&g_saved_config, (void *)&g_config, sizeof(Configuration));
        log_info(_LOG_PFX "Flash configuration result: %d\r\n", rc);
        return rc;
}

static bool version_changed(const VersionInfo *pv)
{
        const VersionInfo* cv = get_current_version_info();
        const bool changed = cv->major != pv->major ||
                             cv->minor != pv->minor;
        if (changed) {
                log_info(_LOG_PFX "Version changed %d.%d -> %d.%d\r\n", pv->major, pv->minor, cv->major, cv->minor);
        }
        return changed;
}

void init_configuration(void)
{
        memcpy(&g_config, (void *)&g_saved_config, sizeof(Configuration));
        if (version_changed(&g_config.version_info)) {
                log_info(_LOG_PFX "Version changed! flash default config\r\n");
                g_config = (Configuration) DEFAULT_CONFIG;
                flash_config();
        }
        g_initialized = true;
}

void check_flash_configuration(void)
{
        if (!g_initialized) {
                init_configuration();
        }
        if (memcmp((void *)&g_config, (void *)&g_saved_config, sizeof(Configuration)) != 0) {
                flash_config();
        } else {
                log_info(_LOG_PFX "Config not changed, not flashing.\r\n");
        }
}

Configuration * get_config(void)
{
        return &g_config;
}
