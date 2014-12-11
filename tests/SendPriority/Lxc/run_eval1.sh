#!/bin/bash

HIGH="1024000"
MEDIUM="1025024"
LOW="1026048"

rm -rf ./TestEvaluationFramework/Eval1/*
cd TestEvaluationFramework/TestGenerator
python test_generator.py send_priority_eval1.json
cd ../
ls -d Eval1/* > send_priority_eval1
./testrunner.sh send_priority_eval1 | tee /tmp/output
cd ../
OUTPUT_DIR="send_priority_eval1_results_$(date '+%s')"
mkdir ${OUTPUT_DIR}

rm -f /tmp/disabled-*
rm -f /tmp/enabled-*

i=0
mkdir ${OUTPUT_DIR}/disabled_delays
for disabled in $(cat /tmp/output | grep "Log files" | grep "eval-disabled" | awk '{print $5}'); do
    cat ${disabled}/apps_output | grep "Received," | grep ",${LOW}," | wc -l >> /tmp/disabled-LOW
    cat ${disabled}/apps_output | grep "Received," | grep ",${LOW}," | awk -F, '{printf "%d\n", 1000*$7}' | sort -n > ${OUTPUT_DIR}/disabled_delays/${LOW}.delays.${i}.processed
    cat ${disabled}/apps_output | grep "Received," | grep ",${MEDIUM}," | wc -l >> /tmp/disabled-MEDIUM
    cat ${disabled}/apps_output | grep "Received," | grep ",${MEDIUM}," | awk -F, '{printf "%d\n", 1000*$7}' | sort -n > ${OUTPUT_DIR}/disabled_delays/${MEDIUM}.delays.${i}.processed
    cat ${disabled}/apps_output | grep "Received," | grep ",${HIGH}," | wc -l >> /tmp/disabled-HIGH
    cat ${disabled}/apps_output | grep "Received," | grep ",${HIGH}," | awk -F, '{printf "%d\n", 1000*$7}' | sort -n > ${OUTPUT_DIR}/disabled_delays/${HIGH}.delays.${i}.processed
    i=$((i+1))
done

awk '{if(min==""){min=max=$1}; if($1>max) {max=$1}; if($1< min) {min=$1}; total+=$1; count+=1} END {print total/count, min, max}' /tmp/disabled-LOW  > ${OUTPUT_DIR}/disabled-LOW-avg-min-max
awk '{if(min==""){min=max=$1}; if($1>max) {max=$1}; if($1< min) {min=$1}; total+=$1; count+=1} END {print total/count, min, max}' /tmp/disabled-MEDIUM  > ${OUTPUT_DIR}/disabled-MEDIUM-avg-min-max
awk '{if(min==""){min=max=$1}; if($1>max) {max=$1}; if($1< min) {min=$1}; total+=$1; count+=1} END {print total/count, min, max}' /tmp/disabled-HIGH  > ${OUTPUT_DIR}/disabled-HIGH-avg-min-max

for f in $(cat ${OUTPUT_DIR}/disabled_delays/${LOW}*.processed); do
    A=""
    for d in $(ls ${OUTPUT_DIR}/disabled_delays/${LOW}*.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo "$f $A" >> ${OUTPUT_DIR}/disabled.LOW.delays.dat
done

for f in $(cat ${OUTPUT_DIR}/disabled_delays/${MEDIUM}*.processed); do
    A=""
    for d in $(ls ${OUTPUT_DIR}/disabled_delays/${MEDIUM}*.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo "$f $A" >> ${OUTPUT_DIR}/disabled.MEDIUM.delays.dat
done

for f in $(cat ${OUTPUT_DIR}/disabled_delays/${HIGH}*.processed); do
    A=""
    for d in $(ls ${OUTPUT_DIR}/disabled_delays/${HIGH}*.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo "$f $A" >> ${OUTPUT_DIR}/disabled.HIGH.delays.dat
done

i=0
mkdir ${OUTPUT_DIR}/enabled_delays
for enabled in $(cat /tmp/output | grep "Log files" | grep "eval-enabled" | awk '{print $5}'); do
    cat ${enabled}/apps_output | grep "Received," | grep ",${LOW}," | wc -l >> /tmp/enabled-LOW
    cat ${enabled}/apps_output | grep "Received," | grep ",${LOW}," | awk -F, '{printf "%d\n", 1000*$7}' | sort -n > ${OUTPUT_DIR}/enabled_delays/${LOW}.delays.${i}.processed
    cat ${enabled}/apps_output | grep "Received," | grep ",${MEDIUM}," | wc -l >> /tmp/enabled-MEDIUM
    cat ${enabled}/apps_output | grep "Received," | grep ",${MEDIUM}," | awk -F, '{printf "%d\n", 1000*$7}' | sort -n > ${OUTPUT_DIR}/enabled_delays/${MEDIUM}.delays.${i}.processed
    cat ${enabled}/apps_output | grep "Received," | grep ",${HIGH}," | wc -l >> /tmp/enabled-HIGH
    cat ${enabled}/apps_output | grep "Received," | grep ",${HIGH}," | awk -F, '{printf "%d\n", 1000*$7}' | sort -n > ${OUTPUT_DIR}/enabled_delays/${HIGH}.delays.${i}.processed
    i=$((i+1))
done

awk '{if(min==""){min=max=$1}; if($1>max) {max=$1}; if($1< min) {min=$1}; total+=$1; count+=1} END {print total/count, min, max}' /tmp/enabled-LOW  > ${OUTPUT_DIR}/enabled-LOW-avg-min-max
awk '{if(min==""){min=max=$1}; if($1>max) {max=$1}; if($1< min) {min=$1}; total+=$1; count+=1} END {print total/count, min, max}' /tmp/enabled-MEDIUM  > ${OUTPUT_DIR}/enabled-MEDIUM-avg-min-max
awk '{if(min==""){min=max=$1}; if($1>max) {max=$1}; if($1< min) {min=$1}; total+=$1; count+=1} END {print total/count, min, max}' /tmp/enabled-HIGH  > ${OUTPUT_DIR}/enabled-HIGH-avg-min-max

for f in $(cat ${OUTPUT_DIR}/enabled_delays/${LOW}*.processed); do
    A=""
    for d in $(ls ${OUTPUT_DIR}/enabled_delays/${LOW}*.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo "$f $A" >> ${OUTPUT_DIR}/enabled.LOW.delays.dat
done

for f in $(cat ${OUTPUT_DIR}/enabled_delays/${MEDIUM}*.processed); do
    A=""
    for d in $(ls ${OUTPUT_DIR}/enabled_delays/${MEDIUM}*.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo "$f $A" >> ${OUTPUT_DIR}/enabled.MEDIUM.delays.dat
done

for f in $(cat ${OUTPUT_DIR}/enabled_delays/${HIGH}*.processed); do
    A=""
    for d in $(ls ${OUTPUT_DIR}/enabled_delays/${HIGH}*.processed); do
        A="$A $(cat $d | awk -F, "{ if (\$1 <= $f) { print \$1 }}" | wc -l)"
    done
    A="$(echo $A | tr ' ' '\n' | awk -f stats.awk | awk -F, '{print $1;}')"
    echo "$f $A" >> ${OUTPUT_DIR}/enabled.HIGH.delays.dat
done

echo "P1 $(cat ${OUTPUT_DIR}/disabled-LOW-avg-min-max) $(cat ${OUTPUT_DIR}/enabled-LOW-avg-min-max)" >> ${OUTPUT_DIR}/delivered.dat
echo "P2 $(cat ${OUTPUT_DIR}/disabled-MEDIUM-avg-min-max) $(cat ${OUTPUT_DIR}/enabled-MEDIUM-avg-min-max)" >> ${OUTPUT_DIR}/delivered.dat
echo "P3 $(cat ${OUTPUT_DIR}/disabled-HIGH-avg-min-max) $(cat ${OUTPUT_DIR}/enabled-HIGH-avg-min-max)" >> ${OUTPUT_DIR}/delivered.dat

cp delivered.plot ${OUTPUT_DIR} 
cp delays.plot ${OUTPUT_DIR}
cp delays_low.plot ${OUTPUT_DIR}
cp delays_medium.plot ${OUTPUT_DIR}
cp delays_high.plot ${OUTPUT_DIR}
cd ${OUTPUT_DIR}
gnuplot delivered.plot
gnuplot delays.plot
gnuplot delays_low.plot
gnuplot delays_medium.plot
gnuplot delays_high.plot
cd ../ 
