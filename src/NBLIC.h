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

#define    MIN_EFFORT          1
#define    MAX_EFFORT          4



// function  : NBLIC image compress
//
// parameter :
//    - p_buf    : Pointer to the compressed stream buffer. The buffer will be written.
//    - p_img    : Pointer to the image pixel buffer. Each pixel is a 8-bit luminance value, which occupy an unsigned char. The buffer will be read. Pixels should be stored in this buffer in raster scan order (from left to right, from up to down).
//    - height   : image height
//    - width    : image width
//    - p_near   : Pointer to the near value of near-lossless compression. The user should specify a near value in it.
//                   0         : lossless
//                   1,2,3,... : lossy
//    - p_effort : Pointer to the effort value. The user should specify a effort value in it.
//                   1 : fastest and lowest compression ratio
//                   2 : 
//                   3 : 
//                   4 : slowest and highest compression ratio
//
// return :
//    - positive value : compressed stream length
//                  -1 : failed
//
extern int NBLICcompress   (unsigned char *p_buf, unsigned char *p_img, int height, int width, int *p_near, int *p_effort);



// function  : NBLIC image decompress
//
// parameter :
//    - p_buf    : Pointer to the compressed stream buffer, which will be read.
//    - p_img    : Pointer to the image pixel buffer. Each pixel is a 8-bit luminance value, which occupy an unsigned char. The buffer will be written. Pixels will be stored in this buffer in raster scan order (from left to right, from up to down).
//    - p_height : Pointer to the image height. The user do not need to specify it, instead, he will get the image height in this pointer, which is parsed from the compressed file header.
//    - p_width  : Pointer to the image width. The user do not need to specify it, instead, he will get the image width in this pointer, which is parsed from the compressed file header.
//    - p_near   : Pointer to the near value of near-lossless compression. The user do not need to specify it, instead, he will get the near in this pointer, which is parsed from the compressed file header.
//    - p_effort : Pointer to the effort value. The user do not need to specify it, instead, he will get the effort in this pointer, which is parsed from the compressed file header.
//
// return :
//    -   0 : success
//    -  -1 : failed
//
extern int NBLICdecompress (unsigned char *p_buf, unsigned char *p_img, int *p_height, int *p_width, int *p_near, int *p_effort);


#endif // __NBLIC_H__
