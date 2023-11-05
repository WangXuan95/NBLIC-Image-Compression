
#ifndef   __FILE_IO_H__
#define   __FILE_IO_H__


// return:
//     -1             : failed
//     positive value : file length
extern int loadBytesFromFile (const char *p_filename,       unsigned char *p_buf, int len_limit);


// return:
//     -1 : failed
//      0 : success
extern int writeBytesToFile  (const char *p_filename, const unsigned char *p_buf, int len);


// return:
//     -1 : failed
//      0 : success
extern int loadPgmImageFile  (const char *p_filename,       unsigned char *p_img, int *p_height, int *p_width);


// return:
//     -1 : failed
//      0 : success
extern int writePgmImageFile (const char *p_filename, const unsigned char *p_img, int height, int width);


#endif // __FILE_IO_H__
