// ...existing code...
#include "utils/Algorithm.h"
#include <stdint.h>
#include <stddef.h>

static uint32_t crc_table[256];
static int crc_table_computed = 0;


static void make_crc_table(void)
{
    uint32_t c;
    int n, k;

    for (n = 0; n < 256; n++)
    {
        c = (uint32_t)n;
        for (k = 0; k < 8; k++)
        {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

void Algorithm_InitCRC32(void)
{
    if (!crc_table_computed)
    {
        make_crc_table();
    }
}

uint32_t Algorithm_UpdateCRC32(uint32_t currentCrc, const uint8_t* data, size_t length)
{
    if (!crc_table_computed) Algorithm_InitCRC32();
    uint32_t c = currentCrc ^ 0xFFFFFFFF;
    for (size_t n = 0; n < length; n++)
    {
        c = crc_table[(c ^ data[n]) & 0xff] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFF;
}

uint32_t Algorithm_CalculateCRC32(const uint8_t* data, size_t length)
{
    return Algorithm_UpdateCRC32(0, data, length);
}

