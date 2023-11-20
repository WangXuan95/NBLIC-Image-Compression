#include <stdio.h>

#include "FileIO.h"
#include "NBLIC.h"


#define COPY_ARRAY(p_dst,p_src,len) {                         \
    int i;                                                    \
    for (i=0; i<(len); i++)                                   \
        (p_dst)[i] = (p_src)[i];                              \
}                                                             \


static char toLower (char c) {
    if (c >= 'A' && c <= 'Z')
        return c + 32;
    else
        return c;
}


// return:
//     1 : match
//     0 : mismatch
static int suffix_match (const char *string, const char *suffix) {
    const char *p1, *p2;
    for (p1=string; *p1; p1++);
    for (p2=suffix; *p2; p2++);
    while (toLower(*p1) == toLower(*p2)) {
        if (p2 <= suffix)
            return 1;
        if (p1 <= string)
            return 0;
        p1 --;
        p2 --;
    }
    return 0;
}



const char *USAGE = 
  "|----------------------------------------------------------------------------|\n"
  "| NBLIC: a lossless & near-lossless gray 8-bit image compressor              |\n"
  "|----------------------------------------------------------------------------|\n"
  "| Compress:                                                                  |\n"
  "|   nblic_codec <input-image-file> <output-file(.nblic)> [<effort>] [<near>] |\n"
  "|     where: <input-image-file> can be .pgm, .pnm, or .bmp                   |\n"
  "|            <output-file>      can only be .nblic                           |\n"
  "|            <effort>           can be 1 (fast) or 2 (deepest)               |\n"
  "|            <near>             can be 0 (lossless) or 1,2,3,... (lossy)     |\n"
  "|----------------------------------------------------------------------------|\n"
  "| Decompress:                                                                |\n"
  "|   nblic_codec <input-file(.nblic)> <output-image-file>                     |\n"
  "|     where: <input-file>        can only be .nblic                          |\n"
  "|            <output-image-file> can be .pgm, .pnm, or .bmp                  |\n"
  "|----------------------------------------------------------------------------|\n"
  "\n";



#define   IMG_MAX_LEN   (NBLIC_MAX_HEIGHT * NBLIC_MAX_WIDTH)
#define   BUF_MAX_LEN   (IMG_MAX_LEN * 2)


// return:
//     -1 : exit with error
//      0 : exit normally
int main (int argc, char **argv) {
    static unsigned char img     [IMG_MAX_LEN];
    static unsigned char img_tmp [IMG_MAX_LEN];
    static unsigned char buf     [BUF_MAX_LEN];

    int height      = -1;
    int width       = -1;
    int effort      =  MIN_EFFORT;
    int near        =  0;
    int len         = -1;
    int in_is_nblic =  0;
    int in_is_bmp   =  0;
    int out_is_bmp  =  0;

    const char *p_src_fname=NULL, *p_dst_fname=NULL;
    
    if (argc < 3) {                                      // illegal arguments: print USAGE and exit
        printf(USAGE);
        return -1;
    }
    
    p_src_fname = argv[1];
    p_dst_fname = argv[2];
    
    if (argc >= 4)
        if ( sscanf(argv[3], "%d", &effort) <= 0 )
            effort = MIN_EFFORT;
    
    if (argc >= 5)
        if ( sscanf(argv[4], "%d", &near) <= 0 )
            near = 0;
    
    printf("  input  file        = %s\n" , p_src_fname);
    printf("  output file        = %s\n" , p_dst_fname);
    
    in_is_nblic = suffix_match(p_src_fname, ".nblic");
    out_is_bmp  = suffix_match(p_dst_fname, ".bmp");
    
    if (!in_is_nblic) {         // src file name not ends with .nblic, compress
        
        if     ( loadPGMImageFile    (p_src_fname, img, &height, &width) ) {
            if ( loadBMPGrayImageFile(p_src_fname, img, &height, &width) ) {
                printf("  ***Error : open %s failed\n", p_src_fname);
                return -1;
            }
            in_is_bmp = 1;
        }
        
        printf("  input image format = %s\n"      , in_is_bmp?"BMP":"PGM");
        printf("  input image shape  = %d x %d\n" , width , height );
        printf("  input image size   = %d B\n"    , width * height );
        printf("  compressing ...\n");
        
        if (effort >= MAX_EFFORT) {
            int len1;
            
            COPY_ARRAY(img_tmp, img, height*width);
            effort = MIN_EFFORT;
            len1 = NBLICcodec(0, buf, img_tmp, &height, &width, &near, &effort);
            
            COPY_ARRAY(img_tmp, img, height*width);
            effort = MAX_EFFORT;
            len  = NBLICcodec(0, buf, img_tmp, &height, &width, &near, &effort);
            
            if (len1 <= len) {
                effort = MIN_EFFORT;
                len  = NBLICcodec(0, buf, img, &height, &width, &near, &effort);
                printf("  Info : use effort=%d, since it's better than effort=%d\n", MIN_EFFORT, MAX_EFFORT);
            }
        } else {
            len = NBLICcodec(0, buf, img, &height, &width, &near, &effort);
        }
        
        if (len < 0) {
            printf("  ***Error : compress failed\n");
            return -1;
        }
        
        printf("  effort             = %d (%s)\n" , effort , (effort<=MIN_EFFORT)?"fast":"deepest");
        printf("  near               = %d (%s)\n" , near   , (near  <=0)?"lossless":"lossy"  );
        printf("  output size        = %d B\n" , len    );
        printf("  compression rate   = %.5f\n" , (1.0*width*height)/len );
        printf("  compression bpp    = %.5f\n" , (8.0*len)/(width*height) );
        
        if ( writeBytesToFile(p_dst_fname, buf, len) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
        
    } else {                    // src file name ends with .nblic, decompress
        
        len = loadBytesFromFile(p_src_fname, buf, BUF_MAX_LEN);

        if (len < 0) {
            printf("  ***Error : open %s failed\n", p_src_fname);
            return -1;
        }
        
        printf("  input size         = %d B\n" , len );
        printf("  decompressing ...\n");
        
        if ( NBLICcodec(1, buf, img, &height, &width, &near, &effort) < 0 ) {
            printf("  ***Error : decompress failed\n");
            return -1;
        }
        
        printf("  effort             = %d (%s)\n" , effort , (effort<=MIN_EFFORT)?"fast":"deepest");
        printf("  near               = %d (%s)\n" , near   , (near  <=0)?"lossless":"lossy"  );
        printf("  output image shape = %d x %d\n" , width , height );
        printf("  output image format= %s\n"      , out_is_bmp?"BMP":"PGM");
        
        if (out_is_bmp) {
            if ( writeBMPGrayImageFile(p_dst_fname, img, height, width) ) {
                printf("  ***Error : write %s failed\n", p_dst_fname);
                return -1;
            }
        } else {
            if ( writePGMImageFile(p_dst_fname, img, height, width) ) {
                printf("  ***Error : write %s failed\n", p_dst_fname);
                return -1;
            }
        }
    }

    return 0;
}
