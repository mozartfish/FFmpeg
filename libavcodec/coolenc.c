/*
  Encoder for the cool image format
  Authors: Thomas Ady && Pranav Rajan
  2/17/19
 */

#include "avcodec.h"
#include "internal.h"
#include "libavutil/imgutils.h"





AVCodec ff_cool_encoder {
    .name           = "cool",
    .long_name      = NULL_IF_CONFIG_SMALL("COOL image (CS 3505 Spring 2019)"),
    .type           = AVMEDIA_TYPE_VIDEO,
    .id             = AV_CODEC_ID_COOL,
    .init           = cool_encode_init,
    .encode2        = cool_encode_frame,
};
