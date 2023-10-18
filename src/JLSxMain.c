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
// This is an example, including a main() function
//


#include <stdio.h>

#include "FileIO.h"
#include "JLSx.h"



// return:
//     1 : match
//     0 : mismatch
static int suffix_match (const char *string, const char *suffix) {
    const char *p1, *p2;
    for (p1=string; *p1; p1++);
    for (p2=suffix; *p2; p2++);
    while (*p1 == *p2) {
        if (p2 <= suffix)
            return 1;
        if (p1 <= string)
            return 0;
        p1 --;
        p2 --;
    }
    return 0;
}



#define   MAX_YSZ            8192
#define   MAX_XSZ            8192
#define   BUF_MAX_LEN        (MAX_YSZ*MAX_XSZ*2)


// return:
//     -1 : exit with error
//      0 : exit normally
int main (int argc, char **argv) {
    static unsigned char img      [MAX_YSZ*MAX_XSZ];
    static unsigned char buf      [BUF_MAX_LEN] = {0};

    int ysz=-1 , xsz=-1 , near=0 , len;

    const char *p_src_fname=NULL, *p_dst_fname=NULL;
    
    if (argc < 3) {                                    // illegal arguments: print USAGE and exit
        printf("Usage:\n");
        printf("    Compress:\n");
        printf("        %s  <input-image-file(.pgm)>  <output-file(.jlsx)>  [<near>]\n" , argv[0] );
        printf("    Decompress:\n");
        printf("        %s  <input-file(.jlsx)>  <output-image-file(.pgm)>\n" , argv[0] );
        printf("\n");
        return -1;
    }
    
    p_src_fname = argv[1];
    p_dst_fname = argv[2];
    
    if (argc >= 4)
        if ( sscanf(argv[3], "%d", &near) <= 0 )
            near = 0;
    
    printf("  input  file        = %s\n" , p_src_fname);
    printf("  output file        = %s\n" , p_dst_fname);
    
    if ( suffix_match(p_src_fname, ".pgm") ) {         // src file is a pgm, compress
        
        printf("  near               = %d\n" , near);
        
        if ( loadPgmImageFile(p_src_fname, img, &ysz, &xsz) ) {
            printf("  ***Error : open %s failed\n", p_src_fname);
            return -1;
        }
        
        printf("  input image shape  = %d x %d\n" , xsz , ysz );
        printf("  input image size   = %d B\n" , xsz*ysz );
        printf("  compressing ...\n");
        
        len = JLSxCompress(buf, img, ysz, xsz, near);
        
        if (len < 0) {
            printf("  ***Error : compress failed\n");
            return -1;
        }
        
        printf("  output size        = %d B\n" , len );
        printf("  compression rate   = %.5f\n" , (1.0*xsz*ysz)/len );
        printf("  compression bpp    = %.5f\n" , (8.0*len)/(xsz*ysz) );
        
        if ( writeBytesToFile(p_dst_fname, buf, len) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
        
    } else {
        
        len = loadBytesFromFile(p_src_fname, buf, BUF_MAX_LEN);

        if (len < 0) {
            printf("  ***Error : open %s failed\n", p_src_fname);
            return -1;
        }
        
        printf("  input size         = %d B\n" , len );
        printf("  decompressing ...\n");
        
        if ( JLSxDecompress(buf, img, &ysz, &xsz, &near) < 0 ) {
            printf("  ***Error : decompress failed\n");
            return -1;
        }
        
        printf("  near               = %d\n" , near);
        printf("  output image shape = %d x %d\n"  , xsz , ysz );
        
        if ( writePgmImageFile(p_dst_fname, img, ysz, xsz) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
    }

    return 0;
}
