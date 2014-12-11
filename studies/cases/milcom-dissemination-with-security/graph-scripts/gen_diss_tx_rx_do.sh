#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
rm -f tx_rx_do.data
echo 'dissemination "" "Rx" "" "" "Tx" "" "" "DRx" "" "" "# DO" ""' >> tx_rx_do.data 
echo "ID" $(${DIR}/print_diss_btx_brx_dorx.sh ID) >> tx_rx_do.data 
echo "MD" $(${DIR}/print_diss_btx_brx_dorx.sh MD) >> tx_rx_do.data 
echo "PR" $(${DIR}/print_diss_btx_brx_dorx.sh PR) >> tx_rx_do.data 
echo "NONE" $(${DIR}/print_diss_btx_brx_dorx.sh NONE) >> tx_rx_do.data 
gnuplot ${DIR}/diss-tx_rx_do.plot 
