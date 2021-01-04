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
#include "analogx_api.h"

#include "system_config.h"
#include "logging.h"
#include "settings.h"
#include "ch.h"
#include "hal.h"
#define _LOG_PFX "API:         "

static bool g_provisioned = false;

bool api_is_provisoned(void)
{
        return g_provisioned;
}

void set_api_is_provisioned(bool provisioned)
{
        g_provisioned = provisioned;
}

void api_initialize(void)
{
        /* NOOP */
}

void api_reset(void)
{
        NVIC_SystemReset();
}

void api_set_config_group_1(CANRxFrame *rx_msg)
{
        if (rx_msg->DLC < API_SET_CONFIG_GROUP_1_SIZE) {
                log_info(_LOG_PFX "Invalid params for set config group 1\r\n");
                return;
        }

        ConfigGroup1 *config_group_1 = &get_config()->group_1;

        config_group_1->sample_rate_hz = rx_msg->data8[0];

        check_flash_configuration();
}

uint8_t get_sample_rate(void)
{
        return get_config()->group_1.sample_rate_hz;
}

void api_send_announcement(void)
{
        CANTxFrame announce;
        prepare_can_tx_message(&announce, CAN_IDE_EXT, get_can_base_id());
        announce.data8[0] = SETTINGS_CHANNEL_COUNT;
        announce.data8[1] = 0x55;
        announce.data8[2] = 0x55;
        announce.data8[3] = 0x55;
        announce.data8[4] = 0x55;
        announce.data8[5] = MAJOR_VER;
        announce.data8[6] = MINOR_VER;
        announce.data8[7] = PATCH_VER;
        announce.DLC = API_ANNOUNCEMENT_SIZE;
        canTransmit(&CAND1, CAN_ANY_MAILBOX, &announce, MS2ST(CAN_TRANSMIT_TIMEOUT));
        log_info(_LOG_PFX "Broadcast announcement\r\n");
}

