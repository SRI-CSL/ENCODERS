#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

dd if=/dev/urandom of=/tmp/data-a1.10k bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b1.11k bs=1024 count=11 >& /dev/null

sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
