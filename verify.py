# -*- coding: utf-8 -*-
# Python3

# python standard libraries importation
import sys
import os
import subprocess as sp

# python third-party libraries importation (additional installation is required, if you do not have)
import numpy as np
from PIL import Image      # run "python -m pip install --upgrade Pillow" to install



TMP_PGM_FILE1 = 'tmp1.pgm'
TMP_JLSx_FILE = 'tmp.jlsx'
TMP_PGM_FILE2 = 'tmp2.pgm'

#NEAR_LIST = [0]
NEAR_LIST = [0,1,2]



#JLSx_BIN = './JLSx'               # for linux
JLSx_BIN = '.\\JLSx.exe'          # for windows



def callJLSx (pgm_file_name, jlsx_file_name, mode='encode', near=0) :
    if mode == 'encode' :
        COMMANDS = [ JLSx_BIN, pgm_file_name, jlsx_file_name, str(near) ]
    else :
        COMMANDS = [ JLSx_BIN, jlsx_file_name, pgm_file_name ]
    p = sp.Popen(COMMANDS, stdin=sp.PIPE, stdout=sp.PIPE, stderr=sp.PIPE)
    assert p.wait() == 0



def convertImageToPGM (image_file_name, pgm_file_name) :
    img_obj = Image.open(image_file_name)
    img = img_obj.convert('L')
    img_obj.close()
    img.save(pgm_file_name)



def check2Images (image1_file_name, image2_file_name, max_abs_err_tolerance) :
    img_obj = Image.open(image1_file_name)
    img = img_obj.convert('L')
    img_obj.close()
    img1 = np.asarray(img)
    
    img_obj = Image.open(image2_file_name)
    img = img_obj.convert('L')
    img_obj.close()
    img2 = np.asarray(img)
    
    h1, w1 = img1.shape
    h2, w2 = img2.shape
    
    assert h1 == h2, 'different height: %d %d' % (h1, h2)
    assert w1 == w2, 'different width : %d %d' % (w1, w2)
    
    max_abs_err = np.max(np.abs(np.int32(img1) - np.int32(img2)))
    
    assert max_abs_err <= max_abs_err_tolerance, 'max_abs_err=%d, max_abs_err_tolerance=%d' % (max_abs_err, max_abs_err_tolerance)





if __name__ == '__main__' :
    
    try :
        in_dir = sys.argv[1]
    except :
        print('Usage: python %s <input_dir_name>' % sys.argv[0])
        exit(-1)
    
    
    origin_total_size = 0
    jlsx_total_size   = [0] * len(NEAR_LIST)
    
    
    for fname in os.listdir(in_dir) :
        fname_full = in_dir + os.path.sep + fname
        
        try :
            convertImageToPGM(fname_full, TMP_PGM_FILE1)
        except :
            print('skip %s' % fname)
            continue
        
        print('%s ----------------------------------------------' % fname )
        
        origin_total_size += os.path.getsize(TMP_PGM_FILE1)
        
        for inear, near in enumerate(NEAR_LIST) :
            try :
                callJLSx(TMP_PGM_FILE1, TMP_JLSx_FILE, mode='encode', near=near)
            except AssertionError as e:
                print('*** %s JLS encode error' % fname)
                exit(-1)
            
            try :
                callJLSx(TMP_PGM_FILE2, TMP_JLSx_FILE, mode='decode')
            except AssertionError as e:
                print('*** %s JLS decode error' % fname)
                exit(-1)
            
            try :
                check2Images(TMP_PGM_FILE1, TMP_PGM_FILE2, near)
            except AssertionError as e:
                print('*** %s verify failed: %s' % (fname, str(e)) )
                exit(-1)
            
            jlsx_total_size[inear] += os.path.getsize(TMP_JLSx_FILE)
            
            print('near=%d   compressed_ratio=%.3f' % (near, os.path.getsize(TMP_PGM_FILE1) / os.path.getsize(TMP_JLSx_FILE) ) )
        
        print('total compression ratios:' , end='' )
        
        for near, jlsx_size in zip(NEAR_LIST, jlsx_total_size) :
            print('      near=%d  %7.4f' % (near, origin_total_size/jlsx_size) , end='' )
        print()
    
    
    os.remove(TMP_PGM_FILE1)
    os.remove(TMP_JLSx_FILE)
    os.remove(TMP_PGM_FILE2)


