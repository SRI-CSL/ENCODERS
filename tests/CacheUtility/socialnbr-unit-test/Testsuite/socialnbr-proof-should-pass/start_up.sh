#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

dd if=/dev/urandom of=/tmp/data-a1.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-a2.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-a3.10K bs=1024 count=10 >& /dev/null

dd if=/dev/urandom of=/tmp/data-b1.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b2.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-b3.10K bs=1024 count=10 >& /dev/null

dd if=/dev/urandom of=/tmp/data-c1.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-c2.10K bs=1024 count=10 >& /dev/null
dd if=/dev/urandom of=/tmp/data-c3.10K bs=1024 count=10 >& /dev/null
sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
