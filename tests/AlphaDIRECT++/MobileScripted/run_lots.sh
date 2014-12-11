#!/bin/bash

sudo su

for i in {1..3}; do
    date
    echo "Run: $i"
    ./testrunner.sh
    ts=$(date +%s)
    mkdir -p ~/Desktop/Run/${i}-${ts}/
    mv /tmp/*sh_logs ~/Desktop/Run/${i}-${ts}/
done
