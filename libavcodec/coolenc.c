/*
  Encoder for the cool image format
  Authors: Thomas Ady && Pranav Rajan
  2/17/19
 */

#include "libavutil/imgutils.h"
#include "libavutil/avassert.h"
#include "avcodec.h"
#include "bytestream.h"
#include "cool.h"
#include "internal.h"

static const uint32_t monoblack_pal[] = { 0x000000, 0xFFFFFF };
static const uint32_t rgb565_masks[]  = { 0xF800, 0x07E0, 0x001F };

static av_cold int cool_encode_init(AVCodecContext *avctx){

  //For use with RGB 555 or 565
    avctx->bits_per_coded_sample = 16;
    return 0;
}

static int cool_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                            const AVFrame *pict, int *got_packet)
{
    const AVFrame * const p = pict;
    int n_bytes_image, n_bytes_per_row, n_bytes, i, n, hsize, ret;
    const uint32_t *pal = NULL;
    // uint32_t palette256[256];
    int pad_bytes_per_row, pal_entries = 0, compression = COOL_RGB;
    int bit_count = avctx->bits_per_coded_sample;
    uint8_t *ptr, *buf;

#if FF_API_CODED_FRAME
FF_DISABLE_DEPRECATION_WARNINGS
    avctx->coded_frame->pict_type = AV_PICTURE_TYPE_I;
    avctx->coded_frame->key_frame = 1;
FF_ENABLE_DEPRECATION_WARNINGS
#endif

        compression = COOL_BITFIELDS;
        pal = rgb565_masks; // abuse pal to hold color masks
        pal_entries = 3;
   
    if (pal && !pal_entries) pal_entries = 1 << bit_count;
    n_bytes_per_row = ((int64_t)avctx->width * (int64_t)bit_count + 7LL) >> 3LL;
    pad_bytes_per_row = (4 - n_bytes_per_row) & 3;
    n_bytes_image = avctx->height * (n_bytes_per_row + pad_bytes_per_row);

    // Cool file header encoding
#define SIZE_COOLFILEHEADER 10
#define SIZE_COOLINFOHEADER 36
    hsize = SIZE_COOLFILEHEADER + SIZE_COOLINFOHEADER + (pal_entries << 2);
    n_bytes = n_bytes_image + hsize;
    if ((ret = ff_alloc_packet2(avctx, pkt, n_bytes, 0)) < 0)
        return ret;
    buf = pkt->data;
    bytestream_put_byte(&buf, 'C');                   //COOL header
    bytestream_put_byte(&buf, 'O');                   
    bytestream_put_le32(&buf, n_bytes);               // COOL FILE SIZE
    bytestream_put_le32(&buf, hsize);                 // COOLFILEHEADER HEADER OFFSET
    bytestream_put_le32(&buf, SIZE_COOLINFOHEADER);   // Info header Size
    bytestream_put_le32(&buf, avctx->width);          // COOLINFOHEADER WIDTH
    bytestream_put_le32(&buf, avctx->height);         // COOLINFOHEADER HEIGHT
    bytestream_put_le16(&buf, 1);                     // Color planes 
    bytestream_put_le16(&buf, bit_count);             // Bits per pixel
    bytestream_put_le32(&buf, compression);           // Pixel array compression
    bytestream_put_le32(&buf, n_bytes_image);         // SIZE OF RAW COLOR DATA
    bytestream_put_le32(&buf, 0);                     // PRINT RESOLUTION X
    bytestream_put_le32(&buf, 0);                     // PRINT RESOLUTION Y
    bytestream_put_le32(&buf, 0);                     // NUMBER OF COLORS IN PALETTE

    for (i = 0; i < pal_entries; i++)
        bytestream_put_le32(&buf, pal[i] & 0xFFFFFF);

    // BMP files are bottom-to-top so we start from the end...
    ptr = p->data[0] + (avctx->height - 1) * p->linesize[0];
    buf = pkt->data + hsize;
    for(i = 0; i < avctx->height; i++) {
        if (bit_count == 16) {
            const uint16_t *src = (const uint16_t *) ptr;
            uint16_t *dst = (uint16_t *) buf;
            for(n = 0; n < avctx->width; n++)
                AV_WL16(dst + n, src[n]);
        } else {
            memcpy(buf, ptr, n_bytes_per_row);
        }
        buf += n_bytes_per_row;
        memset(buf, 0, pad_bytes_per_row);
        buf += pad_bytes_per_row;
        ptr -= p->linesize[0]; // ... and go back
    }

    pkt->flags |= AV_PKT_FLAG_KEY;
    *got_packet = 1;
    return 0;
}



AVCodec ff_cool_encoder = {
    .name           = "cool",
    .long_name      = NULL_IF_CONFIG_SMALL("COOL image (CS 3505 Spring 2019)"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_COOL,
    .init           = cool_encode_init,
    .encode2        = cool_encode_frame,
    .pix_fmts       = (const enum AVPixelFormat[]){
        AV_PIX_FMT_RGB565,
	AV_PIX_FMT_NONE
  },
};
