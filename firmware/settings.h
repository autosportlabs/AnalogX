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

#ifndef SETTINGS_H_
#define SETTINGS_H_

#define MAJOR_VER 1
#define MINOR_VER 0
#define PATCH_VER 0

#define SETTINGS_CHANNEL_COUNT 9

/* how long we wait before resetting the system */
#define SYSTEM_RESET_DELAY 10

/* The timeout value while we wait
 * for an available CAN transmission slot */
#define CAN_TRANSMIT_TIMEOUT 100

/* The default sample rate at power up */
#define DEFAULT_SAMPLE_RATE 50

#endif /* SETTINGS_H_ */
