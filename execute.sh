#!/bin/bash -ev


pushd studies/framework/lxc/bin
./clean.sh ${1}
./generate.sh ${1}
./generate_test_list.sh ${1}
./run.sh ${1}
popd
