#
#get used bytes

start=`grep "Used bytes:" memory.results-Start | awk -F, '{print $1}' | awk -F: '{print $2}'`

#all use 'used bytes' for consistency
grep "Used bytes:" memory.results-Up1 | awk -F, '{print $1}' | awk -F: '{print $2}' > up1.data
grep "Used bytes:" memory.results-Up2 | awk -F, '{print $1}' | awk -F: '{print $2}' > up2.data
echo "0, $start" > plot.info

grep "Free bytes:" memory.results-Down1 | awk -F, '{print $1}' | awk -F: '{print $2}' > down1.data
grep "Free bytes:" memory.results-Down2 | awk -F, '{print $1}' | awk -F: '{print $2}' > down2.data

cat up1.data | awk '{sum +=1; print sum, $1}' >> plot.info
cat down1.data | awk '{sum +=1; jim=50+sum; print jim, $1}' >> plot.info
cat up2.data | awk '{sum +=1; jim=100+sum; print jim, $1}' >> plot.info
cat down2.data | awk '{sum +=1; jim=150+sum; print jim, $1}' >> plot.info
