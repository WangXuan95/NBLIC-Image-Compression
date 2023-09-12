// Copyright https://github.com/WangXuan95
// source: https://github.com/WangXuan95/JPEG-LS_extension
// 
// A enhanced implementation of JPEG-LS extension (ITU-T T.870) image encoder/decoder
// which will get a higher compression ratio than original JPEG-LS extension
// see https://www.itu.int/rec/T-REC-T.870/en
// Warning: This implementation is not compliant with ITU-T T.870 standard.
//
// This is an example, including a main() function
//


#include <stdio.h>

#include "JLSx.h"



// return:   -1:failed   0:success
int loadPGMfile (const char *filename, unsigned char *img, int *pysz, int *pxsz) {
    int i, maxval=-1;
    FILE *fp;

    *pysz = -1;
    *pxsz = -1;
    
    if ( (fp = fopen(filename, "rb")) == NULL )
        return -1;

    if ( fgetc(fp) != 'P' ) {
        fclose(fp);
        return -1;
    }
    
    if ( fgetc(fp) != '5' ) {
        fclose(fp);
        return -1;
    }

    if ( fscanf(fp, "%d", pxsz) < 1 ) {
        fclose(fp);
        return -1;
    }
    
    if ( fscanf(fp, "%d", pysz) < 1 ) {
        fclose(fp);
        return -1;
    }

    if ( fscanf(fp, "%d", &maxval) < 1 ) {
        fclose(fp);
        return -1;
    }
    
    if ((*pxsz) < 1 || (*pysz) < 1 || maxval < 1 || maxval > 255) {    // PGM format error or not support
        fclose(fp);
        return -1;
    }
    
    fgetc(fp);                                                         // skip a white char
    
    for (i=0; i<(*pxsz)*(*pysz); i++) {
        if (feof(fp)) {                                                // pixels not enough
            fclose(fp);
            return -1;
        }
        
        *(img++) = (fgetc(fp) & 0xFF);
    }

    fclose(fp);
    return 0;
}



// return:   -1:failed   0:success
int writePGMfile (const char *filename, const unsigned char *img, const int ysz, const int xsz) {
    int i;
    FILE *fp;
    
    if (xsz < 1 || ysz < 1)
        return -1;
    
    if ( (fp = fopen(filename, "wb")) == NULL )
        return -1;

    fprintf(fp, "P5\n%d %d\n255\n", xsz, ysz);
    
    for (i=0; i<xsz*ysz; i++) {
        unsigned char pixel = *(img++);
        if ( fputc( pixel , fp) == EOF ) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}



// return:   -1:failed   positive:file length
int readBytesFromFile (const char *filename, unsigned char *buffer, const int len_limit) {
    const unsigned char *buffer_base    = buffer;
    const unsigned char *buffer_end_ptr = buffer + len_limit;
    
    FILE *fp;
    
    if ( (fp = fopen(filename, "rb")) == NULL )
        return -1;

    while (buffer<buffer_end_ptr) {
        if (feof(fp))
            return buffer - buffer_base;
        *(buffer++) = (unsigned char)fgetc(fp);
    }

    return -1;
}



// return:   -1:failed   0:success
int writeBytesToFile (const char *filename, const unsigned char *buffer, const int len) {
    const unsigned char *buffer_end_ptr = buffer + len;
    
    FILE *fp;
    
    if ( (fp = fopen(filename, "wb")) == NULL )
        return -1;

    for (; buffer<buffer_end_ptr; buffer++) {
        if ( fputc( *buffer , fp) == EOF ) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}




// return:  1:match   0:mismatch
int suffix_match (const char *string, const char *suffix) {
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




#define   MAX_YSZ            16384
#define   MAX_XSZ            16384
#define   JLS_LENGTH_LIMIT   (MAX_YSZ*MAX_XSZ*3)



int main (int argc, char **argv) {

    static unsigned char img        [MAX_YSZ*MAX_XSZ];
    static unsigned char imgrcon    [MAX_YSZ*MAX_XSZ];
    static unsigned char jls_buffer [JLS_LENGTH_LIMIT] = {0};

    const char *src_fname=NULL, *dst_fname=NULL;

    int ysz=-1 , xsz=-1 , near=0 , jls_length;


    printf("JPEG-LS extension ITU-T T.870 (a non-standard version)\n");
    printf("see https://github.com/WangXuan95/JPEG-LS_extension\n");
    printf("\n");


    if (argc < 3) {                                    // illegal arguments: print USAGE and exit
        printf("Usage:\n");
        printf("    Encode:\n");
        printf("        %s  <input-image-file(.pgm)>  <output-file(.jlsxn)>  [<near>]\n" , argv[0] );
        printf("    Decode:\n");
        printf("        %s  <input-file(.jlsxn)>  <output-image-file(.pgm)>\n" , argv[0] );
        printf("\n");
        printf("Note: the file suffix \".jlsxn\" means JPEG-LS eXtension Non-standard\n");
        printf("\n");
        return -1;
    }
    
    
    src_fname = argv[1];
    dst_fname = argv[2];
    
    
    if (argc >= 4) {
        if ( sscanf(argv[3], "%d", &near) <= 0 )
            near = 0;
    }


    printf("  input  file        = %s\n" , src_fname);
    printf("  output file        = %s\n" , dst_fname);


    if ( suffix_match(src_fname, ".pgm") ) {                                   // src file is a pgm, encode
        
        printf("  near               = %d\n"       , near);

        if ( loadPGMfile(src_fname, img, &ysz, &xsz) ) {
            printf("open %s failed\n", src_fname);
            return -1;
        }
    
        printf("  image shape        = %d x %d\n"  , xsz , ysz );
        printf("  original size      = %d Bytes\n" , xsz*ysz );

        jls_length = JLSxEncode(jls_buffer, img, imgrcon, ysz, xsz, near);
        
        if (jls_length < 0) {
            printf("encode failed\n");
            return -1;
        }

        printf("  compressed length  = %d Bytes\n" , jls_length );
        printf("  compression ratio  = %.5f\n"     , (1.0*xsz*ysz)/jls_length );
        printf("  compressed bpp     = %.5f\n"     , (8.0*jls_length)/(xsz*ysz) );

        if ( writeBytesToFile(dst_fname, jls_buffer, jls_length) ) {
            printf("write %s failed\n", dst_fname);
            return -1;
        }

    } else {
        
        jls_length = readBytesFromFile(src_fname, jls_buffer, JLS_LENGTH_LIMIT);

        if ( jls_length < 0 ) {
            printf("open %s failed\n", src_fname);
            return -1;
        }
        
        printf("  JLS length         = %d Bytes\n" , jls_length );

        if ( JLSxDecode(jls_buffer, img, &ysz, &xsz, &near) < 0 ) {
            printf("decode failed\n");
            return -1;
        }
    
        printf("  near               = %d\n"       , near);
        printf("  image shape        = %d x %d\n"  , xsz , ysz );
        printf("  image size         = %d Bytes\n" , xsz*ysz );
        printf("  compression ratio  = %.5f\n"     , (1.0*xsz*ysz)/jls_length );
        printf("  compressed bpp     = %.5f\n"     , (8.0*jls_length)/(xsz*ysz) );

        if ( writePGMfile(dst_fname, img, ysz, xsz) ) {
            printf("write %s failed\n", dst_fname);
            return -1;
        }
    }

    return 0;
}
