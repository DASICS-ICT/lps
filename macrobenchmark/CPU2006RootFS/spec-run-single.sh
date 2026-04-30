#!/bin/sh

# input a dir path to the test
# env: DASICS=1 run native bin with -dasics(DASICS_FLAG)
#      WASM=1   run wasm bin
#      else     run native bin

RUN_DIR=$1

if [ ! -d "$RUN_DIR" ]; then
    echo "Error: Directory $RUN_DIR does not exist."
    exit 1
fi

WORKLOAD="./$RUN_DIR"

if [ -n "$LPS" ]; then
    export DASICS_FLAG="-dasics"
    LOG_FLAG="-dasics"
    LOADER="lps-go"
fi

if [ -n "$WASM" ]; then
    LOG_FLAG="-wasm"
    WORKLOAD="$WORKLOAD.wasm"
    LOADER="wasmtime --dir=."
fi

RUN_SH="run-train.sh"

TIME_LOG="$RUN_SH$LOG_FLAG.timelog"

cd $RUN_DIR
if [ -e "$TIME_LOG" ]; then
    rm $TIME_LOG
fi

export TIME="%U user %S system %E elapsed %P CPU (%X text + %D data %M max)k | %I inputs + %O outputs (%F major + %R minor)pagefaults %W swaps | %e # elapsed in second"

export APP="time -a -o $TIME_LOG $LOADER $WORKLOAD"

sh $RUN_SH
