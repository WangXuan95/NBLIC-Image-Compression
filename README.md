 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# enhanced JPEG-LS extension

An enhanced version of JPEG-LS extension (ITU-T T.870) image encoder/decoder in C language.

Its lossless compression ratio is better than PNG, JPEG2000, WEBP, HEIF, JPEG-LS baseline, and JPEG-LS extension.

　

## Why ?

JPEG-LS (JLS) is a lossless/lossy compression standard. There are 2 generations:

- JPEG-LS baseline (ITU-T T.87) [8]
- JPEG-LS extension (ITU-T T.870) [1], which has a higher compression ratio.

The current publicly implementations online are all about JPEG-LS baseline, and none of them supports JPEG-LS extension.

This repo is a enhanced implementation of JPEG-LS extension, which gets a higher compression ratio than original JPEG-LS extension.

Note: due to my modification, this repo is not compatible with the original JPEG-LS extension standard.

　

# Development progress

- [x] grey 8 bit lossless image encode/decode
- [x] grey 8 bit lossy (near-lossless) image encode/decode

　

# Code list

The code files are in pure-C, located in the [src](./src) folder:

- `JLSx.c` : Implement a enhanced JLS extension encode/decode
- `JLSx.h` : Expose the functions of enhanced JLS extension encode/decode to users.
- `JLSxMain.c` : A main program with `main()` function, which call `JLSx.h` to achieve image file compression/decompression.

　

# Compile

### Compile in Windows (CMD)

If you add the Microsoft C compiler (`cl. exe`) of Visual Studio to environment variables, you can compile using the command line (CMD).

```bash
cl src\*.c /FeJLSx.exe /Ox
```

We'll get the executable file `JLSx.exe` . Here I've compiled it for you, you can use it directly.

### Compile in Linux

Run the command in the current directory:

```bash
gcc src/*.c -o JLSx -O3 -Wall
```

We'll get the binary file `JLSx` . Here I've compiled it for you, you can use it directly.

　

# Run image compression/decompression

This program can compress `.pgm` image file to `.jlsxn` image file. Or decompress  `.jlsxn` image file to `.pgm` image file.

Note that `.pgm` is a simple uncompressed image format (see PGM Image File Specification [2]). PGM file format contains:

- A simple header that contains the width, height, and depth of this image.
- The raw pixel values of this image.

### Run in Windows (CMD)

用以下命令把图像文件 `1.pgm` 压缩为 `1.jlsxn`  。

Use following command to compress `1.pgm` to `1.jlsxn`

```bash
JLSx.exe 1.pgm 1.jlsxn <near>
```

Where `[near]` is a optional parameter of range 0\~9 :

- 0 : lossless (default)
- 1\~9 : lossy. The larger the near value, the higher distortion and the lower compressed size.

Use following command to compress `1.jlsxn` to `1.pgm`

```
JLSx.exe 1.jlsxn 1.pgm
```

### Run in Linux

The command format is similar to Windows. You only need to replace the executable file name with `./JLSx` .

　

# Evaluating compression ratio

This section presents the results of comparing **enhanced JPEG-LS extension** (this design) to other lossless image compression formats.

## Formats for participating in comparison

The following table shows the formats involved in the comparison and how they were generated. Note that they all operate in lossless compression mode.

|          Compressed format          |   Using what software?    |          options          |          meaning of options          |
| :---------------------------------: | :-----------------------: | :-----------------------: | :----------------------------------: |
|                 PNG                 |        OptiPNG [3]        |           `-o7`           |         heaviest compression         |
|              JPEG2000               |  Python Pillow 9.5.0 [4]  |   `irreversible=False`    |         lossless compression         |
|                WEBP                 |  Python Pillow 9.5.0 [4]  | `lossless=True, method=6` |    lossless heaviest compression     |
|   FLIF (now part of JPEG-XL) [5]    |         FLIF [5]          |        `-N -E100`         | non-interlaced, heaviest compression |
|              CALIC [6]              | CALIC executable file [7] |             -             |                  -                   |
|  JPEG-LS baseline (ITU-T T.87) [8]  |  Python Pillow-jpls [9]   |         `near=0`          |         lossless compression         |
| JPEG-LS extension (ITU-T T.870) [1] |  我按照文档实现，未开源   |         `near=0`          |         lossless compression         |
|   **enhanced JPEG-LS extension**    |       **This repo**       |         `near=0`          |         lossless compression         |

## Benchmark

Use the following two image datasets to compare compression ratios.

|                                 |                  small image benchmark                   |                    large image benchmark                     |
| :-----------------------------: | :------------------------------------------------------: | :----------------------------------------------------------: |
|             source              | [Kodak Image Suite](https://r0k.us/graphics/kodak/) [10] | Gray 8 bit in [this website](http://imagecompression.info/test_images/) |
|           preprocess            |                convert RGB to grey 8-bit                 |                           needn't                            |
| pixel format before compression |                        grey 8-bit                        |                          grey 8-bit                          |
| file format before compression  |                           .pgm                           |                             .pgm                             |
| data volume before compression  |                      9437544 bytes                       |                       162870743 bytes                        |
|           image count           |                            24                            |                              15                              |
|              notes              |        See [img_kodak](./img_kodak) in this repo         |      didn't include in this repo. Download it yourself       |

## result of compression ratio

Compress the above two benchmarks and obtain the compression ratio as follows.

Note: Compression ratio=size before compression/size after compression. The larger the better.

|             Compressed format              | small image benchmark | large image benchmark |
| :----------------------------------------: | :-------------------: | :-------------------: |
|                    PNG                     |         1.725         |         2.202         |
|                  JPEG2000                  |         1.793         |         2.309         |
|                    WEBP                    |         1.847         |         2.356         |
|         FLIF (now part of JPEG-XL)         |         1.864         |  **2.534** (**1st**)  |
|                   CALIC                    |  **1.913** (**1st**)  |         2.458         |
|       JPEG-LS baseline (ITU-T T.87)        |         1.845         |         2.333         |
|      JPEG-LS extension (ITU-T T.870)       |         1.884         |         2.411         |
| **enhanced JPEG-LS extension (this repo)** |  **1.911** (**2nd**)  |  **2.470** (**2nd**)  |

　

　

　

# Reference

[1] JPEG-LS extension (ITU-T T.870) : https://www.itu.int/rec/T-REC-T.870/en 

[2] PGM Image File Specification : https://netpbm.sourceforge.net/doc/pgm.html#index

[3] OptiPNG: Advanced PNG Optimizer : https://optipng.sourceforge.net/

[4] Python Pillow Supported Image file formats : https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html

[5] FLIF: Free Lossless Image Format : https://github.com/FLIF-hub/FLIF

[6] CALIC paper: Context-based, adaptive, lossless image coding : https://ieeexplore.ieee.org/document/585919/

[7] CALIC executable file : https://www.ece.mcmaster.ca/~xwu/calicexe/

[8] JPEG-LS baseline (ITU-T T.87) : https://www.itu.int/rec/T-REC-T.87/en

[9] pillow-jpls: A JPEG-LS plugin for the Python *Pillow* library: https://github.com/planetmarshall/pillow-jpls

[10] Kodak Lossless True Color Image Suite : https://r0k.us/graphics/kodak/
