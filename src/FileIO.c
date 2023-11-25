
#include "FileIO.h"

#include <stdio.h>



static void writeLittleEndian (int value, int len, FILE *fp) {
    for (; len>0; len--) {
        fputc((value&0xFF), fp);
        value >>= 8;
    }
}



static int loadLittleEndian (int len, FILE *fp) {
    int i, value=0;
    for (i=0; i<len; i++)
        value |= (fgetc(fp) << (8*i));
    return value;
}



// return:
//     -1             : failed
//     positive value : file length
int loadBytesFromFile (const char *p_filename, unsigned char *p_buf, int len_limit) {
    FILE *fp;
    int   len;
    
    if ( (fp = fopen(p_filename, "rb")) == NULL )
        return -1;
    
    len = fread(p_buf, sizeof(unsigned char), len_limit, fp);
    
    if (fgetc(fp) != EOF) {             // there are still some data in the file
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    
    return (len > len_limit) ? -1 : len;
}



// return:
//     -1 : failed
//      0 : success
int writeBytesToFile (const char *p_filename, const unsigned char *p_buf, int len) {
    FILE *fp;
    int   len_actual;
    
    if ( (fp = fopen(p_filename, "wb")) == NULL )
        return -1;
    
    len_actual = fwrite(p_buf, sizeof(unsigned char), len, fp);
    
    fclose(fp);
    
    return (len != len_actual) ? -1 : 0;
}



// return:
//     -1 : failed
//      0 : success
int loadPGMImageFile (const char *p_filename, unsigned char *p_img, int *p_height, int *p_width) {
    FILE *fp;
    int   len, len_actual, maxval=0;

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
    
    len = ((*p_width)*(*p_height));
    
    len_actual = fread(p_img, sizeof(unsigned char), len, fp);
    
    fclose(fp);
    
    return (len != len_actual) ? -1 : 0;
}



// return:
//     -1 : failed
//      0 : success
int writePGMImageFile (const char *p_filename, const unsigned char *p_img, int height, int width) {
    FILE *fp;
    int   len = width*height;
    int   len_actual;
    
    if (width < 1 || height < 1)
        return -1;
    
    if ( (fp = fopen(p_filename, "wb")) == NULL )
        return -1;
    
    fprintf(fp, "P5\n%d %d\n255\n", width, height);
    
    len_actual = fwrite(p_img, sizeof(unsigned char), len, fp);
    
    fclose(fp);
    
    return (len != len_actual) ? -1 : 0;
}



#define   BMP_ROW_ALIGN   4



// return:
//     -1 : failed
//      0 : success
int loadBMPGrayImageFile (const char *p_filename, unsigned char *p_img, int *p_height, int *p_width) {
    int   bm, offset, color_plane, bpp, cmprs_method, align_skip, i;
    FILE *fp;
    
    if ( (fp = fopen(p_filename, "rb")) == NULL )
        return -1;
    
    // load 14B BMP file header -----------------------------------------------------------------------
    bm          = loadLittleEndian(2, fp);      // 'BM'
                  loadLittleEndian(8, fp);      // whole file size + reserved
    offset      = loadLittleEndian(4, fp);      // start position of pixel data
    
    // load first 20B of DIB header -------------------------------------------------------------------
                  loadLittleEndian(4, fp);      // DIB header size
    (*p_width)  = loadLittleEndian(4, fp);      // width
    (*p_height) = loadLittleEndian(4, fp);      // height
    color_plane = loadLittleEndian(2, fp);      // color plane
    bpp         = loadLittleEndian(2, fp);      // bits per pixel
    cmprs_method= loadLittleEndian(4, fp);      // compress method
    
    if (bm != 0x4D42 || color_plane != 1 || bpp != 8 || cmprs_method != 0 || (*p_width) < 1 || (*p_height) < 1) {
        fclose(fp);
        return -1;
    }
    
    offset -= 34;             // we've read 34B
    
    if (offset < 0) {
        fclose(fp);
        return -1;
    }
    
    // skip to the start of pixel data ----------------------------------------------------------------
    for (; offset>0; offset--) {
        if (fgetc(fp) == EOF) {
            fclose(fp);
            return -1;
        }
    }
    
    //printf("%08x  %08x  %08x  %08x  %08x  %08x  %08x\n", bm, offset, (*p_width), (*p_height), color_plane, bpp, cmprs_method);
    
    align_skip = (((*p_width) + BMP_ROW_ALIGN - 1) / BMP_ROW_ALIGN) * BMP_ROW_ALIGN - (*p_width);
    
    // load pixel data, note that the scan order of BMP is from down to up, from left to right --------
    for (i=(*p_height)-1; i>=0; i--) {
        unsigned char *p_row = p_img + (i * (*p_width));
        if ((*p_width) != (int)fread(p_row, sizeof(unsigned char), (*p_width), fp)) {
            fclose(fp);
            return -1;
        }
        loadLittleEndian(align_skip, fp);
    }
    
    fclose(fp);
    return 0;
}



// return:
//     -1 : failed
//      0 : success
int writeBMPGrayImageFile (const char *p_filename, const unsigned char *p_img, int height, int width) {
    const int align_width = ((width + BMP_ROW_ALIGN - 1) / BMP_ROW_ALIGN) * BMP_ROW_ALIGN;
    const int align_skip  = align_width - width;
    const int file_size = 14 + 40 + 1024 + height * align_width;  // 14B BMP file header + 40B DIB header + 1024B palette + pixels
    int   i;
    FILE *fp;
    
    if (width < 1 || height < 1)
        return -1;
    
    if ( (fp = fopen(p_filename, "wb")) == NULL )
        return -1;
    
    // write 14B BMP file header -----------------------------------------------------------------------
    writeLittleEndian(    0x4D42, 2, fp);   // 'BM'
    writeLittleEndian( file_size, 4, fp);   // whole file size
    writeLittleEndian(0x00000000, 4, fp);   // reserved
    writeLittleEndian(0x00000436, 4, fp);   // start position of pixel data
    
    // write 40B DIB header ----------------------------------------------------------------------------
    writeLittleEndian(0x00000028, 4, fp);   // DIB header size
    writeLittleEndian(     width, 4, fp);   // width
    writeLittleEndian(    height, 4, fp);   // height
    writeLittleEndian(    0x0001, 2, fp);   // one color plane
    writeLittleEndian(    0x0008, 2, fp);   // 8 bits per pixel
    writeLittleEndian(0x00000000, 4, fp);   // BI_RGB
    writeLittleEndian(0x00000000, 4, fp);   // pixel data size (height * width), a dummy 0 can be given for BI_RGB bitmaps
    writeLittleEndian(0x00000EC4, 4, fp);   // horizontal resolution of the image. (pixel per metre, signed integer)
    writeLittleEndian(0x00000EC4, 4, fp);   // vertical resolution of the image. (pixel per metre, signed integer)
    writeLittleEndian(0x00000100, 4, fp);   // number of colors in the color palette, or 0 to default to 2^n
    writeLittleEndian(0x00000000, 4, fp);   // number of important colors used, or 0 when every color is important; generally ignored
    
    // write palette -----------------------------------------------------------------------------------
    for (i=0; i<256; i++) {
        fputc(i, fp);
        fputc(i, fp);
        fputc(i, fp);
        fputc(0xFF, fp);
    }
    
    // write pixel data, note that the scan order of BMP is from down to up, from left to right --------
    for (i=height-1; i>=0; i--) {
        const unsigned char *p_row = p_img + (i * width);
        fwrite(p_row, sizeof(unsigned char), width, fp);
        writeLittleEndian(0x00000000, align_skip, fp);
    }
    
    if (ftell(fp) < file_size) {
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}
