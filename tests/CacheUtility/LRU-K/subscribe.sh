#!/bin/bash

./haggletest -w 1 sub start
./haggletest -b 1 pub response
  
(./haggletest -w 18 sub key="value")

