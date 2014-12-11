#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
GRAPH_NAME="$(basename $(pwd)).eps"

$DIR/rename_log_dirs.sh
$DIR/gen_diss_tx_rx_do.sh
mv dissemination-tx_rx_do.eps $GRAPH_NAME
