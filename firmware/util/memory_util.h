#ifndef _MEMORY_UTIL_H_
#define _MEMORY_UTIL_H_

#include "board.h"

enum memory_flash_result_t {
        MEMORY_FLASH_SUCCESS = 0,
        MEMORY_FLASH_ERASE_ERROR = -1,
        MEMORY_FLASH_WRITE_ERROR = -2,
        MEMORY_FLASH_VERIFY_ERROR = -3,
        MEMORY_FLASH_INVALID_PARAM_ERROR = -4
};

enum memory_flash_result_t memory_flash_region(const void *address, const void *data,
        size_t length);

#endif /* _MEMORY_UTIL_H_ */