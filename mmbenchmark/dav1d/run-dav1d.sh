#!/bin/ash

LPS_GO="lps-go"
BINDIR="bin"
INDIR="in"
DAV1D="$BINDIR/dav1d"

if [ ! -d "$INDIR" ]; then
    echo "Error: input directory not found at $INDIR"
    exit 1
fi

echo "=== dav1d Video Decoding Benchmark ==="
echo ""

for INPUT in $INDIR/*; do
    [ -f "$INPUT" ] || continue

    FILENAME=$(basename "$INPUT")
    echo "Testing: $FILENAME"
    echo "----------------------------------------"

    echo "[LPS] Running dav1d with LPS..."
    $LPS_GO $DAV1D -i $INPUT -o /dev/null -dasics
    if [ $? -ne 0 ]; then
        echo "✗ LPS test failed"
        continue
    fi
    echo ""

    echo "[Native] Running dav1d natively..."
    $DAV1D -i $INPUT -o /dev/null
    if [ $? -ne 0 ]; then
        echo "✗ Native test failed"
        continue
    fi
    echo ""
    echo "----------------------------------------"
    echo ""
done

echo "✓ dav1d benchmark completed!"
exit 0
