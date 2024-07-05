 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# NBLIC: New Bee Lossless Image Compression

NBLIC is a lossless & near-lossless gray 8-bit image compression algorithm with high compression ratio.

see [Here](#Comparison) and [Gray 8-bit Lossless Compression Bench](https://github.com/WangXuan95/Gray8bit-Image-Compression-Benchmark) for comparison to other image formats.

### Features

- **High compression ratio** for photos
- **Low code** (only 1000 lines of C language of encoder and decoder).
- Just single pass scan encode/decode, friendly for FPGA hardware streaming implementation.

### Development progress

*Current Version: v0.3*

- [x] 8-bit gray image lossless encode/decode is support now.
- [x] 8-bit gray image near-lossless encode/decode is support now.
- [ ] 24-bit RGB image lossless encode/decode will be supported in the future.
- [ ] 24-bit RGB image near-lossless encode/decode will be supported in the future.

　

# Code List

The code files are in pure-C, located in the [src](./src) folder:

| File Name    | Description                                                  |
| ------------ | ------------------------------------------------------------ |
| NBLIC.c      | Implement NBLIC encoder/decoder                              |
| NBLIC.h      | Expose the functions of NBLIC encoder/decoder to users.      |
| QNBLIC.c     | Implement QNBLIC (Quicker NBLIC) encoder/decoder (for -e0)   |
| QNBLIC.h     | Expose the functions of QNBLIC encoder/decoder to users.     |
| FileIO.c     | Implement BMP and PGM image file reading/writing functions and binary file reading/writing functions. |
| FileIO.h     | Expose the functions in FileIO.c to users.                   |
| NBLIC_main.c | Include `main()` function. It calls `NBLIC.h` and `FileIO.h` to achieve image file encoding/decoding. |

　

# Compile

### Compile in Linux

Run the command in the current directory:

```bash
gcc src/*.c -o nblic_codec -O3 -Wall
```

We'll get the binary file `nblic_codec` . Here I've compiled it for you, you can use it directly.

### Compile in Windows (CMD)

If you installed MinGW Compiler for Windows, you can compile using the command line (CMD).

```powershell
gcc src/*.c -o nblic_codec.exe -O3 -Wall
```

We'll get the executable file `nblic_codec.exe` . Here I've compiled it for you, you can use it directly.

> It is recommended to use the x64 compiler for compilation, as NBLIC performs a 64 bit integer calculation (int64_t in C) when using -e2 and -e3. If using a 32-bit x86 compiler, it will result in slower compression/decompression speed.

　

# Usage

This program can compress gray BMP/PNM/PGM image file to NBLIC file (.nblic), or decompress NBLIC file to gray BMP/PNM/PGM image file.

Note: BMP, PNM, and PGM [2] are all image files without compression (i.e., they saves raw pixels).

### To compress:

```bash
nblic_codec -c [-swiches] <input-image-file> <output-file(.nblic)>
  where:
    <input-image-file> can be .pgm, .pnm, or .bmp (must be gray 8-bit image)
    <output-file> can only be .nblic
  swiches:
    -n<number> : near, can be 0 (lossless) or 1,2,3,... (lossy)
    -e<number> : effort, can be 0 (fastest), 1 (normal), 2 (slow), or 3 (slowest)
                 note: when using lossy (near>0), effort cannot be 0
    -v         : verbose, print infomations
    -V         : verbose, print infomations and progress
    -t         : multithread speedup, currently only support -e0 on Windows
```

For example :

fastest lossless compression:

```bash
./nblic_codec -c -V -n0 -e0 in.bmp out.nblic
```

fastest lossless compression with multithread speedup:

```bash
./nblic_codec -c -V -n0 -e0 -t in.bmp out.nblic
```

slowest lossless compression:

```bash
./nblic_codec -c -V -n0 -e3 in.bmp out.nblic
```

slow lossy compression:

```bash
./nblic_codec -c -V -n2 -e2 in.bmp out.nblic
```

Note: There is no requirement for the order of switches. Compact switches are also supported, such as:

```bash
./nblic_codec -cn2e2V in.bmp out.nblic
```

### To decompress

```bash
nblic_codec -d [-swiches] <input-file(.nblic)> <output-image-file>
  where:
    <input-file> can only be .nblic
    <output-image-file> can be .pgm, .pnm, or .bmp
  swiches:
    -v : verbose, print infomations
    -V : verbose, print infomations and progress
```

For example:

```bash
./nblic_codec -dV in.nblic out.bmp
```

　

### Run in Windows

In Windows, just use `.\nblic_codec.exe` instead of `./nblic_codec` .

　

　

　

# Comparison

This section presents the results of comparing **NBLIC** (this design) to other lossless image compression formats.

*Note: for more comparison result, see [Gray 8-bit Lossless Compression Bench](https://github.com/WangXuan95/Gray8bit-Image-Compression-Benchmark).*

## Formats for participating in comparison

The following table shows the image formats involved in the comparison.

|          Compressed format          |   Using what software?    |
| :---------------------------------: | :-----------------------: |
| PNG (deeply optimized) |                OptiPNG [3]                |
|     AVIF lossless      | Python pillow 9.5.0 with pillow-heif [1] |
|   JPEG2000 lossless    |          Python pillow 9.5.0 [4]          |
|      JPEG-LS [8]       | Python pillow 9.5.0 with pillow-jpls [10] |
|     WEBP lossless      |          Python pillow 9.5.0 [4]          |
|    WEBP2 lossless *    |                     ???                     |
|              CALIC [6]              | CALIC executable file [7] |
| JPEG-XL lossless  |                 libjxl (v0.9.0) [5]                 |
|              **NBLIC**              |       **This repo**       |

> \* I would like to add WEBP2 in comparison in the future, but I currently do not have it, since it is too new to obtain.
>

　

## Encode Commands / Arguments

The following table shows the compression command/arguments for each formats. They all all operated in lossless compression mode.

I try to use the "slowest but deepest compression" configuration for all these formats. But except JPEG2000 and AVIF, since I haven't studied their arguments yet.

Note that since I use Python's pillow library to encode/decode some formats, some of the following instructions are in the Python language.

|      Format       |  In Which?  | Encode Command / Arguments                             |
| :---------------: | :---------: | :----------------------------------------------------- |
|        PNG        | Windows CMD | `optipng.exe -o7 a.pgm -out a.png`                     |
|   AVIF lossless   |   Python    | `img.save('a.avif', quality=-1)`                       |
| JPEG2000 lossless |   Python    | `img.save('a.j2k', irreversible=False)`                |
|      JPEG-LS      |   Python    | `img.save('a.jls', spiff=None)`                        |
|   WEBP lossless   |   Python    | `img.save('a.webp', lossless=True, method=6)`          |
|       CALIC       | Windows CMD | `calic8e.exe a.raw <width> <height> <depth> 0 a.calic` |
|      JPEG-XL      | Windows CMD | `cjxl.exe a.pgm a.jxl -q 100 -e 9`                     |
|  **NBLIC** (-e?)  | Windows CMD | `nblic_codec.exe -cVtn0e? a.pgm a.nblic`               |

　

## Decode Commands / Arguments

Decoding commands are simple, as shown in following table.

|      Format       |  In Which?  | Decode Command / Arguments            |
| :---------------: | :---------: | :------------------------------------ |
|        PNG        |   Python    | `numpy.asarray(Image.open('a.png'))`  |
|   AVIF lossless   |   Python    | `numpy.asarray(Image.open('a.avif'))` |
| JPEG2000 lossless |   Python    | `numpy.asarray(Image.open('a.j2k'))`  |
|      JPEG-LS      |   Python    | `numpy.asarray(Image.open('a.jls'))`  |
|   WEBP lossless   |   Python    | `numpy.asarray(Image.open('a.webp'))` |
|       CALIC       | Windows CMD | `calic8d.exe a.calic a.raw`           |
|      JPEG-XL      | Windows CMD | `djxl.exe a.jxl a.pgm`                |
|     **NBLIC**     | Windows CMD | `nblic_codec.exe -dV a.nblic a.pgm`   |

　

## Environment

All encoders/decoders are run on my laptop (Windows 10, Intel Core i7-12700H, 16GB DDR4 3200MHz). All the Linux commands are run in Windows Subsystem of Linux (WSL).

　

## Comparative indicators

- **Compressed BPP** = total size after compression in bits / total pixel count. **The smaller the better**.
- **Compression time** is the total time to compress the entire image dataset. The smaller the better.
- **Decompression time** is the total time to decompress the entire image dataset. The smaller the better.

　

## Benchmarks

I use the following four image datasets for comparison.

|             |                     benchmark1                     |                          benchmark2                          |                          benchmark3                          |
| :---------: | :------------------------------------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
| source from | [Kodak Image](https://r0k.us/graphics/kodak/) [11] | [CLIC'24 test set](https://www.compression.cc/tasks/#image) [14] | [Gray 8 bit](https://imagecompression.info/test_images/) [12] |
| preprocess  |                convert to gray8bit                 |                     convert to gray8bit                      |                           needn't                            |
| image count |                         24                         |                              32                              |                              15                              |
| total bytes |                     9,437,544                      |                          88,605,214                          |                         162,870,743                          |
|    notes    |       [img_kodak](./img_kodak) in this repo        |                                                              |                                                              |

　

## Results on benchmark1

|         Format         | compressed BPP | compress time (s) | decompress time (s) |
| :--------------------: | :------------: | :---------------: | :-----------------: |
|          PNG           |     4.637      |       124.3       |         0.1         |
|   AVIF (not deepest)   |     4.646      |        3.1        |         0.9         |
| JPEG2000 (not deepest) |     4.462      |        1.1        |         0.8         |
|        JPEG-LS         |     4.338      |        0.5        |         0.5         |
|          WEBP          |     4.332      |       118.6       |         0.3         |
|         CALIC          |     4.181      |        8.1        |         7.6         |
|        JPEG-XL         |     4.143      |       77.1        |         1.1         |
|    **NBLIC** (-e0)     |   **4.227**    |        1.0        |         2.8         |
|    **NBLIC** (-e1)     |   **4.146**    |        2.6        |         2.7         |
|    **NBLIC** (-e2)     |   **4.088**    |       10.1        |        10.3         |
|    **NBLIC** (-e3)     |   **4.066**    |       29.2        |        29.3         |

　

## Results on benchmark2

|         Format         | compressed BPP | compress time (s) | decompress time (s) |
| :--------------------: | :------------: | :---------------: | :-----------------: |
|          PNG           |     3.510      |      1987.0       |         0.7         |
|   AVIF (not deepest)   |     3.448      |       18.6        |         5.8         |
| JPEG2000 (not deepest) |     3.317      |        6.2        |         5.3         |
|        JPEG-LS         |     3.185      |        2.5        |         2.4         |
|          WEBP          |     3.253      |       448.1       |         1.5         |
|         CALIC          |     3.012      |       95.0        |        56.3         |
|        JPEG-XL         |     2.958      |       641.8       |         2.1         |
|    **NBLIC** (-e0)     |   **3.056**    |        2.6        |         6.2         |
|    **NBLIC** (-e1)     |   **3.027**    |       11.2        |        11.8         |
|    **NBLIC** (-e2)     |   **2.981**    |       83.0        |        83.7         |
|    **NBLIC** (-e3)     |   **2.963**    |       261.4       |        263.8        |

　

## Results on benchmark3

|         Format         | compressed BPP | compress time (s) | decompress time (s) |
| :--------------------: | :------------: | :---------------: | :-----------------: |
|          PNG           |     3.689      |      3696.6       |         1.4         |
|   AVIF (not deepest)   |     3.745      |       23.6        |         7.7         |
| JPEG2000 (not deepest) |     3.465      |       13.6        |        10.5         |
|        JPEG-LS         |     3.429      |        4.9        |         4.1         |
|          WEBP          |     3.395      |       887.9       |         2.8         |
|         CALIC          |     3.255      |       33.9        |        33.1         |
|        JPEG-XL         |     3.085      |      1357.4       |         2.4         |
|    **NBLIC** (-e0)     |   **3.259**    |        3.2        |         6.5         |
|    **NBLIC** (-e1)     |   **3.236**    |       17.0        |        18.2         |
|    **NBLIC** (-e2)     |   **3.012**    |       142.9       |        144.2        |
|    **NBLIC** (-e3)     |   **2.987**    |       455.4       |        462.1        |

　

　

　

# Reference

[1] pillow-heif documentation : https://pillow-heif.readthedocs.io/en/latest/index.html

[2] PGM Image File Specification : https://netpbm.sourceforge.net/doc/pgm.html#index

[3] OptiPNG: Advanced PNG Optimizer : https://optipng.sourceforge.net/

[4] Python Pillow Supported Image file formats : https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html

[5] libjxl : https://github.com/libjxl/libjxl/releases

[6] CALIC paper: Context-based, adaptive, lossless image coding : https://ieeexplore.ieee.org/document/585919/

[7] CALIC executable file : https://www.ece.mcmaster.ca/~xwu/calicexe/

[8] JPEG-LS baseline (ITU-T T.87) : https://www.itu.int/rec/T-REC-T.87/en

[9] JPEG-LS extension (ITU-T T.870) : https://www.itu.int/rec/T-REC-T.870/en 

[10] pillow-jpls: A JPEG-LS plugin for the Python *Pillow* library: https://github.com/planetmarshall/pillow-jpls

[11] Kodak Lossless True Color Image Suite : https://r0k.us/graphics/kodak/

[12] Rawzor's Lossless compression benchmark for camera raw images: https://imagecompression.info/test_images/

[13] The USC-SIPI Image Database: Volume 3: Miscellaneous: https://sipi.usc.edu/database/database.php?volume=misc

[14] Test Set of CLIC2024 : https://www.compression.cc/tasks/#image

