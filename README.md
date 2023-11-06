 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# NBLIC: lossless image compression

NBLIC - Niu-Bi Lossless Image Compression, is a lossless & near-lossless image codec.

### Development progress

- [x] 8-bit gray image lossless encode/decode is support now.
- [x] 8-bit gray image near-lossless encode/decode is support now.
- [ ] 24-bit RGB image lossless encode/decode will be supported in the future.
- [ ] 24-bit RGB image near-lossless encode/decode will be supported in the future.

### Features

- **Very high compression ratio**, higher than state-of-the-art lossless image compression standards such as JPEG-XL lossless, AVIF lossless, etc.
- **Low code** with pure C language (only 700 lines of C of encoder/decoder).
- Acceptable performance.
- Just single pass scan encode/decode, good for FPGA hardware streaming implementation.

　

# Code list

The code files are in pure-C, located in the [src](./src) folder:

- `NBLIC.c` : Implement NBLIC encoder/decoder
- `NBLIC.h` : Expose the functions of NBLIC encoder/decoder to users.
- `FileIO.c` : Provide PGM image file reading/writing functions, provide binary file reading/writing functions.
- `FileIO.h` : Expose the functions in `FileIO.c` to users.
- `NBLIC_main.c` : A main program with `main()` function, which call `NBLIC.h` and `FileIO.h` to achieve image file encoding/decoding.

　

# Compile

### Compile in Windows (CMD)

If you add the Microsoft C compiler (`cl.exe`) to environment variables, you can compile using the command line (CMD).

```powershell
cl src\*.c /Fenblic_codec.exe /Ox
```

We'll get the executable file `nblic_codec.exe` . Here I've compiled it for you, you can use it directly.

### Compile in Linux

Run the command in the current directory:

```bash
gcc src/*.c -o nblic_codec -O3 -Wall
```

We'll get the binary file `nblic_codec` . Here I've compiled it for you, you can use it directly.

　

# Run image encode/decode

This program can compress a PGM image file (.pgm) to a NBLIC file (.nblic), or decompress a NBLIC file to a PGM image file

Note that PGM is a simple uncompressed image format (see PGM Image File Specification [2]). PGM file contains:

- A simple header that contains the width, height, and depth of this image.
- The raw pixel values of this image.

### Run in Windows (CMD)

Use following command to compress `1.pgm` to `1.nblic`

```powershell
nblic_codec.exe 1.pgm 1.nblic [near] [effort]
```

Where `[near]` is a optional parameter of range 0\~9 :

- 0 : lossless (default)
- 1\~9 : near-lossless. The larger the near value, the higher distortion and the lower compressed size.

And `[offert]` is a optional parameter of range 1\~2 :

- 1 : fast mode
- 2 : slow mode, higher compression ratio

Use following command to decompress `1.nblic` to `1.pgm` with `near=0` (lossless) and `effort=2`

```powershell
nblic_codec.exe 1.nblic 1.pgm 0 2
```

### Run in Linux

The command format is similar to Windows. You only need to replace the executable file name with `./nblic_codec` .

　

　

# Comparison

This section presents the results of comparing **NBLIC** (this design) to other lossless or near-lossless image compression formats.

## Formats for participating in comparison

The following table shows the image formats involved in the comparison.

|          Compressed format          |   Using what software?    |
| :---------------------------------: | :-----------------------: |
|   JPEG2000 lossless    |          Python pillow 9.5.0 [4]          |
| PNG (deeply optimized) |                OptiPNG [3]                |
|      JPEG-LS [8]       | Python pillow 9.5.0 with pillow-jpls [10] |
|     HEIF lossless      | Python pillow 9.5.0 with pillow-heif [11] |
|     AVIF lossless      | Python pillow 9.5.0 with pillow-heif [11] |
|     WEBP lossless      |          Python pillow 9.5.0 [4]          |
|    WebP2 lossless *    |                     -                     |
|              CALIC [6]              | CALIC executable file [7] |
| JPEG-XL lossless* [5]  |                 FLIF [5]                  |
|              **NBLIC**              |       **This repo**       |

> \* I would like to add WebP2's data in the future, but I currently do not have it.
>
> \* **JPEG-XL lossless (also called FLIF)** is the state-of-the-art lossless image compression standard by 2023. WebP2 lossless may be on par with JPEG-XL.

　

### Encode Commands / Arguments

The following table shows the compression command/arguments for each formats. Note that they all operate in lossless compression mode.

I try to use the "highest compression ratio" configuration of these formats. But it is not yet clear whether the HEIF and AVIF are really operated in the highest compression ratio mode.

Note that since we use Python's pillow library to encode/decode some formats, some of the following instructions are in the Python language

|  Format   |                  Encode command / arguments                  | meaning                      |
| :-------: | :----------------------------------------------------------: | :--------------------------- |
| JPEG2000  |           `img.save('a.j2k', irreversible=False)`            | lossless compression         |
|    PNG    |              `optipng.exe -o7 a.pgm -out a.png`              | deepest lossless compression |
|  JPEG-LS  |           `img.save('a.jls', irreversible=False)`            | lossless compression         |
|   HEIF    | `img.save('a.heif', quality=-1, enc_params={'preset':'placebo'})` | deepest lossless compression |
|   AVIF    |               `img.save('a.heif', quality=-1)`               | lossless compression         |
|   WEBP    |        `img.save('a.webp', lossless=True, method=6)`         | deepest lossless compression |
|   CALIC   |    `calic8e.exe a.raw <width> <height> <depth> 0 a.calic`    | lossless compression         |
|  JPEG-XL  |              `./flif -e -N -E1100 a.pgm a.flif`              | deepest lossless compression |
| **NBLIC** |              `nblic_codec.exe a.pgm a.nblic 0`               | lossless compression         |

　

