
#include "FileIO.h"

#include <stdio.h>



// return:
//     -1             : failed
//     positive value : file length
int loadBytesFromFile (const char *p_filename, unsigned char *p_buf, int len_limit) {
    unsigned char *p_base  = p_buf;
    unsigned char *p_limit = p_buf + len_limit;
    FILE *fp;
    int   ch;
    
    if ( (fp = fopen(p_filename, "rb")) == NULL )
        return -1;
    
    while ((ch = fgetc(fp)) != EOF) {
        if (p_buf >= p_limit)
            return -1;
        *(p_buf++) = (unsigned char)ch;
    }

    return p_buf - p_base;
}



// return:
//     -1 : failed
//      0 : success
int writeBytesToFile (const char *p_filename, const unsigned char *p_buf, int len) {
    const unsigned char *p_limit = p_buf + len;
    FILE *fp;
    
    if ( (fp = fopen(p_filename, "wb")) == NULL )
        return -1;

    for (; p_buf<p_limit; p_buf++) {
        if (fputc(*p_buf, fp) == EOF) {
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}



// return:
//     -1 : failed
//      0 : success
int loadPgmImageFile (const char *p_filename, unsigned char *p_img, int *p_height, int *p_width) {
    int   i, maxval=0;
    FILE *fp;

    (*p_height) = (*p_width) = -1;
    
    if ( (fp = fopen(p_filename, "rb")) == NULL )
        return -1;

    if ( fgetc(fp) != 'P' ) {
        fclose(fp);
        return -1;
    }
    
    if ( fgetc(fp) != '5' ) {
        fclose(fp);
        return -1;
    }

    if ( fscanf(fp, "%d", p_width) < 1 ) {
        fclose(fp);
        return -1;
    }
    
    if ( fscanf(fp, "%d", p_height) < 1 ) {
        fclose(fp);
        return -1;
    }

    if ( fscanf(fp, "%d", &maxval) < 1 ) {
        fclose(fp);
        return -1;
    }
    
    if (maxval < 1 || maxval > 255) {        // PGM pixel depth not support
        fclose(fp);
        return -1;
    }
    
    if ((*p_width) < 1 || (*p_height) < 1) { // PGM size error
        fclose(fp);
        return -1;
    }
    
    fgetc(fp);                               // skip a white char
    
    for (i=((*p_width)*(*p_height)); i>0; i--) {
        int ch = fgetc(fp);
        
        if (ch == EOF) {                     // pixels not enough
            fclose(fp);
            return -1;
        }
        
        *(p_img++) = (unsigned char)ch;
    }

    fclose(fp);
    return 0;
}



// return:
//     -1 : failed
//      0 : success
int writePgmImageFile (const char *p_filename, const unsigned char *p_img, int height, int width) {
    int   i;
    FILE *fp;
    
    if (width < 1 || height < 1)
        return -1;
    
    if ( (fp = fopen(p_filename, "wb")) == NULL )
        return -1;

    fprintf(fp, "P5\n%d %d\n255\n", width, height);
    
    for (i=width*height; i>0; i--) {
        unsigned char pixel = *(p_img++);
        if (fputc(pixel, fp) == EOF) {
            fclose(fp);                       // write file failed
            return -1;
        }
    }

    fclose(fp);
    return 0;
}
