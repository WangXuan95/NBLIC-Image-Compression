// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/JPEG-LS_extension
// 
// A enhanced implementation of JPEG-LS extension (ITU-T T.870) image encoder/decoder
// which will get a higher compression ratio than original JPEG-LS extension
// see https://www.itu.int/rec/T-REC-T.870/en
// Warning: This implementation is not compliant with ITU-T T.870 standard.
//

#ifndef   __JLSX_H__
#define   __JLSX_H__

extern int JLSxEncode (unsigned char *pbuf, unsigned char *img, unsigned char *imgrcon, int ysz, int xsz, int NEAR);   // import from JLSx.c
extern int JLSxDecode (unsigned char *pbuf, unsigned char *img, int *pysz, int *pxsz, int *pNEAR);                     // import from JLSx.c

#endif // __JLSX_H__
