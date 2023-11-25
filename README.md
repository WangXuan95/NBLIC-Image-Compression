 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# NBLIC: lossless image compression

NBLIC (New Bee Lossless Image Compression) is a lossless & near-lossless gray image codec.

### Features

- **Very high compression ratio**, significantly higher than state-of-the-art lossless image compression standards such as JPEG-XL lossless, AVIF lossless, and maybe WebP2 lossless (will be compared in future).
- **Low code** (only 1000 lines of C language of encoder and decoder).
- Acceptable performance.
- Just single pass scan encode/decode, friendly for FPGA hardware streaming implementation.

### Development progress

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

If you add the Microsoft C compiler (`cl.exe`) to environment variables, you can compile using the command line (CMD).

```powershell
cl src\*.c /Fenblic_codec.exe /Ox
```

We'll get the executable file `nblic_codec.exe` . Here I've compiled it for you, you can use it directly.

　

# Usage

This program can compress gray BMP/PNM/PGM image file to NBLIC file (.nblic), or decompress NBLIC file to gray BMP/PNM/PGM image file.

Note: BMP, PNM, and PGM [2] are all image files without compression (i.e., they saves raw pixels).

### Compress

In Linux, Use following command to compress a BMP/PNM/PGM file to a NBLIC file.

```bash
./nblic_codec  <input-image-file>  <output-file(.nblic)>  [<effort>]  [<near>]
```

Where:

- `input-image-file` can ends with `.pgm` , `.pnm` , or `.bmp`
- `output-file` can only ends with `.nblic`
- `effort` can be 1 (fastest), 2, 3, or 4 (deepest)
- `near` can be 0 (lossless) or 1,2,3,... (lossy)

For example:

```bash
./nblic_codec img_kodak/01.bmp out.nblic 3 0
```

### Decompress

In Linux, Use following command to decompress a NBLIC file to a BMP/PNM/PGM file.

```bash
./nblic_codec  <input-file(.nblic)>  <output-image-file>
```

Where:

- `input-file` can only ends with `.nblic`
- `output-image-file` can only ends with `.pgm` , `.pnm` , or `.bmp`

For example:

```bash
./nblic_codec out.nblic out.bmp
```

### Run in Windows

In Windows, just use `.\nblic_codec.exe` instead of `./nblic_codec` .

　

　

　

# Comparison

This section presents the results of comparing **NBLIC** (this design) to other lossless image compression formats.

## Formats for participating in comparison

The following table shows the image formats involved in the comparison.

|          Compressed format          |   Using what software?    |
| :---------------------------------: | :-----------------------: |
|   JPEG2000 lossless    |          Python pillow 9.5.0 [4]          |
| PNG (deeply optimized) |                OptiPNG [3]                |
|      JPEG-LS [8]       | Python pillow 9.5.0 with pillow-jpls [10] |
|     AVIF lossless      | Python pillow 9.5.0 with pillow-heif [1] |
|     WEBP lossless      |          Python pillow 9.5.0 [4]          |
|    WEBP2 lossless *    |                     ???                     |
|              CALIC [6]              | CALIC executable file [7] |
| JPEG-XL lossless* [5]  |                 FLIF [5]                  |
|              **NBLIC**              |       **This repo**       |

> \* I would like to add WEBP2 in comparison in the future, but I currently do not have it, since it is too new to obtain.
>
> \* **JPEG-XL lossless (also called FLIF)** is the state-of-the-art lossless image compression standard by 2023. WebP2 lossless may be on par with JPEG-XL.

　

### Encode Commands / Arguments

The following table shows the compression command/arguments for each formats. Note that they all operate in lossless compression mode.

I try to use the "slowest but deepest compression" configuration for all these formats. But except JPEG2000 and AVIF, since I haven't studied their arguments yet.

Note that since I use Python's pillow library to encode/decode some formats, some of the following instructions are in the Python language.

