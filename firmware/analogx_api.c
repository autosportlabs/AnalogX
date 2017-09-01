/*
 * shiftx2_api.c
 *
 *  Created on: Feb 4, 2017
 *      Author: brent
 */
#include "analogx_api.h"

#include "logging.h"
#include "settings.h"
#include "ch.h"
#include "hal.h"
#define _LOG_PFX "API:         "

static struct ConfigGroup1 g_config_group_1 = {ANALOGX_DEFAULT_SAMPLE_RATE};

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

void api_set_config_group_1(CANRxFrame *rx_msg)
{
        if (rx_msg->DLC < 1) {
                log_info(_LOG_PFX "Invalid params for set config group 1\r\n");
                return;
        }
        uint8_t sample_rate = rx_msg->data8[0];

        set_sample_rate(sample_rate);
}

uint8_t get_sample_rate(void)
{
        return g_config_group_1.update_rate_hz;
}

void set_sample_rate(uint8_t sample_rate)
{
        g_config_group_1.update_rate_hz = sample_rate;
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
        announce.DLC = 8;
        canTransmit(&CAND1, CAN_ANY_MAILBOX, &announce, MS2ST(CAN_TRANSMIT_TIMEOUT));
        log_info(_LOG_PFX "Broadcast announcement\r\n");
}

