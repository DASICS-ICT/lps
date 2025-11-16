RUN_SH="./run-test.sh"
NAME=$1

echo "running baseline test: $NAME..."
$RUN_SH $NAME
echo "running dasics test: $NAME..."
LOADER=lps-run DASICS_FLAG=-dasics $RUN_SH $NAME