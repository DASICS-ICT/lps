编译

```bash
meson setup build-rv --prefix=$PWD/install -Dliblfi-only=true --cross-file=$PWD/cross/riscv64.txt -Dbuildtype=debug -Dliblfi-gdb=true
cd build-rv
ninja install
```