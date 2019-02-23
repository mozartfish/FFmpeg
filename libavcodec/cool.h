
/*
  Authors: Thomas Ady && Pranav Rajan
  CS3505 Spring 2019
  2/16/19


  Format declarations for the cool image format
 */
#ifndef AVCODEC_COOL_H
#define AVCODEC_COOL_H

#include "avcodec.h"

typedef enum {
    COOL_RGB         =0,
    COOL_RLE8        =1,
    COOL_RLE4        =2,
    COOL_BITFIELDS   =3,
} BiCompression;

#endif
