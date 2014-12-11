#!/bin/sh
grep Up1 ./haggle.log | awk -F: '{ print $3}' | awk -F, 'BEGIN {cnt = -5} {cnt=cnt+5; print  cnt $1}' > $1
grep Down1 ./haggle.log | awk -F: '{ print $3}' | awk -F, 'BEGIN {cnt = 245 } {cnt=cnt+5; print cnt $1}' >> $1
grep Up2 ./haggle.log  | awk -F: '{ print $3}' | awk -F, 'BEGIN {cnt = 495} {cnt=cnt+5; print cnt $1}' >> $1
grep Down2 ./haggle.log | awk -F: '{ print $3}' | awk -F, 'BEGIN {cnt = 745} {cnt=cnt+5; print cnt $1}' >> $1
