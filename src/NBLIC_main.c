#include <stdio.h>

#include "FileIO.h"
#include "NBLIC.h"     // NBLIC effort  =0
#include "QNBLIC.h"    // NBLIC effort >=1



const char *USAGE = 
  "|----------------------------------------------------------------------------|\n"
  "| NBLIC: a lossless & near-lossless gray 8-bit image compressor (v0.3)       |\n"
  "|   Copyright (C) 2023 Xuan Wang.                                            |\n"
  "|   source from https://github.com/WangXuan95/NBLIC-Image-Compression        |\n"
  "|                                                                            |\n"
  "|----------------------------------------------------------------------------|\n"
  "| To compress:                                                               |\n"
  "|   nblic_codec -c [-swiches] <input-image-file> <output-file(.nblic)>       |\n"
  "|     where:                                                                 |\n"
  "|            <input-image-file> can be .pgm, .pnm, or .bmp                   |\n"
  "|                               and must be gray 8-bit image                 |\n"
  "|            <output-file>      can only be .nblic                           |\n"
  "|     swiches:                                                               |\n"
  "|            -n<number> : near, can be 0 (lossless) or 1,2,3,... (lossy)     |\n"
  "|            -e<number> : effort, can be 0 (fastest), 1, 2, or 3 (slowest)   |\n"
  "|                         note: when using lossy(near>0), effort cannot be 0 |\n"
  "|            -v : verbose, print infomations                                 |\n"
  "|            -V : verbose, print infomations and progress                    |\n"
  "|            -t : multithread speedup, currently only support -e0 on Windows |\n"
  "|                                                                            |\n"
  "| compression examples :                                                     |\n"
  "|   fastest lossless:    ./nblic_codec -c -V -n0 -e0 in.bmp out.nblic        |\n"
  "|   slowest lossless:    ./nblic_codec -c -V -n0 -e3 in.bmp out.nblic        |\n"
  "|   slow lossy(near=2):  ./nblic_codec -c -V -n2 -e2 in.bmp out.nblic        |\n"
  "|                                                                            |\n"
  "|----------------------------------------------------------------------------|\n"
  "| To decompress:                                                             |\n"
  "|   nblic_codec -d [-swiches] <input-file(.nblic)> <output-image-file>       |\n"
  "|     where:                                                                 |\n"
  "|            <input-file>        can only be .nblic                          |\n"
  "|            <output-image-file> can be .pgm, .pnm, or .bmp                  |\n"
  "|     swiches:                                                               |\n"
  "|            -v : verbose, print infomations                                 |\n"
  "|            -V : verbose, print infomations and progress                    |\n"
  "|                                                                            |\n"
  "| decompression example :   ./nblic_codec -d -V in.nblic out.bmp             |\n"
  "|                                                                            |\n"
  "|----------------------------------------------------------------------------|\n"
  "\n";



static void parseSwitches (char *arg, int *p_d, int *p_n, int *p_e, int *p_v, int *p_t) {
    for (; arg[0]; arg++) {
        switch (arg[0]) {
            case 'c' :
            case 'C' :
                *p_d = 0;  // compress
                break;
            
            case 'd' :
            case 'D' :
                *p_d = 1;  // decompress
                break;
            
            case 'v' :
                *p_v = 1;
                break;
            
            case 'V' :
                *p_v = 2;
                break;
            
            case 'n' :
            case 'N' :
                (*p_n) = 0;
                for (; ('0'<=arg[1] && arg[1]<='9'); arg++) {
                    (*p_n) *= 10;
                    (*p_n) += (arg[1] - '0');
                }
                break;
            
            case 'e' :
            case 'E' :
                if ('0'<=arg[1] && arg[1]<='9')
                    (*p_e) = (arg[1] - '0');
                arg ++;
                break;
            
            case 't' :
            case 'T' :
                *p_t = 1;  // enable multithread
                break;
        }
    }
}


static void parseCommand (int argc, char **argv, char **pp_src_fname, char **pp_dst_fname, int *p_d, int *p_n, int *p_e, int *p_v, int *p_t) {
    int i;
    
    for (i=1; i<argc; i++) {
        char *arg = argv[i];
        
        if      (arg[0] == '-')
            parseSwitches(&arg[1], p_d, p_n, p_e, p_v, p_t);
        else if (*pp_src_fname == NULL)
            *pp_src_fname = arg;
        else
            *pp_dst_fname = arg;
    }
}


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



