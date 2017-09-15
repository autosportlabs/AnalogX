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

#ifndef SERIAL_H_
#define SERIAL_H_
#include "ch.h"
#include "hal.h"

#define SD1_BAUD 115200
#define SD2_BAUD 115200

size_t serial_getline(SerialDriver *sdp, uint8_t *buf, size_t buf_len);
void system_serial_init_SD1(uint32_t speed);
void system_serial_init_SD2(uint32_t speed);
void system_serial_init(void);

#endif /* SERIAL_H_ */
