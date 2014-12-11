#!/bin/bash
grep " W" benchmark-*.log | awk '{print $1;}' > dobj_delete_delays.dat
