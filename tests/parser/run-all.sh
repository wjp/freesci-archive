#! /bin/bash

. config

DATA_DIR=data
TEST_SPECS=test-specs


RUN_SSCI=run-ssci.sh
RUN_FSCI=run-fsci.sh
RUN_DIFF=diff.sh

STATS_DIR=${DATA_DIR}/stats

WITH_SSCI=yes

while [ x$1 != x ]; do
    case $1 in
	-h) echo "$0: Supported paramters:"
	    echo "  --fast	: Disable re-computation of SSCI results"
	    ;;
	--fast)
	    WITH_SSCI=no
	    echo "Disabling SSCI re-computation."
	    ;;
    esac
    shift
done


COUNT=0
TESTS_TOTAL=0
FP_TOTAL=0
FN_TOTAL=0

for w in `cat ${TEST_SPECS} | tr ' ' '_'`; do
    INDEX=`printf "%03x" ${COUNT}`
    SPEC=`echo $w | tr '_' ' '`

    if [ x${VERBOSE} != x ] ; then
	echo "--- Spec #${INDEX} = ${SPEC}"
    fi

    COUNT=$(( COUNT + 1 ))
    (cd ${DATA_DIR}
	if [ x${WITH_SSCI} != xno ]; then
	    ./${RUN_SSCI} "${SPEC}" ${INDEX}
	fi
	./${RUN_FSCI} "${SPEC}" ${INDEX}
	./${RUN_DIFF} ${INDEX}
    )

    FP_FILE=${STATS_DIR}/false-positive.${INDEX}
    FN_FILE=${STATS_DIR}/false-negative.${INDEX}
    COUNT_FILE=${STATS_DIR}/count.${INDEX}

    LOCAL_TESTS=`cat ${COUNT_FILE}`
    LOCAL_FP=`wc -l ${FP_FILE} | awk '{print $1}'`
    LOCAL_FN=`wc -l ${FN_FILE} | awk '{print $1}'`

    TESTS_TOTAL=$(( TESTS_TOTAL + LOCAL_TESTS ))
    FP_TOTAL=$(( FP_TOTAL + LOCAL_FP ))
    FN_TOTAL=$(( FN_TOTAL + LOCAL_FN ))
done

echo "Tested ${COUNT} specs, for a total of ${TESTS_TOTAL} test cases."
echo "False Positives:	${FP_TOTAL}"
echo "False Negatives:	${FN_TOTAL}"

