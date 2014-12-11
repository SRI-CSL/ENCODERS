#!/bin/bash

for i in {0..99}; do
  echo haggletest sub_app_$i sub attr_$i
  haggletest -w 1 sub_app_$i sub attr_$i >sub_app_$i.log &
  sleep 1
  i=$((i + 1))
done
