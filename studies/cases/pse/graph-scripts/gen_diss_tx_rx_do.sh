#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SECURITY=$1
rm -f tx_rx_do.data
echo 'dissemination "" "Rx" "" "" "Tx" "" "" "DRx" "" "" "# A2+A3" "" "" "# ALL" ""' >> tx_rx_do.data 
echo "NONE" $(${DIR}/print_diss_btx_brx_dorx.sh NONE_UDISS1_${SECURITY}) >> tx_rx_do.data 
echo "PR" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY_UDISS1_${SECURITY}) >> tx_rx_do.data 
echo "PR\nUD1" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY-UDISS_UDISS1_${SECURITY}) >> tx_rx_do.data 
echo "PR\nUD2" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY-UDISS_UDISS2_${SECURITY}) >> tx_rx_do.data 
echo "PR\nUC\nUD1" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY-UDISS-UCACHING_UDISS1_${SECURITY}) >> tx_rx_do.data 
echo "PR\nUC\nUD2" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY-UDISS-UCACHING_UDISS2_${SECURITY}) >> tx_rx_do.data 
echo "PR\nSC\nUD1" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY-UDISS-SCACHING_UDISS1_${SECURITY}) >> tx_rx_do.data 
echo "PR\nSC\nUD2" $(${DIR}/print_diss_btx_brx_dorx.sh PRIORITY-UDISS-SCACHING_UDISS2_${SECURITY}) >> tx_rx_do.data 
gnuplot ${DIR}/diss-tx_rx_do.plot 
