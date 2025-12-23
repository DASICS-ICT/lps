
<h1>LPS</h1>

[![MPL License](https://img.shields.io/badge/license-MPL%202.0-blue)](https://github.com/zyedidia/lfi/blob/master/LICENSE)

LPS (Lightweight Process Sandbox)

编译
```bash
meson setup build --prefix=$PWD/install --cross-file=$PWD/toolchains/riscv64-linux-gcc.meson -Dbuildtype=debug
cd build
ninja install
```