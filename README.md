
<h1>LPS</h1>

LPS (Lightweight Process Sandbox)

编译
```bash
meson setup build --prefix=$PWD/install --cross-file=$PWD/toolchains/riscv64-linux-gcc.meson -Dbuildtype=debug
cd build
ninja install
```