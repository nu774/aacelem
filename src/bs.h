#ifndef BS_H
#define BS_H

typedef struct bitstream_t {
    unsigned char *data;
    unsigned capacity;
    unsigned current;
    unsigned position;
} bitstream_t;

void bitstream_init(bitstream_t *bs, unsigned char *data, unsigned size);

void bitstream_close(bitstream_t *bs);

unsigned bitstream_position(bitstream_t *bs);

unsigned bitstream_peak(bitstream_t *bs, unsigned nbits);

void bitstream_advance(bitstream_t *bs, unsigned nbits);

unsigned bitstream_get(bitstream_t *bs, unsigned nbits);

void bitstream_put(bitstream_t *bs, unsigned value, unsigned nbits);

void bitstream_rewind(bitstream_t *bs);

void bitstream_byte_align(bitstream_t *bs);

#endif