|        Format        |   In Which?   | Encode Command / Arguments                             | Meaning                       |
| :------------------: | :-----------: | :----------------------------------------------------- | :---------------------------- |
|       JPEG2000       |    Python     | `img.save('a.j2k', irreversible=False)`                | lossless                      |
|       JPEG-LS        |    Python     | `img.save('a.jls', irreversible=False)`                | lossless                      |
|         PNG          |  Windows CMD  | `optipng.exe -o7 a.pgm -out a.png`                     | lossless, deepest compression |
|         AVIF         |    Python     | `img.save('a.avif', quality=-1)`                       | lossless                      |
|         WEBP         |    Python     | `img.save('a.webp', lossless=True, method=6)`          | lossless, deepest compression |
|        CALIC         |  Windows CMD  | `calic8e.exe a.raw <width> <height> <depth> 0 a.calic` | lossless                      |
|    JPEG-XL (FLIF)    | Linux command | `./flif -e -N -E100 a.pgm a.flif`                      | lossless, deepest compression |
| **NBLIC** (effort=?) | Linux command | `./nblic_codec a.pgm a.nblic ? 0`                      | lossless                      |

　

### Decode Commands / Arguments

Decoding commands are simple, as shown in following table.

|     Format     |   In Which?   | Decode Command / Arguments            |
| :------------: | :-----------: | :------------------------------------ |
|    JPEG2000    |    Python     | `numpy.asarray(Image.open('a.j2k'))`  |
|    JPEG-LS     |    Python     | `numpy.asarray(Image.open('a.jls'))`  |
|      PNG       |    Python     | `numpy.asarray(Image.open('a.png'))`  |
|      AVIF      |    Python     | `numpy.asarray(Image.open('a.avif'))` |
|      WEBP      |    Python     | `numpy.asarray(Image.open('a.webp'))` |
|     CALIC      |  Windows CMD  | `calic8d.exe a.calic a.raw`           |
| JPEG-XL (FLIF) | Linux command | `./flif -d a.flif a.pgm`              |
|   **NBLIC**    | Linux command | `./nblic_codec a.nblic a.pgm`         |

　

## Comparative indicators

- **Compressed BPP** = total size after compression in bits / total pixel count. **The smaller the better**.
- **Compression time** is the total time to compress the entire image dataset. The smaller the better.
- **Decompression time** is the total time to decompress the entire image dataset. The smaller the better.

　

## Benchmarks

I use the following four image datasets for comparison.

