// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/JPEG-LS_extension
// 
// A enhanced implementation of JPEG-LS extension (ITU-T T.870) image compressor/decompressor
// which will get a better compression ratio than original JPEG-LS extension,
// and also, significantly better than JPEG-LS baseline (ITU-T T.87)
//
// It now supports lossless & lossy compression of 8-bit gray images
//
// for standard documents, see:
//    JPEG-LS baseline  (ITU-T T.87) : https://www.itu.int/rec/T-REC-T.870/en
//    JPEG-LS extension (ITU-T T.870): https://www.itu.int/rec/T-REC-T.870/en
// Warning: This implementation is not compliant with these standards, although it is modified from ITU-T T.870
//

#ifndef   __JLS_X_H__
#define   __JLS_X_H__

extern int JLSxCompress   (unsigned char *p_buf, unsigned char *p_img, int ysz   , int xsz   , int near);
extern int JLSxDecompress (unsigned char *p_buf, unsigned char *p_img, int *p_ysz, int *p_xsz, int *p_near);

#endif // __JLS_X_H__
