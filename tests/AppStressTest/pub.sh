#!/bin/bash

for i in {0..99}; do
  echo haggletest pub_app_$i pub attr_$i
  haggletest pub_app_$i pub attr_$i >pub_app_$i.log &
  sleep 1
  i=$((i + 1))
done
