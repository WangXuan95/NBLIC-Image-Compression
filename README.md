 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# JPEG-LS extension

A implementation of JPEG-LS extension (ITU-T T.870) image encoder/decoder.

C 语言实现的 JPEG-LS extension 图像编码/解码器。可以获得很高的 8-bit 无损压缩率。

## Why ?

JPEG-LS 是一种无损/有损压缩标准。目前网上的 JPEG-LS 实现均为 JPEG-LS baseline (ITU-T T.87) ，而没有 JPEG-LS extension (ITU-T T.870) [1]。我按照 JPEG-LS extension 标准文档编写了一个实现，发现其 8-bit 无损压缩率略高于目前最先进的无损压缩标准 FLIF [5] (FLIF 现在是 JPEG-XL 的一部分)。

JPEG-LS extension 标准文档详见 [1]。本实现不完全遵循文档！！尤其是文件头完全我自己定义的。

　

# 开发进度

- [x] 灰度 8 bit 图像无损编码/解码：已充分测试，足够使用
- [x] 灰度 8 bit 图像有损编码/解码：已充分测试，足够使用
- [ ] 灰度 9-16 bit 图像无损编码/解码：初步支持，由于尚不符合标准而压缩率不够高，暂时不建议使用
- [ ] 灰度 9-16 bit 图像无损编码/解码：初步支持，由于尚不符合标准而压缩率不够高，暂时不建议使用
- [ ] 彩色图像编码/解码：尚未计划

　

# 代码说明

代码文件在目录 src 中。包括 2 个文件：

- `JLSx.c` : 实现了 JPEG-LS extension encoder/decoder
- `JLSxMain.c` : 包含 `main` 函数的文件，是调用 `JLSx.c` 的一个示例。

　

# 编译

### Windows (命令行)

如果你把 Visual Studio 里的 C 编译器 (`cl.exe`) 加入了环境变量，也可以用命令行 (CMD) 进行编译。在本目录里运行命令：

```bash
cl src\*.c /FeJLSx.exe /Ox
```

产生可执行文件 JLSx.exe 。这里我已经编译好。

### Linux (命令行)

在本目录里运行命令：

```bash
gcc src/*.c -o JLSx -O3 -Wall
```

产生可执行文件 JLSx 。这里我已编译好。

　

# 运行

### Windows (命令行)

用以下命令把图像文件 `1.pgm` 压缩为 `1.jlsxn`  。其中 `<near>` 值可以取非负整数。0 代表无损，≥1 代表有损，越大则压缩率越高，图像质量越差

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

　

# 压缩率评估

[img_kodak](./img_kodak) 目录里的24个文件是 kodak提供的24张图像 [3] 转化为 8-bit 灰度后的图像。我用各种图像压缩标准对他们进行无损压缩，总压缩率如下表。

|   格式   |  PNG  | JPEG2000 | WEBP  | FLIF  | JPEG-LS baseline | JPEG-LS extension |
| :------: | :---: | :------: | :---: | :---: | :--------------: | :---------------: |
| 总压缩率 | 1.714 |  1.793   | 1.847 | 1.864 |      1.845       |       1.884       |

其中：

- PNG 使用 Python 的 Pillow 库生成。选项为： `optimize=True` (优化文件大小)
- JPEG2000 使用 Python  的 Pillow 9.5.0 库生成。选项为： `irreversible=False, quality_mode='rates'`
- WEBP 使用 Python 的 Pillow 9.5.0 库生成。选项为：`lossless=True, quality=100, method=6` 。(无损，最高压缩率，最低性能)
- FLIF 使用 FLIF 官方实现生成 [5] 。选项为：`-N -E100` 。(non-interlace，最高压缩率，最低性能)
- JPEG-LS baseline 使用 Python 的 Pillow 9.5.0 库和 Pillow-jpls 插件生成。选项为：`near=0` (无损)
- JPEG-LS extension 使用本代码生成。选项为：`near=0` (无损)

Pillow 库的各选项说明见 [4]

　

# 参考资料

[1] JPEG-LS extension ITU-T T.870 : https://www.itu.int/rec/T-REC-T.870/en 

[2] PGM Image File Specification : https://netpbm.sourceforge.net/doc/pgm.html#index

[3] Kodak Lossless True Color Image Suite : https://r0k.us/graphics/kodak/

[4] Python Pillow Supported Image file formats : https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html

[5] FLIF: Free Lossless Image Format : https://github.com/FLIF-hub/FLIF
