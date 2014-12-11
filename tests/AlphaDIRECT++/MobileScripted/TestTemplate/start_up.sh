#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

dd if=/dev/zero of=%%pub_file%% bs=1024 count=%%size_kb%%

sed "s:%%scen_path%%:${DIR}/mobile.scen:" ${DIR}/mobile.imn.template > ${DIR}/mobile.imn

exit 0
