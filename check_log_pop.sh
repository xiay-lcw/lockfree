#!/bin/bash

FILE=$1
THREAD_COUNT=$2
SORT=$3

if [ -z "${FILE}" ]; then
    FILE=log
fi

if [ -z "${MAX_INDEX}" ]; then
    THREAD_COUNT=8
fi

if [ -z "${SORT}" ]; then
    SORT=true
fi

MAX_INDEX=$(( ${THREAD_COUNT}-1 ))

check_thread()
{
    echo "Checking thread $1"
    if ${SORT} ; then
        cat ${FILE} | grep "^$1\." | sed -e "s/^$1\.//" | sort | awk 'NR==1{p=$1;next} { if ($1-p != 1) print p, $1, $1-p; p=$1}'
    else
        cat ${FILE} | grep "^$1\." | sed -e "s/^$1\.//" | awk 'NR==1{p=$1;next} { if ($1-p != 1) print p, $1, $1-p; p=$1}'
    fi
}

for i in `seq 0 ${MAX_INDEX}`; do
    check_thread $i
done
