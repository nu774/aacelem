#include <stdlib.h>
#include <string.h>
#include "bs.h"

void bitstream_init(bitstream_t *bs, unsigned char *data, unsigned size)
{
    memset(bs, 0, sizeof(bitstream_t));
    if (data && size) {
        bs->data = malloc(size);
        bs->capacity = size;
        memcpy(bs->data, data, size);
    } else {
        bs->data = malloc(16);
        bs->capacity = 16;
    }
}

void bitstream_close(bitstream_t *bs)
{
    free(bs->data);
}

unsigned bitstream_position(bitstream_t *bs)
{
    return (bs->current << 3) + bs->position;
}

unsigned bitstream_peak(bitstream_t *bs, unsigned nbits)
{
    unsigned char *p = bs->data + bs->current;
    unsigned v = (*p++ << bs->position) & 0xff;
    if (nbits <= 8 - bs->position)
        return v >> (8 - nbits);
    v >>= bs->position;
    nbits = nbits - 8 + bs->position;
    for (; nbits >= 8; nbits -= 8)
        v = v << 8 | *p++;
    if (nbits > 0)
        v = v << nbits | (*p << nbits) >> 8;
    return v;
}

void bitstream_advance(bitstream_t *bs, unsigned nbits)
{
    if (nbits) {
        bs->position += nbits;
        bs->current += (bs->position >> 3);
        bs->position &= 7;
    }
}

unsigned bitstream_get(bitstream_t *bs, unsigned nbits)
{
    unsigned value = bitstream_peak(bs, nbits);
    bitstream_advance(bs, nbits);
    return value;
}

void bitstream_put(bitstream_t *bs, unsigned value, unsigned nbits)
{
    unsigned free_bits = 8 - bs->position;
    while (nbits > 0) {
        unsigned width = free_bits > nbits ? nbits : free_bits;
        unsigned new_free_bits = free_bits - width;
        unsigned v = value >> (nbits - width);
        unsigned mask = 0xffu >> (8 - width);
        mask <<= new_free_bits;
        v = (v << new_free_bits) & mask;
        if (bs->current == bs->capacity) {
            bs->capacity *= 2;
            bs->data = realloc(bs->data, bs->capacity);
        }
        bs->data[bs->current] = (bs->data[bs->current] & ~mask) | v;
        nbits -= width;
        free_bits = new_free_bits;
        if (free_bits == 0) {
            ++bs->current;
            free_bits = 8;
        }
    }
    bs->position = 8 - free_bits;
}

void bitstream_rewind(bitstream_t *bs)
{
    bs->position = bs->current = 0;
}

void bitstream_byte_align(bitstream_t *bs)
{
    if (bs->position)
        bitstream_put(bs, 0, 8 - bs->position);
}
