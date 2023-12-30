
#ifndef   __QNBLIC_H__
#define   __QNBLIC_H__


#include <stdint.h>


#define    QNBLIC_MAX_HEIGHT    65535
#define    QNBLIC_MAX_WIDTH     65535
#define    QNBLIC_MAX_IMG_SIZE  100000000


extern int QNBLICcompress   (uint16_t *p_buf, unsigned char *p_img, int height, int width);


extern int QNBLICdecompress (uint16_t *p_buf, unsigned char *p_img, int *p_height, int *p_width);


#endif // __QNBLIC_H__
