#ifndef _SHARED_DATA_STRUCT_H_
#define _SHARED_DATA_STRUCT_H_

#include <stdbool.h>
#include <stdint.h>

// R5 Shared Memory Note
// - It seems that using a struct for the ATCM memory does not work 
//   (hangs when accessing memory via a struct pointer).
// - Therefore, using an array.

#define MSG_OFFSET 0
#define MSG_SIZE   32
#define PROGRESS_OFFSET (MSG_OFFSET + sizeof(uint32_t))
#define COLOR_OFFSET (PROGRESS_OFFSET + sizeof(uint32_t))

#define MEM_UINT8(addr) *(uint8_t*)(addr)
#define MEM_UINT32(addr) *(uint32_t*)(addr)
#endif