|                     |                          benckmark1                          |                     benchmark2                     |                          benchmark3                          |                          benchmark4                          |
| :-----------------: | :----------------------------------------------------------: | :------------------------------------------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
|     source from     | [USC-SIPI](https://sipi.usc.edu/database/database.php?volume=misc) [13] | [Kodak Image](https://r0k.us/graphics/kodak/) [11] | [CLIC'24 test set](https://www.compression.cc/tasks/#image) [14] | [Rawzor's Benchmark](https://imagecompression.info/test_images/) [12] |
|     preprocess      |                    convert to gray 8-bit                     |               convert to gray 8-bit                |                    convert to gray 8-bit                     |                           needn't                            |
|     image count     |                              39                              |                         24                         |                              32                              |                              15                              |
| data volume (bytes) |                          9,830,991                           |                     9,437,544                      |                          88,605,214                          |                         162,870,743                          |
|        notes        |                                                              |     see [img_kodak](./img_kodak) in this repo      |                                                              |                                                              |

　

## Results on benchmark1

|        Format        | compressed BPP | compress time (s) | decompress time (s) |
| :------------------: | :------------: | :---------------: | :-----------------: |
|       JPEG2000       |     4.584      |        1.3        |         1.0         |
|         PNG          |     4.537      |       109.1       |         0.1         |
|         AVIF         |     4.701      |        3.4        |         1.0         |
|         WEBP         |     4.190      |       100.9       |         0.3         |
|        CALIC         |     4.277      |       11.9        |        11.2         |
|    JPEG-XL (FLIF)    |   **4.157**    |       13.4        |         4.4         |
| **NBLIC** (effort=1) |   **4.163**    |        4.9        |         4.9         |
| **NBLIC** (effort=2) |   **4.117**    |       15.0        |        12.3         |
| **NBLIC** (effort=3) |   **4.106**    |       47.0        |        27.2         |
| **NBLIC** (effort=4) |   **4.106**    |       112.0       |        29.0         |

　

## Results on benchmark2

|          Format           | compressed BPP | compress time (s) | decompress time (s) |
| :-----------------------: | :------------: | :---------------: | :-----------------: |
|         JPEG2000          |      4.46      |        1.1        |         0.8         |
|          JPEG-LS          |      4.34      |        0.5        |         0.5         |
|            PNG            |      4.64      |       124.3       |         0.1         |
|           AVIF            |      4.64      |        2.9        |         0.8         |
|           WEBP            |      4.33      |       118.6       |         0.3         |
|           CALIC           |     4.181      |        8.1        |         7.6         |
|      JPEG-XL (FLIF)       |     4.295      |       10.0        |         3.1         |
| ~~NBLIC old version 0.1~~ |   ~~4.113~~    |      ~~9.2~~      |       ~~7.6~~       |
|   **NBLIC** (effort=1)    |   **4.152**    |        3.2        |         3.1         |
|   **NBLIC** (effort=2)    |   **4.091**    |       11.4        |        10.7         |
|   **NBLIC** (effort=3)    |   **4.072**    |       40.5        |        29.7         |
|   **NBLIC** (effort=4)    |   **4.071**    |       100.7       |        35.0         |

　

## Results on benchmark3

|        Format        | compressed BPP | compress time (s) | decompress time (s) |
| :------------------: | :------------: | :---------------: | :-----------------: |
|       JPEG2000       |     3.317      |        6.2        |         5.3         |
|         PNG          |     3.510      |      1987.0       |         0.7         |
|         AVIF         |     3.448      |       18.6        |         5.8         |
|         WEBP         |     3.253      |       448.1       |         1.5         |
|        CALIC         |   **3.012**    |       95.0        |        56.3         |
|    JPEG-XL (FLIF)    |     3.050      |       94.1        |        14.7         |
| **NBLIC** (effort=1) |   **3.032**    |       14.6        |        14.3         |
| **NBLIC** (effort=2) |   **2.984**    |       96.8        |        81.6         |
| **NBLIC** (effort=3) |   **2.966**    |       353.6       |        238.6        |
| **NBLIC** (effort=4) |   **2.965**    |       904.1       |        281.1        |

　

## Results on benchmark4

|          Format           | compressed BPP | compress time (s) | decompress time (s) |
| :-----------------------: | :------------: | :---------------: | :-----------------: |
|         JPEG2000          |      3.46      |       13.6        |        10.5         |
|          JPEG-LS          |      3.44      |        4.9        |         4.1         |
|            PNG            |      3.63      |      4462.5       |         2.2         |
|           AVIF            |      3.74      |       23.6        |         7.7         |
|           WEBP            |      3.39      |      1910.1       |         3.0         |
|           CALIC           |     3.255      |       33.1        |        33.1         |
|      JPEG-XL (FLIF)       |   **3.159**    |       186.5       |        28.3         |
| ~~NBLIC old version 0.1~~ |   ~~3.036~~    |     ~~139.7~~     |      ~~91.6~~       |
|   **NBLIC** (effort=1)    |   **3.242**    |       26.5        |        25.8         |
|   **NBLIC** (effort=2)    |   **3.017**    |       177.7       |        152.7        |
|   **NBLIC** (effort=3)    |   **2.993**    |       650.6       |        472.6        |
|   **NBLIC** (effort=4)    |   **2.992**    |      1751.9       |        535.6        |

　

　

　

# Reference

[1] pillow-heif documentation : https://pillow-heif.readthedocs.io/en/latest/index.html

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

[12] Rawzor's Lossless compression benchmark for camera raw images: https://imagecompression.info/test_images/

[13] The USC-SIPI Image Database: Volume 3: Miscellaneous: https://sipi.usc.edu/database/database.php?volume=misc

[14] Test Set of CLIC2024 : https://www.compression.cc/tasks/#image

