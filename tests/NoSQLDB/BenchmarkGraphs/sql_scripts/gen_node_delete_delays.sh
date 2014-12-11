#!/bin/bash
grep "Node .* deleted suc"  haggle.log  | awk '{print $8;}' | grep -v "us" > node_delete_delays.dat
