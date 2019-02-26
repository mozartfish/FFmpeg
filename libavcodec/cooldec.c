/*
  Decoder for the cool image format
  Authors: Thomas Ady && Pranav Rajan
  2/17/19
 */

#include <inttypes.h>

#include "avcodec.h"
#include "bytestream.h"
#include "cool.h"
#include "internal.h"

static int cool_decode_frame(AVCodecContext *avctx,
                            void *data, int *got_frame,
                            AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size       = avpkt->size;
    AVFrame *p         = data;
    unsigned int fsize, hsize;
    int width, height;
    unsigned int depth;
    BiCompression comp;
    unsigned int ihsize;
    int i, linesize, ret;
    uint8_t *ptr;
    int dsize;
    const uint8_t *buf0 = buf;
    GetByteContext gb;

    if (buf_size < 14) {
        av_log(avctx, AV_LOG_ERROR, "buf size too small (%d)\n", buf_size);
        return AVERROR_INVALIDDATA;
    }


    /* Start header read */


    if (bytestream_get_byte(&buf) != 'C' ||
        bytestream_get_byte(&buf) != 'O') {
        av_log(avctx, AV_LOG_ERROR, "bad magic number\n");
        return AVERROR_INVALIDDATA;
    }

    fsize = bytestream_get_le32(&buf);
    if (buf_size < fsize) {
        av_log(avctx, AV_LOG_ERROR, "not enough data (%d < %u), trying to decode anyway\n",
               buf_size, fsize);
        fsize = buf_size;
    }

    hsize  = bytestream_get_le32(&buf); /* header size */
    ihsize = bytestream_get_le32(&buf); /* info header size */
    if (ihsize + 14LL > hsize) {
        av_log(avctx, AV_LOG_ERROR, "invalid header size %u\n", hsize);
        return AVERROR_INVALIDDATA;
    }

    /* sometimes file size is set to some headers size, set a real size in that case */
    if (fsize == 14 || fsize == ihsize + 14)
        fsize = buf_size - 2;

    if (fsize <= hsize) {
        av_log(avctx, AV_LOG_ERROR,
               "Declared file size is less than header size (%u < %u)\n",
               fsize, hsize);
        return AVERROR_INVALIDDATA;
    }


    width  = bytestream_get_le32(&buf); /* Get height */
    height = bytestream_get_le32(&buf); /* Get width */
    

    /* planes */
    if (bytestream_get_le16(&buf) != 1) {
        av_log(avctx, AV_LOG_ERROR, "invalid COOL header\n");
        return AVERROR_INVALIDDATA;
    }

    depth = bytestream_get_le16(&buf); /* Bits per pixel */

    comp = bytestream_get_le32(&buf); /* Pixel array compression */

    if (comp != COOL_RGB && comp != COOL_BITFIELDS && comp != COOL_RLE4 &&
        comp != COOL_RLE8) {
        av_log(avctx, AV_LOG_ERROR, "COOL coding %d not supported\n", comp);
        return AVERROR_INVALIDDATA;
    }

    /* End header read */

    /* Set dimensions for image */
    ret = ff_set_dimensions(avctx, width, height > 0 ? height : -(unsigned)height);
    if (ret < 0) {
        av_log(avctx, AV_LOG_ERROR, "Failed to set dimensions %d %d\n", width, height);
        return AVERROR_INVALIDDATA;
    }

    avctx->pix_fmt = AV_PIX_FMT_RGB565; /* Set pixel format to RGB 565 */

    if ((ret = ff_get_buffer(avctx, p, 0)) < 0)
        return ret;
    p->pict_type = AV_PICTURE_TYPE_I;
    p->key_frame = 1;

    buf   = buf0 + hsize; /* Move buffer to just beyond the header */
    dsize = buf_size - hsize; /* data size */

    if (height > 0) {
        ptr      = p->data[0] + (avctx->height - 1) * p->linesize[0];
        linesize = -p->linesize[0];
    } else {
        ptr      = p->data[0];
        linesize = p->linesize[0];
    }
        

    /* Write the data to the buffer */
            for (i = 0; i < avctx->height; i++) {
                uint16_t src = bytestream_get_le16(&buf);
                uint16_t *dst       = (uint16_t *) ptr;
		uint64_t bytes_read = 2;

                while(src != 0x0000) {

		  uint16_t rgb = (src & 0xF000) + ((src & 0x0F00) >> 1) + ((src & 0x00F0) >> 2);
		  

		  for (uint16_t pix_rep = (src & 0x000F); pix_rep > 0; pix_rep--)
		    {
                    *dst++ = av_le2ne16(rgb);
		    }

		  src = bytestream_get_le16(&buf);
		  bytes_read += 2;
		}

		if (bytes_read % 4 != 0)
		  bytestream_get_le16(&buf);

                ptr += linesize;
            }
            
    

    *got_frame = 1;

    return buf_size;
}

AVCodec ff_cool_decoder = {
   .name           = "cool",
    .long_name      = NULL_IF_CONFIG_SMALL("COOL image (CS 3505 Spring 2019)"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_COOL,
    .decode           = cool_decode_frame,
   .capabilities   = AV_CODEC_CAP_DR1,
};
