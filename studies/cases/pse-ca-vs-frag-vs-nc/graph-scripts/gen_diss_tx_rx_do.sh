#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
rm -f tx_rx_do.data
echo 'dissemination "" "Rx" "" "" "Tx" "" "" "DRx" "" "" "# A2+A3" "" "" "# ALL" ""' >> tx_rx_do.data 
echo "frag" $(${DIR}/print_diss_btx_brx_dorx.sh frag) >> tx_rx_do.data 
echo "nc" $(${DIR}/print_diss_btx_brx_dorx.sh nc) >> tx_rx_do.data 
echo "ca" $(${DIR}/print_diss_btx_brx_dorx.sh ca) >> tx_rx_do.data 
gnuplot ${DIR}/diss-tx_rx_do.plot 
