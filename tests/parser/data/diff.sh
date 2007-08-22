#! /bin/bash

NR=$1

. globals

FALSE_POSITIVE=${STATS_DIR}/false-positive.${NR}
FALSE_NEGATIVE=${STATS_DIR}/false-negative.${NR}
INDEX_FILE=${GEN_DIR}/index.${NR}
SPEC_FILE=${GEN_DIR}/spec.${NR}
DEST_FILE=PARSE.${NR}
COUNT_FILE=${STATS_DIR}/count.${NR}

SSCI_FILE=${SSCI_RESULT}/${DEST_FILE}
FSCI_FILE=${FSCI_RESULT}/${DEST_FILE}

TESTCASE=`cat ${SPEC_FILE}` || (echo "Invalid testcase number"; exit 1)

if [ x${NR} = x ]; then
    echo "Usage: $0 <nr>"
    exit 1
fi

rm -f ${FALSE_POSITIVE}
touch ${FALSE_POSITIVE}
rm -f ${FALSE_NEGATIVE}
touch ${FALSE_NEGATIVE}

TAB_SSCI=${TMP_DIR}/ssci.tab
TAB_FSCI=${TMP_DIR}/fsci.tab

cat ${SSCI_FILE} | sed 's/\(.\)/\1\n/g' | awk '{printf "%d\t%d\n", (n++), $1}' > ${TAB_SSCI}
cat ${FSCI_FILE} | sed 's/\(.\)/\1\n/g' | awk '{printf "%d\t%d\n", (n++), $1}' > ${TAB_FSCI}

TAB_JOIN=${TMP_DIR}/joined.tab
TAB_ALL=${TMP_DIR}/all.tab
MARKED_INDEX=${TMP_DIR}/marked-index.tab

cat ${INDEX_FILE} | tr ' ' '*' > ${MARKED_INDEX}
join ${TAB_SSCI} ${TAB_FSCI} > ${TAB_JOIN}
join ${TAB_JOIN} ${MARKED_INDEX} > ${TAB_ALL}

COUNT=0
FP_COUNT=0
FN_COUNT=0

for n in `cat ${TAB_ALL} | tr ' ' '_'`; do
    arr=(`echo $n | tr '_' ' '`)
    idx=${arr[0]}
    ssci=${arr[1]}
    fsci=${arr[2]}
    text=`echo ${arr[3]} | tr '*' ' '`

    COUNT=$(( COUNT + 1 ))

    if [ $ssci == 0 ]; then
	if [ $fsci == 1 ]; then
	    FP_COUNT=$(( FP_COUNT + 1 ))
	    echo $text >> ${FALSE_POSITIVE}
	fi
    else
	if [ $fsci == 0 ]; then
	    FN_COUNT=$(( FN_COUNT + 1 ))
	    echo $text >> ${FALSE_NEGATIVE}
	fi
    fi
done

if [ $((FP_COUNT + FN_COUNT)) != 0 ]; then
    echo " * Spec '${TESTCASE}' has $((FP_COUNT + FN_COUNT)) error(s):"

    if [ ${FP_COUNT} != 0 ]; then
	echo "	- ${FP_COUNT} incorrect matches:"
	cat ${FALSE_POSITIVE} | awk '{print "\t\t" $0}'
    fi

    if [ ${FN_COUNT} != 0 ]; then
	echo "	- ${FN_COUNT} missed matches:"
	cat ${FALSE_NEGATIVE} | awk '{print "\t\t" $0}'
    fi
fi

echo ${COUNT} > ${COUNT_FILE}
