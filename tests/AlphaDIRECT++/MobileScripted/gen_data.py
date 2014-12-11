#!/bin/python

# A python script that generates a bash script, for core.
# "N" is the number of nodes.
# "duration" is the duratio of the entire scenario.
# "warmup" is the sleep time before any apps register.
# "num_pub" is the number of data objects published by each node.
# 
# The app works as follows:
# Every "pub_freq_s" each node publishes a DO "pub_file"
# with their node name as the only attribute. 
# Every "sub_s" each node clears all their subscriptions and
# subscribes to "K" different node names. 
from random import choice
import sys
from time import time

def get_subs(current, k, n, m):
    options = range(1, m) + range(m+1, n+1)
    options = list(set(options) - set(current))
    subs = []
    while k > 0:
        new_sub = choice(options)
        subs = subs + [new_sub]
        options = list(set(options) - set([new_sub]))
        k = k - 1
    return subs


def output_bash_to_file_obj(fo = sys.stdout, K=1, N=20, duration=300, warmup=120, num_pub=14, pub_file="/tmp/500KB", pub_freq_s=10, sub_s=45):
    T=duration-warmup

    now_ms = int(time()*1000)
    tmp_output_file="/tmp/test_output.%d" % now_ms
    tmp_if_output_file="/tmp/test_if_output.%d" % now_ms

    fo.write("#!/bin/bash\n")
    fo.write("TFILE=\"/tmp/$1_temp_file\"\n")
    fo.write("rm -f ${TFILE}\n")
    fo.write("sleep %d\n" % warmup)
    fo.write("if [ $1 == \"n0\" ]; then\n")
    fo.write("    echo \"bogus node\"\n")
    for i in range(1, N+1):
        app_count = 0
        time_elapsed=0
        fo.write("elif [ $1 == \"n%d\" ]; then\n" % i)
        output_files = []
        pub_output_file =  tmp_output_file + ".n%d" % i
        output_files = output_files + [pub_output_file]
        fo.write("    (haggletest app%d -f %s -b %d -p %d pub file %s n%d) &\n" % (app_count, pub_output_file, num_pub, pub_freq_s, pub_file, i))
        current = []
        while (time_elapsed + sub_s) < T:
            my_outputfile = tmp_output_file + ".n%d.%d" % (i, app_count)
            output_files = output_files + [my_outputfile]
            current = get_subs(current, K, N, i) 
            current_s = " ".join(map(lambda x: "n" + str(x), current))
            app_count += 1
            fo.write("    haggletest app%d -f %s -s %d -c sub %s\n" % (app_count, my_outputfile, sub_s, current_s))
            time_elapsed += sub_s
        for f in output_files:
            fo.write("    echo \"%s\" >> ${TFILE}\n" % f)
            fo.write("    cat %s >> ${TFILE}\n" % f)
        fo.write("    /sbin/ifconfig eth0 >> ${TFILE}\n")
    fo.write("else\n")
    fo.write("    exit 0\n")
    fo.write("fi\n")
    fo.write("cat ${TFILE} >> $2\n")

def main():
    output_bash_to_file_obj()

if __name__ == "__main__":
    main()

