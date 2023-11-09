#include <stdio.h>

#include "FileIO.h"
#include "NBLIC.h"



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


#define   IMG_MAX_LEN   (NBLIC_MAX_HEIGHT * NBLIC_MAX_WIDTH)
#define   BUF_MAX_LEN   (IMG_MAX_LEN * 2)


// return:
//     -1 : exit with error
//      0 : exit normally
int main (int argc, char **argv) {
    static unsigned char img [IMG_MAX_LEN];
    static unsigned char buf [BUF_MAX_LEN];

    int height = -1;
    int width  = -1;
    int effort = -1;
    int near   = -1;
    int len    = -1;

    const char *p_src_fname=NULL, *p_dst_fname=NULL;
    
    if (argc < 3) {                                      // illegal arguments: print USAGE and exit
        printf("-----------------------------------------------------------------------------------------------------\n");
        printf("| NBLIC: a lossless & near-lossless image compressor\n");
        printf("| \n");
        printf("| Usage:\n");
        printf("|     Compress:\n");
        printf("|         %s  <input-image-file(.pgm)>  <output-file(.nblic)>  [<effort>]  [<near>]\n" , argv[0] );
        printf("|           where:   <effort> can be 1 (fast) or 2 (deepest)\n");
        printf("|                    <near>   can be 0 (lossless) or 1,2,3,... (lossy)\n");
        printf("|     Decompress:\n");
        printf("|         %s  <input-file(.nblic)>  <output-image-file(.pgm)>\n" , argv[0] );
        printf("-----------------------------------------------------------------------------------------------------\n");
        return -1;
    }
    
    p_src_fname = argv[1];
    p_dst_fname = argv[2];
    
    if (argc >= 4)
        if ( sscanf(argv[3], "%d", &effort) <= 0 )
            effort = 1;
    
    if (argc >= 5)
        if ( sscanf(argv[4], "%d", &near) <= 0 )
            near = 0;
    
    printf("  input  file        = %s\n" , p_src_fname);
    printf("  output file        = %s\n" , p_dst_fname);
    
    if ( !suffix_match(p_src_fname, ".nblic") ) {         // src file name not ends with .nblic, compress
        
        if ( loadPgmImageFile(p_src_fname, img, &height, &width) ) {
            printf("  ***Error : open %s failed\n", p_src_fname);
            return -1;
        }
        
        printf("  input image shape  = %d x %d\n" , width , height );
        printf("  input image size   = %d B\n"    , width * height );
        printf("  compressing ...\n");
        
        len = NBLICcodec(0, buf, img, &height, &width, &near, &effort);
        
        if (len < 0) {
            printf("  ***Error : compress failed\n");
            return -1;
        }
        
        printf("  effort             = %d (%s)\n" , effort , (effort<=1)?"fast"    :"deepest");
        printf("  near               = %d (%s)\n" , near   , (near  <=0)?"lossless":"lossy"  );
        printf("  output size        = %d B\n" , len    );
        printf("  compression rate   = %.5f\n" , (1.0*width*height)/len );
        printf("  compression bpp    = %.5f\n" , (8.0*len)/(width*height) );
        
        if ( writeBytesToFile(p_dst_fname, buf, len) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
        
    } else {                                            // src file name ends with .nblic, decompress
        
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
        
        printf("  effort             = %d (%s)\n" , effort , (effort<=1)?"fast"    :"deepest");
        printf("  near               = %d (%s)\n" , near   , (near  <=0)?"lossless":"lossy"  );
        printf("  output image shape = %d x %d\n" , width , height );
        
        if ( writePgmImageFile(p_dst_fname, img, height, width) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
    }

    return 0;
}
