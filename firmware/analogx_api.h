/*
 * AnalogX firmware
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

#ifndef ANALOGX_API_H_
#define ANALOGX_API_H_
#include "ch.h"
#include "hal.h"
#include "system_CAN.h"

struct ConfigGroup1 {
        uint8_t update_rate_hz;
};

/* API offsets */
#define ANALOGX_CAN_BASE_ID                 0xE4600
#define ANALOGX_CAN_API_RANGE               256
#define ANALOGx_CAN_FILTER_MASK             0x1FFFFF00

#define ANALOGX_DEFAULT_SAMPLE_RATE         DEFAULT_SAMPLE_RATE

/* Configuration and Runtime */

#define API_ANNOUNCEMENT                    0
#define API_RESET_DEVICE                    1
#define API_STATS                           2
#define API_SET_CONFIG_GROUP_1              3

#define API_BROADCAST_SENSORS               20

/* Base API functions */
bool api_is_provisoned(void);
void set_api_is_provisioned(bool);
void api_initialize(void);
void api_set_config_group_1(CANRxFrame *rx_msg);

uint8_t get_sample_rate(void);
void set_sample_rate(uint8_t sample_rate);

void api_send_announcement(void);

#endif /* ANALOGX_API_H_ */
