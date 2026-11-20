#include <common/bk_include.h>
#include <driver/trng.h>

#include "volc_uuid.h"

#include <stdlib.h>

#include "volc_time.h"

static const char* chars = "0123456789abcdef";

void volc_uuid_generate(char* uuid)
{
    uint8_t* byte_buffer = NULL;
    uint16_t* short_buffer = NULL;
    uint64_t random_buffer[2] = {0};

    srand(volc_get_time_ms());

    short_buffer = (uint16_t*) random_buffer;
    for (int i = 0; i < sizeof(random_buffer) / sizeof(uint16_t); i++) {
        short_buffer[i] = (uint16_t)(bk_rand() & 0xFFFF);
    }

    byte_buffer = (uint8_t*) random_buffer;
    for (int i = 0; i < sizeof(random_buffer); i++) {
        uuid[i * 2] = chars[byte_buffer[i] >> 4];
        uuid[i * 2 + 1] = chars[byte_buffer[i] & 0x0F];
    }
}