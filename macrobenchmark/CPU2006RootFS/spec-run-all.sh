#!/bin/sh

get_spec_int() {
    echo "
    401.bzip2
    429.mcf
    445.gobmk
    456.hmmer
    458.sjeng
    462.libquantum
    464.h264ref
    473.astar
    "
}

echo Running Native SPEC...
for name in $(get_spec_int); do
    echo "running $name..."
    ./spec-run-single.sh "$name"
done
echo Native SPEC done.

echo Running LPS SPEC...
for name in $(get_spec_int); do
    echo "running $name..."
    LPS=1 ./spec-run-single.sh "$name"
done
echo LPS SPEC done.

echo Running WASM SPEC...
for name in $(get_spec_int); do
    echo "running $name..."
    WASM=1 ./spec-run-single.sh "$name"
done
echo WASM SPEC done.

