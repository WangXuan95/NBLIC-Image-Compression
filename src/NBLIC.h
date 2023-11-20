// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/NBLIC
// 
// NBLIC - Niu-Bi Lossless Image Compression
//   is a lossless & near-lossless image compressor
//
// Advantages:
//   - very high compression ratio, better than state-of-the-art lossless image compression standards such as JPEG-XL lossless, AVIF lossless, etc.
//   - low code with pure C language
//   - acceptable performace (manybe faster in future versions)
//   - only single pass scan to encode a image, good for FPGA hardware streaming impl.
// 
// Development progress:
//   [y] 8-bit gray image lossless      compression/decompression is support now.
//   [y] 8-bit gray image near-lossless compression/decompression is support now.
//   [x] 24-bit RGB image lossless      compression/decompression will be supported later.
//   [x] 24-bit RGB image near-lossless compression/decompression will be supported later.
//
// Warning:
//   Currently in the development phase,
//   so there is no guarantee that the generated compressed files will be compatible with subsequent versions
//


#ifndef   __NBLIC_H__
#define   __NBLIC_H__


#define    NBLIC_MAX_HEIGHT    10240
#define    NBLIC_MAX_WIDTH     10240

#define    MIN_EFFORT             1
#define    MAX_EFFORT             2


// function  : NBLIC image compress/decompress
//
// parameter :
//    - decode   : 0=encode  1-decode
//    - p_buf    : Pointer to the compressed stream buffer.
//                   For encode, the buffer will be written.
//                   For decode, the buffer will be read.
//    - p_img    : Pointer to the image pixel buffer.
//                   Each pixel is a 8-bit luminance value, which occupy an unsigned char.
//                   For encode, the buffer will be read. Pixels should be stored in this buffer in raster scan order (from left to right, from up to down).
//                   For decode, the buffer will be written. Pixels will be stored in this buffer in raster scan order (from left to right, from up to down).
//    - p_height : Pointer to the image height
//                   For encode, the user should specify the correct image height. The function will just read it, but not modify it.
//                   For decode, the user do not need to specify the height, instead, we will get the image height in this pointer, which is parsed from the compressed file header).
//    - p_width  : Pointer to the image width
//                   For encode and decode, its operation method is just as same as p_height.
//    - p_near   : Pointer to the near value of near-lossless compression
//                   0  : lossless
//                   >0 : near-lossless
//                   For encode and decode, its operation method is just as same as p_height.
//    - p_effort : Pointer to the effort value, the larger, the higher compression ratio and the slower encode/decode speed
//                   1  : fast
//                   2  : slow
//                   For encode and decode, its operation method is just as same as p_height.
//
// return :
//    - For encode : positive value : compressed stream length
//                               -1 : failed
//    - For decode :              0 : success
//                               -1 : failed
//
extern int NBLICcodec (int decode, unsigned char *p_buf, unsigned char *p_img, int *p_height, int *p_width, int *p_near, int *p_effort);


#endif // __NBLIC_H__
