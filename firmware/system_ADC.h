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


#ifndef ADC_H_
#define ADC_H_
#include "ch.h"
#include "hal.h"

#define ADC_CHANNELS 4
struct ADCSamples{
        uint16_t raw_samples[ADC_CHANNELS];
};

void system_adc_init(void);
struct ADCSamples *  system_adc_sample(void);

#endif /* ADC_H_ */
