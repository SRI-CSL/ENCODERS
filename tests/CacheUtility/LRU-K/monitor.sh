#!/bin/bash

./haggletest -w 1 sub start
./haggletest -b 1 pub response  

sleep 15
(./haggletest  -w 1 sub first="file")

