#! /bin/bash

#TEST_GAME_DIR="/home/creichen/work/port/sci/dumb"
#FSCI_DATA_DIR="/home/creichen/.freesci/said"

. globals


if [ x$2 = x ]; then
    echo "Usage: $0 <test-spec> <nr>"
    echo "	e.g. $0 \"shine-48b < up-142 / light-93c\" 000"
    exit 1
fi

./run-test.sh "$1" "$2" "${TEST_GAME_DIR}" "${FSCI_DATA_DIR}" "${FSCI_BIN}" "${FSCI_RESULT}"
