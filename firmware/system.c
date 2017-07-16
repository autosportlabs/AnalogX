/*
 * ShiftX2 firmware
 *
 * Copyright (C) 2016 Autosport Labs
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

#include "system.h"

#include "analogx_api.h"
#include "settings.h"
#include "ch.h"
#include "hal.h"
#include "logging.h"
#include "system_CAN.h"

#define _LOG_PFX "SYS:         "

/* Flag to indicate if system is initialized
 * and ready for normal operation */
static bool system_initialized = false;

/* Get / Set system initialized flag */
void set_system_initialized(bool initialized)
{
        system_initialized = initialized;
}

bool get_system_initialized(void)
{
        return system_initialized;
}


/* Broadcast some current stats */
void broadcast_stats(void)
{
        CANTxFrame can_stats;
        prepare_can_tx_message(&can_stats, CAN_IDE_EXT, get_can_base_id() + API_STATS);

        /* these values reserved for future use */
        can_stats.data8[0] = MAJOR_VER;
        can_stats.data8[1] = MINOR_VER;
        can_stats.data8[2] = PATCH_VER;
        can_stats.DLC = 3;
        canTransmit(&CAND1, CAN_ANY_MAILBOX, &can_stats, MS2ST(CAN_TRANSMIT_TIMEOUT));
        log_info(_LOG_PFX "Broadcast stats\r\n");
}

/* perform a soft reset of this processor */
void reset_system(void)
{
        log_info(_LOG_PFX "Resetting System\r\n");
        chThdSleepMilliseconds(SYSTEM_RESET_DELAY);
        NVIC_SystemReset();
}

/* Check if we're in a state where we need to reset the system */
void check_system_state(void)
{
        /* NOOP for now */
}
