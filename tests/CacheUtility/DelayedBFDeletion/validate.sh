#!/bin/bash


OUTPUT_FILE="test_output.n2"


if [ -x "./custom_validate.sh" ]; then
    bash "./custom_validate.sh" ${OUTPUT_FILE}
    if [ "$?" != "0" ]; then
	echo "custom errors detected"
        exit 1
    fi
else 
    echo "custom_validate.sh not executable!"
fi

exit 0
