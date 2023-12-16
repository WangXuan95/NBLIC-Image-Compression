#include <stdio.h>

#include "FileIO.h"
#include "NBLIC.h"



#define  TO_LOWER(c)   ((((c) >= 'A') && ((c) <= 'Z')) ? ((c)+32) : (c))


// return:
//     1 : match
//     0 : mismatch
static int matchSuffixIgnoringCase (const char *string, const char *suffix) {
    const char *p1, *p2;
    for (p1=string; *p1; p1++);
    for (p2=suffix; *p2; p2++);
    while (TO_LOWER(*p1) == TO_LOWER(*p2)) {
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
  "|            <effort>           can be 1, 2, or 3                            |\n"
  "|            <near>             can be 0 (lossless) or 1,2,3,... (lossy)     |\n"
  "|----------------------------------------------------------------------------|\n"
  "| Decompress:                                                                |\n"
  "|   nblic_codec <input-file(.nblic)> <output-image-file>                     |\n"
  "|     where: <input-file>        can only be .nblic                          |\n"
  "|            <output-image-file> can be .pgm, .pnm, or .bmp                  |\n"
  "|----------------------------------------------------------------------------|\n"
  "\n";



// return:
//     -1 : exit with error
//      0 : exit normally
int main (int argc, char **argv) {
    static unsigned char img [NBLIC_MAX_IMG_SIZE];
    static unsigned char buf [NBLIC_MAX_IMG_SIZE * 2];
    
    int verbose     =  0;
    int height      = -1;
    int width       = -1;
    int near        =  0;
    int effort      =  MIN_EFFORT;
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
    
    if (argc >= 6)
        if (argv[5][0] == 'v')
            verbose = 1;
    
    printf("  input  file        = %s\n" , p_src_fname);
    printf("  output file        = %s\n" , p_dst_fname);
    
    in_is_nblic = matchSuffixIgnoringCase(p_src_fname, ".nblic");
    out_is_bmp  = matchSuffixIgnoringCase(p_dst_fname, ".bmp");
    
    if (!in_is_nblic) {         // src file name not ends with .nblic, compress
        
        if     ( loadPGMImageFile    (p_src_fname, img, &height, &width) ) {
            if ( loadBMPGrayImageFile(p_src_fname, img, &height, &width) ) {
                printf("  ***Error : open %s failed\n", p_src_fname);
                return -1;
            }
            in_is_bmp = 1;
        }
        
        printf("  input image format = %s\n"      , in_is_bmp?"BMP":"PGM");
        printf("  input image shape  = %d x %d\n" , width, height );
        
        printf("\r    encoding ...");
        len = NBLICcompress(verbose, buf, img, height, width, &near, &effort);
        printf("\r                                                                        \r");
        
        if (len < 0) {
            printf("  ***Error : compress failed\n");
            return -1;
        }
        
        printf("  effort             = %d\n"      , effort);
        printf("  near               = %d (%s)\n" , near, (near<=0)?"lossless":"lossy");
        printf("  output size        = %d B\n"    , len   );
        printf("  compression rate   = %.5f\n"    , (1.0*width*height)/len );
        printf("  compression bpp    = %.5f\n"    , (8.0*len)/(width*height) );
        
        if ( writeBytesToFile(p_dst_fname, buf, len) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
        
    } else {                    // src file name ends with .nblic, decompress
        
        len = loadBytesFromFile(p_src_fname, buf, NBLIC_MAX_IMG_SIZE*2);

        if (len < 0) {
            printf("  ***Error : open %s failed\n", p_src_fname);
            return -1;
        }
        
        printf("  input size         = %d B\n" , len );
        
        printf("\r    decoding ...");
        len = NBLICdecompress(verbose, buf, img, &height, &width, &near, &effort);
        printf("\r                                                                        \r");
        
        if (len < 0) {
            printf("  ***Error : decompress failed\n");
            return -1;
        }
        
        printf("  effort             = %d\n"      , effort);
        printf("  near               = %d (%s)\n" , near, (near  <=0)?"lossless":"lossy");
        printf("  output image format= %s\n"      , out_is_bmp?"BMP":"PGM");
        printf("  output image shape = %d x %d\n" , width, height );
        
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
