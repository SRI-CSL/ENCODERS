#!/bin/bash
grep " X" benchmark-*.log | awk '{print $1;}' > node_delete_delays.dat