### Decode Commands / Arguments

Decoding commands are simple, as shown in following table.

|  Format   |      Decode command / arguments       |
| :-------: | :-----------------------------------: |
| JPEG2000  | `numpy.asarray(Image.open('a.j2k'))`  |
|    PNG    | `numpy.asarray(Image.open('a.png'))`  |
|  JPEG-LS  | `numpy.asarray(Image.open('a.jls'))`  |
|   HEIF    | `numpy.asarray(Image.open('a.heif'))` |
|   AVIF    | `numpy.asarray(Image.open('a.avif'))` |
|   WEBP    | `numpy.asarray(Image.open('a.webp'))` |
|   CALIC   |      `calic8d.exe a.calic a.raw`      |
|  JPEG-XL  |       `./flif -d a.flif a.pgm`        |
| **NBLIC** |    `nblic_codec.exe a.nblic a.pgm`    |

　

## Benchmark

Use the following two image datasets in comparison.

|                                 |                  small image benchmark                   |                    large image benchmark                     |
| :-----------------------------: | :------------------------------------------------------: | :----------------------------------------------------------: |
|             source              | [Kodak Image Suite](https://r0k.us/graphics/kodak/) [11] | Gray 8 bit in [this website](http://imagecompression.info/test_images/) [12] |
|           preprocess            |                convert RGB to grey 8-bit                 |                           needn't                            |
| pixel format before compression |                        grey 8-bit                        |                          grey 8-bit                          |
| file format before compression  |                           .pgm                           |                             .pgm                             |
| data volume before compression  |                      9437544 bytes                       |                       162870743 bytes                        |
|           image count           |                            24                            |                              15                              |
|              notes              |        See [img_kodak](./img_kodak) in this repo         |                   download it by yourself                    |

　

## Comparison results on the small image benchmark [11]

The following table shows the **compressed BPP, compression time**, and **decompression time** on the small image benchmark

Note: Compressed BPP = total size after compression in bits / total pixel count. The smaller the better.

Compression time and Decompress time is the total time to compress/decompress the entire image dataset.

|        Format        | compressed BPP | compress time (s) | decompress time (s) |
| :------------------: | :------------: | :---------------: | :-----------------: |
|       JPEG2000       |      4.46      |        1.1        |         0.8         |
|         PNG          |      4.64      |       124.3       |         0.1         |
|       JPEG-LS        |      4.34      |        0.5        |         0.5         |
|         HEIF         |      4.75      |        2.3        |         0.7         |
|         AVIF         |      4.64      |        2.9        |         0.8         |
|         WEBP         |      4.33      |       118.6       |         0.3         |
|        CALIC         |      4.18      |        8.1        |         7.6         |
|       JPEG-XL        |      4.30      |       10.0        |         3.1         |
| **NBLIC** (effort=1) | **4.15** (2nd) |       3.15        |        3.04         |
| **NBLIC** (effort=2) | **4.11** (1st) |       27.8        |        27.0         |



## Comparison results on the large image benchmark [12]

The following table shows the **compressed BPP, compression time**, and **decompression time** on the large image benchmark

|        Format        | compressed BPP | compress time (s) | decompress time (s) |
| :------------------: | :------------: | :---------------: | :-----------------: |
|       JPEG2000       |      3.46      |       13.6        |        10.5         |
|         PNG          |      3.63      |      4462.5       |         2.2         |
|       JPEG-LS        |      3.44      |        4.9        |         4.1         |
|         HEIF         |      3.96      |       17.6        |         6.4         |
|         AVIF         |      3.74      |       23.6        |         7.7         |
|         WEBP         |      3.39      |      1910.1       |         3.0         |
|        CALIC         |      3.26      |       38.8        |        36.9         |
|       JPEG-XL        |   3.16 (2nd)   |       289.6       |        90.7         |
| **NBLIC** (effort=1) | **3.24** (3rd) |       31.7        |        29.3         |
| **NBLIC** (effort=2) | **3.03** (1st) |       511.4       |        468.1        |

　

　

　

# Reference

[1] ANS entropy coding combining speed of Huffman coding with compression rate of arithmetic coding : https://arxiv.org/abs/1311.2540

[2] PGM Image File Specification : https://netpbm.sourceforge.net/doc/pgm.html#index

[3] OptiPNG: Advanced PNG Optimizer : https://optipng.sourceforge.net/

[4] Python Pillow Supported Image file formats : https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html

[5] FLIF: Free Lossless Image Format : https://github.com/FLIF-hub/FLIF

[6] CALIC paper: Context-based, adaptive, lossless image coding : https://ieeexplore.ieee.org/document/585919/

[7] CALIC executable file : https://www.ece.mcmaster.ca/~xwu/calicexe/

[8] JPEG-LS baseline (ITU-T T.87) : https://www.itu.int/rec/T-REC-T.87/en

[9] JPEG-LS extension (ITU-T T.870) : https://www.itu.int/rec/T-REC-T.870/en 

[10] pillow-jpls: A JPEG-LS plugin for the Python *Pillow* library: https://github.com/planetmarshall/pillow-jpls

[11] Kodak Lossless True Color Image Suite : https://r0k.us/graphics/kodak/

[12] Rawzor's Lossless compression software for camera raw images: https://imagecompression.info/test_images/
