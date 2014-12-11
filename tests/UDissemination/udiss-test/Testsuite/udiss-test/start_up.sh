#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

dd if=/dev/urandom of=/tmp/data.30k bs=1024 count=30 >& /dev/null
dd if=/dev/urandom of=/tmp/data.60k bs=1024 count=60 >& /dev/null
dd if=/dev/urandom of=/tmp/data.90k bs=1024 count=90 >& /dev/null
dd if=/dev/urandom of=/tmp/data.120k bs=1024 count=120 >& /dev/null
dd if=/dev/urandom of=/tmp/data.150k bs=1024 count=150 >& /dev/null
dd if=/dev/urandom of=/tmp/data.180k bs=1024 count=180 >& /dev/null

sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
