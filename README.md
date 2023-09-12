 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# enhanced JPEG-LS extension

An enhanced version of JPEG-LS extension (ITU-T T.870) image encoder/decoder.

C 语言实现的增强型 JPEG-LS extension 图像编码/解码器 (比标准的 JPEG-LS extension 压缩率高)。

无损压缩率暴打 PNG, JPEG2000, WEBP, HEIF, JPEG-LS baseline 等格式。

　

## Why ?

JPEG-LS 是一种无损/有损压缩标准。目前网上的 JPEG-LS 实现均为 JPEG-LS baseline (ITU-T T.87) [8]，而没有 JPEG-LS extension (ITU-T T.870) [1]

我按照 JPEG-LS extension 标准文档编写了一个实现。并进行了一些改进，进一步提升压缩率。

　

# 开发进度

- [x] 灰度 8 bit 图像无损编码/解码：已充分测试，可以使用
- [x] 灰度 8 bit 图像有损编码/解码：已充分测试，可以使用
- [ ] 灰度 9-16 bit 图像无损编码/解码：尚未计划
- [ ] 灰度 9-16 bit 图像无损编码/解码：尚未计划
- [ ] 彩色图像编码/解码：尚未计划

　

# 代码文件概览

代码文件在目录 src 中。包括 2 个文件：

- `JLSx.c` : 实现了 enhanced JPEG-LS extension 编码/解码的接口函数
- `JLSx.h` : 包含了 enhanced JPEG-LS extension 编码/解码的接口函数的头文件
- `JLSxMain.c` : 包含 `main` 函数的文件，调用 `JLSx.h` 进行图像文件的压缩

　

# 编译

### Windows (命令行)

如果你把 Visual Studio 里的 C 编译器 (`cl.exe`) 加入了环境变量，也可以用命令行 (CMD) 进行编译。在本目录里运行命令：

```bash
cl src\*.c /FeJLSx.exe /Ox
```

产生可执行文件 `JLSx.exe` 。这里我已经编译好的可执行文件放在了本库中，你可以直接使用。

### Linux (命令行)

在本目录里运行命令：

```bash
gcc src/*.c -o JLSx -O3 -Wall
```

产生可执行文件 `JLSx` 。这里我已经编译好的可执行文件放在了本库中，你可以直接使用。

　

# 运行

### Windows (命令行)

用以下命令把图像文件 `1.pgm` 压缩为 `1.jlsxn`  。

其中 `<near>` 值可以取 0\~9 。0 代表无损，≥1 代表有损，越大则压缩率越高，图像质量越差

```bash
JLSx.exe 1.pgm 1.jlsxn <near>
```

用以下命令把图像文件 `1.jlsxn`  解压为 `1.pgm` 。

```
JLSx.exe 1.jlsxn 1.pgm
```

> :warning: `.pgm` 是一种非压缩的灰度图像文件格式，详见 [2]

### Linux (命令行)

命令格式与 Windows 类似，把可执行文件换成 `./JLSx` 即可。

　

# 压缩率评估和比较

本节展示 **enhanced JPEG-LS extension** (本设计) 与其它无损图像压缩格式对比的结果。

## 参与比较的格式

下表展示了参与对比的格式，以及它们是如何生成的。注意：它们都工作在无损压缩模式下。

|              压缩格式               | 用什么软件来产生这种压缩格式? |           选项            |      选项含义      |
| :---------------------------------: | :---------------------------: | :-----------------------: | :----------------: |
|                 PNG                 |          OptiPNG [3]          |           `-o7`           |     最高压缩率     |
|              JPEG2000               |    Python Pillow 9.5.0 [4]    |   `irreversible=False`    |      无损压缩      |
|                WEBP                 |    Python Pillow 9.5.0 [4]    | `lossless=True, method=6` |  无损，最高压缩率  |
|   FLIF (now part of JPEG-XL) [5]    |           FLIF [5]            |        `-N -E100`         | 非交错，最高压缩率 |
|              CALIC [6]              |   CALIC executable file [7]   |             -             |         -          |
|  JPEG-LS baseline (ITU-T T.87) [8]  |    Python Pillow-jpls [9]     |         `near=0`          |      无损压缩      |
| JPEG-LS extension (ITU-T T.870) [1] |    我按照文档实现，未开源     |         `near=0`          |      无损压缩      |
|   **enhanced JPEG-LS extension**    |          **本代码**           |         `near=0`          |      无损压缩      |

## Benchmark

使用如下两个图像数据集来对比压缩率。

|                  |                     小尺寸图像benchmark                      |                     大尺寸图像benchmark                      |
| :--------------: | :----------------------------------------------------------: | :----------------------------------------------------------: |
|    数据集来源    | [Kodak Lossless True Color Image Suite](https://r0k.us/graphics/kodak/) [10] | [该网站](http://imagecompression.info/test_images/) 里面的 Gray 8 bit |
|  压缩前特殊处理  |            原为RGB彩色图像，将它转为8-bit灰度图像            |                             无需                             |
| 压缩前的图像格式 |                          8-bit 灰度                          |                          8-bit 灰度                          |
| 压缩前的文件格式 |                             .pgm                             |                             .pgm                             |
| 压缩前的总数据量 |                        9437544 bytes                         |                       162870743 bytes                        |
|     图像数量     |                              24                              |                              15                              |
|       备注       |       我放到了本repo的 [img_kodak](./img_kodak) 目录里       |           数据量大，我没有放到本repo中，请自行下载           |

## 结果展示

对以上两种benchmark进行压缩，得到压缩率数据如下。

注：压缩率=压缩前大小/压缩后大小。越大越好

|                 格式                  |  小尺寸图像压缩率   |  大尺寸图像压缩率   |
| :-----------------------------------: | :-----------------: | :-----------------: |
|                  PNG                  |        1.725        |        2.202        |
|               JPEG2000                |        1.793        |        2.309        |
|                 WEBP                  |        1.847        |        2.356        |
|      FLIF (now part of JPEG-XL)       |        1.864        | **2.534** (**1st**) |
|                 CALIC                 | **1.913** (**1st**) |        2.458        |
|     JPEG-LS baseline (ITU-T T.87)     |        1.845        |        2.333        |
|    JPEG-LS extension (ITU-T T.870)    |        1.884        |        2.411        |
| **enhanced JPEG-LS extension (本库)** | **1.911** (**2nd**) | **2.470** (**2nd**) |

　

　

　

# 参考资料

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
