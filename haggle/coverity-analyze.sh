#!/bin/bash
rm -rf ../cov-int
make clean
cov-build --dir ../cov-int make
