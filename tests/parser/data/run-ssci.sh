#! /bin/bash

. globals

SSCI_RESULT=ssci

if [ x$2 = x ]; then
    echo "Usage: $0 <test-spec> <nr>"
    echo "	e.g. $0 \"shine-48b < up-142 / light-93c\" 000"
    exit 1
fi

./run-test.sh "$1" "$2" "${TEST_GAME_DIR}" "${TEST_GAME_DIR}" "${SSCI_BIN}" "${SSCI_RESULT}"