// return:
//     -1 : exit with error
//      0 : exit normally
int main (int argc, char **argv) {
    static unsigned char img [NBLIC_MAX_IMG_SIZE];
    static uint16_t      buf [NBLIC_MAX_IMG_SIZE];

    char *p_src_fname=NULL, *p_dst_fname=NULL;
    
    int decompress = 0;
    int near       = 0;
    int effort     = 1;
    int verbose    = 0;
    int multithread= 0;
    int height     =-1;
    int width      =-1;
    int len        =-1;
    int is_bmp     =0;
    
    parseCommand(argc, argv, &p_src_fname, &p_dst_fname, &decompress, &near, &effort, &verbose, &multithread);
    
    if (p_src_fname==NULL || p_dst_fname==NULL) {
        printf(USAGE);
        return -1;
    }
    
    if (verbose) {
        printf("  input  file        = %s\n" , p_src_fname);
        printf("  output file        = %s\n" , p_dst_fname);
    }
    
    if (!decompress) { // compress ---------------------------------------
        if     ( loadPGMImageFile    (p_src_fname, img, &height, &width) ) {
            if ( loadBMPGrayImageFile(p_src_fname, img, &height, &width) ) {
                printf("  ***Error : open %s failed\n", p_src_fname);
                printf("             please specific a gray 8-bit PGM or BMP file as input\n");
                return -1;
            }
            is_bmp = 1;
        }
        
        if (verbose) {
            printf("  input image format = %s\n"      , is_bmp?"BMP":"PGM");
            printf("  input image shape  = %d x %d\n" , width, height );
        }
        
        if (near==0 && effort==0) {
            if (multithread)
                len = 2 * QNBLICcompressMultiThread(buf, img, height, width);
            else
                len = 2 * QNBLICcompress(buf, img, height, width);
        } else {
            len = NBLICcompress((verbose>1), (unsigned char*)buf, img, height, width, &near, &effort);
        }
        
        if (len < 0) {
            printf("  ***Error : compress failed\n");
            return -1;
        }
        
        if (verbose) {
            printf("  effort             = %d\n"      , effort);
            printf("  near               = %d (%s)\n" , near, (near<=0)?"lossless":"lossy");
            printf("  output size        = %d B\n"    , len   );
            printf("  compression rate   = %.5f\n"    , (1.0*width*height)/len );
            printf("  compression bpp    = %.5f\n"    , (8.0*len)/(width*height) );
        }
        
        if ( writeBytesToFile(p_dst_fname, (unsigned char*)buf, len) ) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
        
    } else { // decompress ---------------------------------------
        len = loadBytesFromFile(p_src_fname, (unsigned char*)buf, sizeof(buf));

        if (len < 0) {
            printf("  ***Error : open %s failed\n", p_src_fname);
            return -1;
        }
        
        if (verbose)
            printf("  input size         = %d B\n" , len );
        
        near = 0;
        effort = 0;
        
        len = QNBLICdecompress(buf, img, &height, &width);
        
        if (len < 0)
            len = NBLICdecompress((verbose>1), (unsigned char*)buf, img, &height, &width, &near, &effort);
        
        if (len < 0) {
            printf("  ***Error : decompress failed\n");
            return -1;
        }
        
        is_bmp = matchSuffixIgnoringCase(p_dst_fname, ".bmp");
        
        if (verbose) {
            printf("  effort             = %d\n"      , effort);
            printf("  near               = %d (%s)\n" , near, (near  <=0)?"lossless":"lossy");
            printf("  output image format= %s\n"      , is_bmp?"BMP":"PGM");
            printf("  output image shape = %d x %d\n" , width, height );
        }
        
        if (is_bmp)
            len = writeBMPGrayImageFile(p_dst_fname, img, height, width);
        else
            len = writePGMImageFile(p_dst_fname, img, height, width);
        
        if (len) {
            printf("  ***Error : write %s failed\n", p_dst_fname);
            return -1;
        }
    }

    return 0;
}
