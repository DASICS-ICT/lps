RUN_SH="./run-test.sh"
NAME=$1

echo "running wasm test: $NAME..."
LOADER="wasmtime --dir=." $RUN_SH $NAME
