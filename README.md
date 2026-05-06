
# LPS

LPS (Lightweight Process Sandbox)

## 简介

## 快速开始

下载release包，解压

```
.
├── bits
│   ├── vcu128_dev_dualcore_utimer.bit
│   └── vcu128_dev_utimer.bit
├── images
│   ├── qemu.bin
│   ├── vcu128.bin
│   └── vcu128-dualcore.bin
├── qemu-bin
│   └── qemu-system-riscv64-utimer
└── run-rootfs.sh
```

`bits`目录下存放硬件bits

`images`目录下分别存放QEMU,单核/双核上版二进制文件

运行QEMU测试:

```bash
./run-rootfs.sh images/qemu.bin
```

根目录下：`spec-run-all.sh`自动测试spec（注意：此测试运行时间较长）

`microbenchmark`目录下：运行`./scripts/name.sh`

`mmbenchmark/dav1d`目录下：运行`./run-dav1d.sh`

`mmbenchmark/security/mini`目录下：运行`./run_test.sh /usr/bin/lps-security`

## 前置条件

- 确保你拥有任意发行版的clang (用于编译运行时)
- 下载riscv64-linux-musl-gcc编译工具，[参考链接](https://toolchains.bootlin.com/downloads/releases/toolchains/riscv64-lp64d/tarballs/riscv64-lp64d--musl--stable-2025.08-1.tar.xz)，放入PATH路径 (用于编译C/C++负载)
- 下载wasm编译工具，[参考链接](https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-30/wasi-sdk-30.0-x86_64-linux.tar.gz) （用于编译Wasm负载）

## LPS运行时编译
```bash
meson setup build --prefix=$PWD/install --cross-file=$PWD/toolchains/riscv64-linux-clang.meson -Dbuildtype=release
cd build
ninja install
```

可执行文件位于install/bin目录下

## 运行负载编译

###  Microbench

配置好riscv64-linux-musl-gcc工具，根据工具名称修改`micorbenchmark/Makefile`中的`CC`值，在`microbenchmark`路径下执行`make`即可。

### SPEC CPU

前提：准备好SPEC CPU 2006的源代码和测试数据。

初始化子仓库`macrobenchmark/CPU2006LiteWrapper`

```bash
git submodule update --init
```

随后按照子仓库指引初始化SPEC CPU配置（SPEC CPU 2006代码/数据拷贝）

#### Native

在`macrobenchmark/CPU2006LiteWrapper`路径下

```bash
make ARCH=riscv64 CROSS_COMPILE=riscv64-linux- build-int -j `nproc`
```
随后可以拷贝可执行文件制作rootfs等。

#### Wasm

在`macrobenchmark/CPU2006LiteWrapper`路径下

切换分支至`wasm`

```bash
git checkout wasm
```

在编译前先clean目录
```bash
make clean-int-build
```

编译
```bash
make build-int -j `nproc`
```
随后可以拷贝可执行文件制作rootfs等。