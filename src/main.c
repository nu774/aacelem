#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <neaacdec.h>
#include "compat.h"
#include "bs.h"

#define MAX_ELEMENTS 48

typedef struct context_t {
    NeAACDecHandle decoder;
    NeAACDecFrameInfo info;
    unsigned frame_count;
    unsigned frame_total;
    unsigned num_channel_elements;
    unsigned char channel_elements[MAX_ELEMENTS];
    unsigned char element_index[MAX_ELEMENTS];
    unsigned output_element[MAX_ELEMENTS];
    FILE *output_file[MAX_ELEMENTS];
    unsigned char adts[7];
    unsigned frame_length;
    unsigned char frame[6144*64];
} context_t;

static
char *mystrsep(char **strp, const char *sep)
{
    char *tok, *s;

    if (!strp || !(tok = *strp))
        return 0;
    if (s = strpbrk(tok, sep)) {
        *s = 0;
        *strp = s + 1;
    } else
        *strp = 0;
    return tok;
}

static
int adts_frame_length(const unsigned char *header)
{
    if (header[0] != 0xff || header[1] >> 4 != 0xf)
        return -1;
    return (header[3] & 3) << 11 | header[4] << 3 | header[5] >> 5;
}

static
int skip_id3(FILE *fp, const unsigned char *header)
{
    if (!memcmp(header, "ID3", 3)) {
        unsigned tagsize;
        unsigned char buf[3] = { 0 };
        fread(buf, 1, 3, fp);
        tagsize = header[6] << 21 | buf[0] << 14 | buf[1] << 7 | buf[2];
        fseek(fp, tagsize, SEEK_CUR);
        return 1;
    } else if (!memcmp(header, "TAG", 3)) {
        fseek(fp, 0, SEEK_END);
        return 1;
    }
    return 0;
}

static
int decode_frame(context_t *ctx)
{
    NeAACDecFrameInfo info = { 0 };
    unsigned char channel_elements[MAX_ELEMENTS] = { 0 };
    unsigned char element_index[MAX_ELEMENTS] = { 0 };
    int i, n = 0;

    if (ctx->frame_count == 0) {
        unsigned long sample_rate;
        unsigned char channels;
        if (NeAACDecInit(ctx->decoder, ctx->frame, ctx->frame_length,
                         &sample_rate, &channels) < 0) {
            fprintf(stderr, "ERROR: NeAACDecInit() failed\n");
            return -1;
        }
    }
    if (!NeAACDecDecode(ctx->decoder, &info, ctx->frame, ctx->frame_length)) {
        fprintf(stderr, "\nERROR: NeAACDecDecode() failed\n");
        return -1;
    }
    for (i = 0; i < MAX_ELEMENTS; ++i) {
        int id = info.syntax_element_id[i];
        if (id <= 3) {
            channel_elements[n] = id;
            element_index[n] = i;
            ++n;
        }
        else if (id == 7)
            break;
    }
    if (n != ctx->num_channel_elements ||
        memcmp(ctx->channel_elements, channel_elements, 7))
    {
        if (ctx->output_file[0] == 0) {
            fprintf(stderr, "frame %d: channel elements:",
                    ctx->frame_count + 1);
            for (i = 0; i < n; ++i) {
                char *tab[] = { "SCE", "CPE", "CCE", "LFE" };
                fprintf(stderr, " %s", tab[channel_elements[i]]);
            }
            putchar('\n');
        }
    }
    ctx->num_channel_elements = n;
    memcpy(ctx->channel_elements, channel_elements, n);
    memcpy(ctx->element_index, element_index, n);
    memcpy(&ctx->info, &info, sizeof(info));
    return n;
}

static
void output_frame(context_t *ctx, FILE *fp, int nth)
{
    unsigned start, count;
    bitstream_t ibs, obs, adts;
    int nchannels[] = { 1, 2, 2, 1 };

    start = ctx->info.syntax_element_pos[ctx->element_index[nth]];
    count = ctx->info.syntax_element_pos[ctx->element_index[nth] + 1] - start;

    bitstream_init(&ibs, ctx->frame, ctx->frame_length);
    bitstream_advance(&ibs, start);
    bitstream_init(&obs, 0, 0);
    while (count > 0) {
        unsigned bits = count >= 16 ? 16 : count;
        unsigned n = bitstream_get(&ibs, bits);
        bitstream_put(&obs, n, bits);
        count -= bits;
    }
    bitstream_put(&obs, 7, 3);
    bitstream_byte_align(&obs);

    bitstream_init(&adts, 0, 0);

    bitstream_put(&adts, 0xfff, 12);
    bitstream_put(&adts, (ctx->adts[1] >> 3) & 0x1, 1);
    bitstream_put(&adts, 0, 2);
    bitstream_put(&adts, 1, 1);
    bitstream_put(&adts, 1, 2);
    bitstream_put(&adts, (ctx->adts[2] >> 2) & 0xf, 4);
    bitstream_put(&adts, 0, 1);
    bitstream_put(&adts, nchannels[ctx->channel_elements[nth]], 3);
    bitstream_put(&adts, 0, 4);
    bitstream_put(&adts, obs.current + 7, 13);
    bitstream_put(&adts, 0x7ff, 11);
    bitstream_put(&adts, 0, 2);
    bitstream_byte_align(&adts);
    fwrite(adts.data, 1, 7, fp);
    fwrite(obs.data, 1, obs.current, fp);

    bitstream_close(&ibs);
    bitstream_close(&obs);
    bitstream_close(&adts);
}

