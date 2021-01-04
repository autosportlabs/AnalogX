/*
 * Race Capture Firmware
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


#include "ch.h"
#include "hal.h"
#include "memory_util.h"

#include "stm32f0xx_flash.h"
#include "logging.h"
#include "string.h"

typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
#define FLASH_TOT_SIZE 0x8000 // 32K
#define FLASH_PAGE_SIZE 0x400 // 1K
#define FLASH_EE_REGION (FLASH_BASE + FLASH_TOT_SIZE - FLASH_PAGE_SIZE)
#define FLASH_EE_REGION_END FLASH_EE_REGION + FLASH_PAGE_SIZE

/* STM32F3 is on 2K page sizes */
static uint32_t selectFlashSector(const void *address)
{
    uint32_t addr = (uint32_t) address;
    return addr % FLASH_PAGE_SIZE == 0 ? addr : 0;
}

enum memory_flash_result_t memory_flash_region(const void *address, const void *data,
        size_t length)
{
        uint32_t flash_page = selectFlashSector(address);
        if (!flash_page)
                return MEMORY_FLASH_INVALID_PARAM_ERROR;

        enum memory_flash_result_t rc = MEMORY_FLASH_SUCCESS;

        /* adjust length to word boundary */
        length += (length % sizeof(uint32_t) == 0) ? 0 : sizeof(uint32_t) - length % sizeof(uint32_t);

        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR | FLASH_FLAG_EOP);

        /*flash all sectors in range */
        for (size_t i = flash_page; i < flash_page + length; i += FLASH_PAGE_SIZE) {
                FLASH_Status rc = FLASH_ErasePage(i);
                log_info("Erasing page %x, status %d\r\n", flash_page, rc);
        }

        uint32_t addrTarget = (uint32_t) address;
        uint32_t *dataTarget = (uint32_t *) data;

        size_t data_index = 0;
        for (size_t i = 0; i < length; i+=sizeof(uint32_t)) {
            FLASH_Status rc = FLASH_ProgramWord(addrTarget + i, dataTarget[data_index++]);
            log_info("Flashing address %x ; rc %d\r\n", addrTarget + i, rc);
            if (rc != FLASH_COMPLETE) {
                rc = MEMORY_FLASH_WRITE_ERROR;
                break;
            }
        }
        FLASH_Lock();

        /* Verify memory */
        if (memcmp(address, data, length) != 0)
                rc = MEMORY_FLASH_VERIFY_ERROR;

        log_info("memory_device_flash_region rc %d\r\n", rc);
        return rc;
}
