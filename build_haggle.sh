#!/bin/bash -ev

pushd haggle
./dependencies_ubuntu.sh
popd
pushd charm
./build_ubuntu.sh
popd
pushd haggle
./build_ubuntu.sh
popd