static
int scan_frames(FILE *fp)
{
    int frame_length, frame_count = 0;
    unsigned char adts[7];

    while (fread(adts, 1, 7, fp) == 7) {
        if (skip_id3(fp, adts))
            continue;
        if ((frame_length = adts_frame_length(adts)) < 0) {
            fprintf(stderr, "ERROR: Invalid ADTS header\n");
            break;
        }
        fseek(fp, frame_length - 7, SEEK_CUR);
        ++frame_count;
    }
    return frame_count;
}

static
int process_frames(context_t *ctx, FILE *fp)
{
    int percent = 0;
    ctx->decoder = NeAACDecOpen();
    while (fread(ctx->adts, 1, 7, fp) == 7) {
        int i;

        if (skip_id3(fp, ctx->adts))
            continue;
        if ((ctx->frame_length = adts_frame_length(ctx->adts)) < 0) {
            fprintf(stderr, "\nERROR: Invalid ADTS header\n");
            break;
        }
        fseek(fp, - 7, SEEK_CUR);
        if (fread(ctx->frame, 1, ctx->frame_length, fp) != ctx->frame_length) {
            break;
        }
        if (decode_frame(ctx) < 0)
            break;
        for (i = 0; i < MAX_ELEMENTS && ctx->output_file[i]; ++i) {
            if (ctx->output_element[i] < ctx->num_channel_elements)
                output_frame(ctx, ctx->output_file[i], ctx->output_element[i]);
        }
        ++ctx->frame_count;
        if (ctx->output_file[0]) {
            int n = (ctx->frame_count * 100 + .5)/ ctx->frame_total;
            if (n != percent) {
                percent = n;
                fprintf(stderr, "\r%d%%", percent);
            }
        }
    }
    if (ctx->output_file[0]) putc('\n', stderr);
    NeAACDecClose(ctx->decoder);
    return ctx->frame_count;
}

static
void usage()
{
    fprintf(stderr,
"Usage: aacelem [-a] INFILE [N1:FILE1, N2:FILE2...]\n"
"-a:     scan only\n"
"INFILE: input ADTS file name\n"
"N:      number of channel element to pull (first: 1)\n"
"FILE:   output file name\n"
    );
    exit(1);
}


int main(int argc, char **argv)
{
    int i, ch, scan_only = 0;
    context_t ctx = { 0 };
    FILE *fp;

    aacenc_getmainargs(&argc, &argv);
    setbuf(stderr, 0);

    while ((ch = getopt(argc, argv, "a")) != EOF) {
        switch (ch) {
        case 'a':
            scan_only = 1;
            break;
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;
    if (!scan_only && argc < 2)
        usage();
    else if (scan_only && argc == 0)
        usage();

    if ((fp = aacenc_fopen(argv[0], "rb")) == 0) {
        aacenc_fprintf(stderr, "ERROR: %s: %s\n", argv[0], strerror(errno));
        return 2;
    }
    if (!scan_only) {
        for (i = 0; i < argc - 1; ++i) {
            int n;
            char *s = argv[i + 1], *tok = mystrsep(&s, ":");
            if (!s || sscanf(tok, "%d", &n) != 1)
                usage();
            if ((ctx.output_file[i] = aacenc_fopen(s, "wb")) == 0) {
                aacenc_fprintf(stderr, "ERROR: %s: %s\n",
                               s, strerror(errno));
                break;
            }
            ctx.output_element[i] = n - 1;
        }
        ctx.frame_total = scan_frames(fp);
        if (ctx.frame_total == 0)
            return 2;
        rewind(fp);
    }
    process_frames(&ctx, fp);
    if (scan_only)
        return ctx.frame_total > 0 ? 0 : 2;
    else
        return ctx.frame_total == ctx.frame_count ? 0 : 2;
}
