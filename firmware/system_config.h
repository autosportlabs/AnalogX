/*
 * RaceCapture/Pro stimulator firmware
 *
 * Copyright (C) 2017 Autosport Labs
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

#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_
#include "ch.h"
#include "hal.h"
#include "settings.h"

typedef struct _VersionInfo {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
} VersionInfo;

typedef struct _ConfigGroup1 {
        uint8_t sample_rate_hz;
} ConfigGroup1;

typedef struct _Configuration {
        VersionInfo version_info;
        ConfigGroup1 group_1;
} Configuration;

void init_configuration(void);
Configuration * get_config(void);
void check_flash_configuration(void);
#endif /* SYSTEM_CONFIG_H_ */
