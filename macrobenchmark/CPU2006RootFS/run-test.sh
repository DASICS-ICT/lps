RUN_DIR=$1


if [ ! -d "$RUN_DIR" ]; then
    echo "Error: Directory $RUN_DIR does not exist."
    exit 1
fi

export DASICS_FLAG
RUN_SH="run-test.sh"
TIME_LOG="$RUN_SH$DASICS_FLAG.timelog"
WORKLOAD="./$RUN_DIR"

cd $RUN_DIR
if [ -e "$TIME_LOG" ]; then
    rm $TIME_LOG
fi

export TIME="%U user %S system %E elapsed %P CPU (%X text + %D data %M max)k | %I inputs + %O outputs (%F major + %R minor)pagefaults %W swaps | %e # elapsed in second"
export APP="time -a -o $TIME_LOG $LOADER $WORKLOAD"
sh $RUN_SH