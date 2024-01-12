# -*- coding: utf-8 -*-
# Python3

# python standard libraries importation
import sys
import os
import time

# python third-party libraries importation (additional installation is required, if you do not have)
import numpy as np
from PIL import Image      # run "python -m pip install --upgrade Pillow" to install



############################################### user config #################
TMP_RAW1_FILE  = 'tmp1.bmp'                                                 #
TMP_CMPRS_FILE = 'tmp.nblic'                                                #
TMP_RAW2_FILE  = 'tmp2.bmp'                                                 #
                                                                            #
CODEC_EXE_FILE = '.\\nblic_codec.exe'          # only for windows           #
CODEC_BIN_FILE = './nblic_codec'               # only for linux             #
############################################### end of user config ##########



def callCodec (input_fname, output_fname, decompress, near=0, windows=1) :
    effort = 0
    
    mode = '-d' if (decompress) else '-c'
    
    if windows :
        command =          CODEC_EXE_FILE + ' ' + mode + ' -t ' + '-e'+str(effort) + ' ' + '-n'+str(near) + ' ' + input_fname + ' ' + output_fname
    else : # linux (WSL, windows subsystem for linux)
        command = 'wsl ' + CODEC_BIN_FILE + ' ' + mode + ' -t ' + '-e'+str(effort) + ' ' + '-n'+str(near) + ' ' + input_fname + ' ' + output_fname
    
    return os.system(command)



def loadImageAsGrayOrRGB (input_image_file_name) :
    img_obj = Image.open(input_image_file_name)
    if img_obj.mode == 'L' :
        img = img_obj.convert('L')
    else :
        img = img_obj.convert('RGB')
    img_obj.close()
    return img



def convertImage (input_image_file_name, output_file_name) :
    img = loadImageAsGrayOrRGB(input_image_file_name)
    img.save(output_file_name)
    return img.width * img.height



def check2Images (image_file_name1, image_file_name2, max_abs_err_tolerance) :
    img1 = loadImageAsGrayOrRGB(image_file_name1)
    img2 = loadImageAsGrayOrRGB(image_file_name2)
    
    img1 = np.asarray(img1)
    img2 = np.asarray(img2)
    
    assert img1.shape == img2.shape, 'different shape :  img1.shape=%s  img2.shape=%s' % (str(img1.shape), str(img2.shape))
    
    img1 = img1.reshape([-1])
    img2 = img2.reshape([-1])
    
    max_abs_err = np.max(np.abs(np.int32(img1) - np.int32(img2)))
    
    assert max_abs_err <= max_abs_err_tolerance, 'max_abs_err=%d, max_abs_err_tolerance=%d' % (max_abs_err, max_abs_err_tolerance)



if __name__ == '__main__' :
    
    # parse command line args -------------------------------------------------------------
    if '--no-check' in sys.argv :
        sys.argv.remove('--no-check')
        check = False
    else :
        check = True
    
    try :
        in_dir    = sys.argv[1]
        near_list = [int(near) for near in sys.argv[2:]]
    except :
        print('Usage    : python %s <input_dir_name> [<near_list>] [--no-check]' % sys.argv[0])
        print('Example1 : python %s ./images 0 1 2'                              % sys.argv[0])
        print('Example2 : python %s ./images 1 3 --no-check'                     % sys.argv[0])
        exit(-1)
    
    if len(near_list) <= 0 :
        near_list = [0]
    
    
    file_count       = 0
    total_n_pixel    = 0
    total_cmprs_size = [0] * len(near_list)
    
    
    time_consume = time.time()
    
    for fname in os.listdir(in_dir) :
        fname_full = in_dir + os.path.sep + fname
        
        try :
            n_pixel = convertImage(fname_full, TMP_RAW1_FILE)
        except :
            print('skip %s' % fname)
            continue
        
        file_count    += 1
        total_n_pixel += n_pixel
        
        print('BPP of %s:' % fname , end='' , flush=True )
        
        for (i, near) in enumerate(near_list) :
            if 0 != callCodec(TMP_RAW1_FILE, TMP_CMPRS_FILE, 0, near) :
                print('\n*** %s encode error' % fname)
                exit(-1)
            
            if check :
                if 0 != callCodec(TMP_CMPRS_FILE, TMP_RAW2_FILE, 1) :
                    print('\n*** %s decode error' % fname)
                    exit(-1)
                
                check2Images(TMP_RAW1_FILE, TMP_RAW2_FILE, near)
            
            size = os.path.getsize(TMP_CMPRS_FILE)
            
            total_cmprs_size[i] += size
            
            bpp = 8.0 * size / n_pixel
            
            print('     %.3f (near=%d)' % (bpp, near) , end='' , flush=True )
        
        print()
        
        os.remove(TMP_RAW1_FILE)
        os.remove(TMP_CMPRS_FILE)
        if check :
            os.remove(TMP_RAW2_FILE)
    
    time_consume = time.time() - time_consume
    
    print('BPP of all %d files:' % file_count , end='' , flush=True )
    
    for near, size in zip(near_list, total_cmprs_size) :
        bpp = 8.0 * size / total_n_pixel
        print('     %.3f (near=%d)' % (bpp, near) , end='' , flush=True )
    
    print('\ntime = %f s' % time_consume)

