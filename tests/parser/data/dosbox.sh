#! /bin/bash

. globals


${DOSBOX_BIN} -exit dir -c "mount s ${TEST_GAME_DIR}
s:
sciv.exe
exit"
