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

  //For use with RGB 555
    avctx->bits_per_coded_sample = 16;
    return 0;
}

static int cool_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                            const AVFrame *pict, int *got_packet)
{
    const AVFrame * const p = pict;
    int n_bytes_image, n_bytes, i, n, hsize, ret;
    const uint32_t *pal = rgb565_masks;
    int pad_bytes_per_row, pal_entries = 3, compression = COOL_BITFIELDS;
    int bit_count = avctx->bits_per_coded_sample;
    uint8_t *ptr, *buf;

#if FF_API_CODED_FRAME
FF_DISABLE_DEPRECATION_WARNINGS
    avctx->coded_frame->pict_type = AV_PICTURE_TYPE_I;
    avctx->coded_frame->key_frame = 1;
FF_ENABLE_DEPRECATION_WARNINGS
#endif
  

 // BMP files are bottom-to-top so we start from the end...
    // Grab FIRST DATA IN P for the LAST ROW IN IMAGE, shift BUFFER to just beyond header
    ptr = p->data[0] + (avctx->height - 1) * p->linesize[0];
    n_bytes_image = 0;

    for(i = 0; i < avctx->height; i++) {

      const uint16_t *src = (const uint16_t *) ptr; /* Get 2B of data from ptr */
      uint16_t line_bytes = 0;

      for(n = 0; n < avctx->width; n++) /* Write each pixel in a line */
	{
	  line_bytes += 2; //2 Bytes per color
	  
	  // Single repeated pixel
	  int pix_rep = 1;
	  while (pix_rep < 16 && n < avctx->width - 1 && (src[n] & 0xF79E) == (src[n + 1] & 0xF79E)) {
	    n++;
	    pix_rep++;
	  }
	  
	}
      // Append 3 bytes of 0 to signify end of line
      line_bytes += 2;
      
	pad_bytes_per_row = (4 - line_bytes) & 3;
	n_bytes_image += line_bytes + pad_bytes_per_row;
        ptr -= p->linesize[0]; // Go up one line of the height
    }

    // Cool file header encoding
#define SIZE_COOLFILEHEADER 10
#define SIZE_COOLINFOHEADER 20
    hsize = SIZE_COOLFILEHEADER + SIZE_COOLINFOHEADER + (pal_entries << 2);
    n_bytes = n_bytes_image + hsize;

    //Allocate the data for the image
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

    /* Add mask data to header */
    for (i = 0; i < pal_entries; i++)
        bytestream_put_le32(&buf, pal[i] & 0xFFFFFF);

    // BMP files are bottom-to-top so we start from the end...
    // Grab FIRST DATA IN P for the LAST ROW IN IMAGE, shift BUFFER to just beyond header
    ptr = p->data[0] + (avctx->height - 1) * p->linesize[0];
    buf = pkt->data + hsize;
    n_bytes_image = 0;

    for(i = 0; i < avctx->height; i++) {

      const uint16_t *src = (const uint16_t *) ptr; /* Get 2B of data from ptr */
      uint16_t line_bytes = 0;

      for(n = 0; n < avctx->width; n++) /* Write each pixel in a line */
	{
	  line_bytes += 2; //2 Bytes per color

	  int pix_rep = 1;
	   while (pix_rep < 16 && n < avctx->width - 1 && (src[n] & 0xF79E) == (src[n + 1] & 0xF79E)) {
	     n++;
	     pix_rep++;
	    av_log(NULL, AV_LOG_INFO, "Copy pixel hit\n");
	  }

	   uint16_t data = 0x0000;
	   data += (src[n] & 0xF000); //4 bit Red
	   data += (src[n] & 0x780) << 1; //4 bit green
	   data += (src[n] & 0x001E) << 3; //4 bit blue
	   data += pix_rep;                // 4 bit repeat pixel
	  bytestream_put_le16(&buf, data );
	}
      // Append 3 bytes of 0 to signify end of line
      line_bytes += 2;
      bytestream_put_le16(&buf, 0x0000);
      
	pad_bytes_per_row = (4 - line_bytes) & 3;
        memset(buf, 0, pad_bytes_per_row);
        buf += pad_bytes_per_row;
        ptr -= p->linesize[0]; // Go up one line of the height
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


