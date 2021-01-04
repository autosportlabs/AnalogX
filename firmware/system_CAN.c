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
#include "system_CAN.h"

#include "analogx_api.h"
#include "logging.h"
#include "system_serial.h"
#include "settings.h"
#include "system.h"
#include "stm32f072xb.h"

#define _LOG_PFX "SYS_CAN:     "

#define CAN_WORKER_STARTUP_DELAY    500
#define ADR1_ADDRESS_PORT           0
#define ADR2_ADDRESS_PORT           4
#define BAUD_RATE_PORT              2
#define CAN_RX_CONTROL_PORT         1

static uint32_t g_can_base_address = ANALOGX_CAN_BASE_ID;
static const CANConfig * g_selected_can_config = NULL;
/*
 * 500K baud; 36MHz clock
 */
static const CANConfig cancfg_500K = {
        CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP | CAN_MCR_NART,
        CAN_BTR_SJW(1) |
        CAN_BTR_TS1(11) | CAN_BTR_TS2(2) | CAN_BTR_BRP(5)
};

/*
 * 1M baud; 36MHz clock
 */
static const CANConfig cancfg_1MB = {
        CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP | CAN_MCR_NART,
        CAN_BTR_SJW(1) |
        CAN_BTR_TS1(11) | CAN_BTR_TS2(2) | CAN_BTR_BRP(2)
};

static void _spin_wait(void)
{
        for (uint32_t i = 0; i < 100000; i++) {
                asm("");
        }
}

static const CANConfig * _select_can_configuration(void)
{
        palSetPadMode(GPIOA, BAUD_RATE_PORT, PAL_STM32_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
        _spin_wait();
        return palReadPad(GPIOA, BAUD_RATE_PORT) == PAL_HIGH ? &cancfg_1MB : &cancfg_500K;
}

/*
 * Initialize our CAN peripheral
 */
static void init_can_gpio(void)
{
        // Remap PA11-12 to PA9-10 for CAN
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGCOMPEN;
        SYSCFG->CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

        /* Enable CAN RX path   */
        palSetPadMode(GPIOA, CAN_RX_CONTROL_PORT, PAL_STM32_MODE_OUTPUT);
        palClearPad(GPIOA, CAN_RX_CONTROL_PORT);

        /* CAN RX.       */
        palSetPadMode(GPIOA, 11, PAL_STM32_MODE_ALTERNATE | PAL_STM32_ALTERNATE(4));
        /* CAN TX.       */
        palSetPadMode(GPIOA, 12, PAL_STM32_MODE_ALTERNATE | PAL_STM32_ALTERNATE(4));

        /* Activates the CAN driver */
        canStart(&CAND1, g_selected_can_config);

        // Disable CAN filtering for now until we can verify proper operation / settings.
        // CANFilter shiftx2_can_filter = {1, 0, 1, 0, 0x000E3700, 0x1FFFFF00}; // g_can_base_address, ANALOGX_CAN_FILTER_MASK
        //canSTM32SetFilters(1, 1, &shiftx2_can_filter);
}

/*
 * Initialize our runtime parameters
 */
static void init_can_operating_parameters(void)
{
        /* Init CAN jumper GPIOs for determining base address offset */
        palSetPadMode(GPIOA, ADR1_ADDRESS_PORT, PAL_STM32_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
        palSetPadMode(GPIOA, ADR2_ADDRESS_PORT, PAL_STM32_MODE_INPUT | PAL_STM32_PUPDR_PULLUP);
        _spin_wait();

        uint32_t offset = 0;
        offset |= palReadPad(GPIOA, ADR1_ADDRESS_PORT) == PAL_HIGH ? 0x01 : 0x00;
        offset |= palReadPad(GPIOA, ADR2_ADDRESS_PORT) == PAL_HIGH ? 0x02 : 0x00;

        g_can_base_address += (ANALOGX_CAN_API_RANGE * offset);

        g_selected_can_config = _select_can_configuration();
}

void system_can_init(void)
{
        init_can_operating_parameters();
        init_can_gpio();
}

/*
 * Dispatch an incoming CAN message
 */
static bool dispatch_can_rx(CANRxFrame *rx_msg)
{
        int32_t can_id = rx_msg->IDE == CAN_IDE_EXT ? rx_msg->EID : rx_msg->SID;
        bool got_config_message = false;
        switch (can_id - g_can_base_address) {
        case API_RESET_DEVICE:
                api_reset();
                break;
        case API_SET_CONFIG_GROUP_1:
                api_set_config_group_1(rx_msg);
                got_config_message = true;
                break;
        default:
                return false;
        }
        /* if we received a configuration message then we are provisioned */
        if (got_config_message)
                set_api_is_provisioned(got_config_message);
        return true;
}

uint32_t get_can_base_id(void)
{
        return g_can_base_address;
}

/* Main worker for receiving CAN messages */
void can_worker(void)
{
        event_listener_t el;
        CANRxFrame rx_msg;
        chRegSetThreadName("CAN receiver");
        chEvtRegister(&CAND1.rxfull_event, &el, 0);

        chThdSleepMilliseconds(CAN_WORKER_STARTUP_DELAY);
        log_info(_LOG_PFX "CAN base address: %u\r\n", g_can_base_address);

        if (g_selected_can_config == &cancfg_500K) {
                log_info(_LOG_PFX "CAN baud: 500K\r\n");
        } else if (g_selected_can_config == &cancfg_1MB) {
                log_info(_LOG_PFX "CAN baud: 1MB\r\n");
        } else {
                log_info(_LOG_PFX "CAN baud: unknown / invalid\r\n");
        }

        api_send_announcement();

        while(!chThdShouldTerminateX()) {

                if (chEvtWaitAnyTimeout(ALL_EVENTS, MS2ST(1000)) == 0) {
                        /* continue to send announcements until we are provisioned */
                        if (!api_is_provisoned())
                                api_send_announcement();
                        continue;
                }
                while (canReceive(&CAND1, CAN_ANY_MAILBOX, &rx_msg, TIME_IMMEDIATE) == MSG_OK) {
                        /* Process message.*/
                        log_CAN_rx_message(_LOG_PFX, &rx_msg);
                        dispatch_can_rx(&rx_msg);
                }
        }
        chEvtUnregister(&CAND1.rxfull_event, &el);
}

/* Prepare a CAN message with the specified CAN ID and type */
void prepare_can_tx_message(CANTxFrame *tx_frame, uint8_t can_id_type, uint32_t can_id)
{
        tx_frame->IDE = can_id_type;
        if (can_id_type == CAN_IDE_EXT) {
                tx_frame->EID = can_id;
        } else {
                tx_frame->SID = can_id;
        }
        tx_frame->RTR = CAN_RTR_DATA;
        tx_frame->DLC = 8;
        tx_frame->data8[0] = 0x55;
        tx_frame->data8[1] = 0x55;
        tx_frame->data8[2] = 0x55;
        tx_frame->data8[3] = 0x55;
        tx_frame->data8[4] = 0x55;
        tx_frame->data8[5] = 0x55;
        tx_frame->data8[6] = 0x55;
        tx_frame->data8[7] = 0x55;
}
