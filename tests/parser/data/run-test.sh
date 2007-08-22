#! /bin/bash
# Runs a single test and saves the result files.

#Binaries:

. globals

TESTCASE=$1
NR=$2
DEST_DIR=$3
SCI_WORK_DIR=$4
RUNNER=$5
OUTPUT_DIR=$6


ASM_HEAD=parsetest.s.head
ASM_BODY=parsetest.s.body
SHUFFLE_SRC=shuffle.sml


# Temp and aux files
ASM_FILE=${GEN_DIR}/test.${NR}.s
INDEX_FILE=${GEN_DIR}/index.${NR}
SPEC_FILE=${GEN_DIR}/spec.${NR}
SHUFFLE_FILE=${TMP_DIR}/shuffle.${NR}.sml
DEST_FILE=PARSE.${NR}


echo ${TESTCASE} > ${SPEC_FILE}

if [ x${OUTPUT_DIR} = x ]; then
    echo "Usage: $n0 <testcase-spec> <nr> <dest-dir> <sci-work-dir> <runner> <ssci|fsci>"
    echo "	testcase-spec:  E.g. \"look-474 / room-804\""
    echo "	nr: PARSE.<nr> will be produced"
    echo "	dest-dir: Installation directory of SCI resource files"
    echo "	sci-work-dir: Directory into which the SCI interpreter will put its result files"
    echo "	runner: Program to invoke for generating the result file"
    exit 1;
fi

SPEC=`echo ${TESTCASE} | sed 's/\b[^- ]*-/\$/g'`
SPEC="${SPEC} !"

WORDS=`echo ${TESTCASE} | sed 's/\b\([^- ]*\)-[^\B ]*/\1/g' | tr '<>()[]/,&#' '          ' | tr ' ' '\n' | awk '{if ($0) print}'`

(
    echo "val (_::words) = [ \"\""
    for w in ${WORDS}; do echo "	,\"${w}\" "; done
    echo "];"
    cat ${SHUFFLE_SRC}
) > ${SHUFFLE_FILE}

${SMLNJ} ${SHUFFLE_FILE} | awk '/@@START/ {ok=1} /@@END/ {ok=0} {if (ok > 2) {print (nr++) "\t" $0}; if (ok) ok++}' > ${INDEX_FILE}

(
    cat ${ASM_HEAD};
    cat ${INDEX_FILE} | awk '{	print "\n\n\t\tldi\t&PARSESTRING"$1"\n\t\tpushi\t0\n\t\tcall\t&TESTSAID\t0\n\n"	}'
    cat ${ASM_BODY};
    echo "";
    echo ".said";
    echo "SPEC0:	${SPEC}"
    echo ""
    echo ".strings"
    echo "FILENAME:	\"${DEST_FILE}\""
    cat ${INDEX_FILE} | awk '{	psn=$1; $1=""; print "PARSESTRING"psn"\t: \""substr($0,2)"\"\n"	}'
) > ${ASM_FILE}

${SCIS} -q ${ASM_FILE} -o ${DEST_DIR}/script.000
(${RUNNER}) > ${TMP_DIR}/log 2> ${TMP_DIR}/log
mv ${SCI_WORK_DIR}/${DEST_FILE} ${OUTPUT_DIR}/
