#!/bin/python

# requires BonnMotion to be installed and "bm" in the bash path!

from gen_data import output_bash_to_file_obj
import time
import os
import io
import sys
import json
from heapq import heappush, heappop
import re
import commands

# fixed params

# see gen_data.py for parameter description.
# sample test file
'''
[{
    "test_name"          : "sampleTest",
    "duration"           : 600,
    "warmup"             : 120,
    "num_pub"            : 30,
    "pub_freq_s"         : 10,
    "sub_s"              : 45,
    "N"                  : 10,
    "K"                  : 1,
    "config_path"        : "/tmp/config.xml",
    "pub_size_kb"        : 500,
    "seed"               : 1,
    "area_width"         : 1500,
    "area_height"        : 300,
    "bandwidth_bps"      : 54000000,
    "latency_ms"         : 20000,
    "pkt_error_rate"     : 0.5,
    "rwp_v_min"          : 1,
    "rwp_v_max"          : 7,
    "rwp_p_max"          : 5,
    "cpu_limit"          : 1.0,
    "test_output_path"   : "/tmp/testX",
    "result_output_path" : "/tmp/testX_results",
    "test_template"      : "/home/sam/Desktop/cbmen-encoders/tests/AlphaDIRECT++/MobileScripted/TestTemplate/"
}]
'''


def cp(in_file_name, out_file_name):
    replace_file(in_file_name, out_file_name)

def replace_file(in_file_name, out_file_name, replace_tuples=[]): 
    of = io.open(out_file_name, 'w')
    for line in io.open(in_file_name, 'r'):
        for (f, r) in replace_tuples:
            line = line.replace(f, str(r)) 
        of.write(line)
    of.close()

def now():
    return str(int(time.time() * 1000))

def output_bash_to_file(app_output_name, K, N, duration, warmup, num_pub, pub_file, pub_freq_s, sub_s):
    app = open(app_output_name, 'w')
    output_bash_to_file_obj(app, K, N, duration, warmup, num_pub, pub_file, pub_freq_s, sub_s)
    app.close()

def generate_imn_file(imn_file_name, N, area_width, area_height, bandwidth_bps, latency_ms, pkt_error_rate):
    of = open(imn_file_name, 'w')
    for i in range(1, N+1):
        of.write('node n%d {\n' % i)
        of.write('    type router\n')
        of.write('    model PC\n')
        of.write('    network-config {\n')
        of.write('\thostname n%d\n' % i)
        of.write('\t!\n')
        of.write('\tinterface eth0\n')
        of.write('\t ip address 10.0.0.%d/32\n' % (i + 19))
        of.write('\t ipv6 address 2001:0::%d/128\n' % (i + 19))
        of.write('\t!\n')
        of.write('    }\n')
        of.write('    canvas c1\n')
        of.write('    iconcoords {0.0 0.0}\n')
        of.write('    labelcoords {0.0 0.0}\n')
        of.write('    interface-peer {eth0 n%d}\n' % (N + 1))
        of.write('    services {DefaultRoute HaggleService}\n')
        of.write('}\n')
        of.write('\n')
    of.write('node n%d {\n' % (N + 1))
    of.write('    type wlan\n')
    of.write('    network-config {\n')
    of.write('\thostname wlan%d\n' % (N + 1))
    of.write('\t!\n')
    of.write('\tinterface wireless\n')
    of.write('\t ip address 10.0.0.0/32\n')
    of.write('\t ipv6 address 2001:0::0/128\n')
    of.write('\t!\n')
    of.write('\tscriptfile\n')
    of.write('\t%%scen_path%%\n')
    of.write('\t!\n')
    of.write('\tmobmodel\n')
    of.write('\tcoreapi\n')
    of.write('\tbasic_range\n')
    of.write('\t!\n')
    of.write('    }\n')
    of.write('    custom-config {\n')
    of.write('\tcustom-config-id basic_range\n')
    of.write('\tcustom-command {3 3 9 9 9}\n')
    of.write('\tconfig {\n')
    of.write('\t250\n')
    of.write('\t%d\n' % bandwidth_bps)
    of.write('\t0\n')
    of.write('\t%d\n' % latency_ms)
    of.write('\t%2.2f\n' % pkt_error_rate)
    of.write('\t}\n')
    of.write('    }\n')
    of.write('    canvas c1\n')
    of.write('    iconcoords {0.0 0.0}\n')
    of.write('    labelcoords {0.0 0.0}\n')
    for i in range(1, N+2):
        of.write('    interface-peer {e%d n%d}\n' % (i-1, i))
    of.write('}\n')
    for i in range(1, N+1):
        of.write('\nlink l%d {\n' % i)
        of.write('    nodes {n%d n%d}\n' % (N+1, i))
        of.write('}\n')
    of.write('\ncanvas c1 {\n')
    of.write('    name {Canvas1}\n')
    of.write('    refpt {0 0 0 0 0}\n')
    of.write('    scale {100}\n')
    of.write('    size {%d %d}\n' % (area_width, area_height))
    of.write('}\n')
    of.write('\n')
    of.write('option global {\n')
    of.write('    interface_names no\n')
    of.write('    ip_addresses yes\n')
    of.write('    ipv6_addresses yes\n')
    of.write('    node_labels yes\n')
    of.write('    link_labels yes\n')
    of.write('    ipsec_configs yes\n')
    of.write('    exec_errors yes\n')
    of.write('    show_api no\n')
    of.write('    background_images no\n')
    of.write('    annotations yes\n')
    of.write('    grid yes\n')
    of.write('    traffic_start 0\n')
    of.write('}\n')
    of.close()

def generate_scen_file(scen_output, N, duration, area_width, area_height, rwp_v_min, rwp_v_max, rwp_p_max, seed):
    beginning_delay = 3600
    cmd_str = "bm -f %s RandomWaypoint -d %d -i %d -n %d -x %d -y %d -R %d -h %2f -l %2f -p %2f -o 3" % (scen_output, duration, beginning_delay, N, area_width, area_height, seed, rwp_v_max, rwp_v_min, rwp_p_max)
    commands.getoutput(cmd_str)
    #subprocess.Popen(cmd_str, shell=True, executable="/bin/bash")
    #time.sleep(2)
    cmd_str = "bm Statistics -f %s -r %d" % (scen_output, 250)
    #subprocess.Popen(cmd_str, shell=True, executable="/bin/bash")
    commands.getoutput(cmd_str)
    cmd_str = "bm NSFile -f %s" % scen_output
    #subprocess.Popen(cmd_str, shell=True, executable="/bin/bash")
    commands.getoutput(cmd_str)
    #time.sleep(2)

    of = open(scen_output + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (N, duration, area_width, area_height))
    pos_array = []
    move_array = []
    comment_array = []
    unknown = 0
    p = re.compile("\$node_\((\d+)\)")
    for line in io.open(scen_output + '.ns_movements', 'r'):
        m = p.search(line)
        if m != None:
            node_num = int(m.group(1))
            line = p.sub("$node_(%d)" % (node_num + 1), line)
        if line.startswith("$node_("):
            pos_array = pos_array + [ line ]
        elif line.startswith("#"):
            comment_array = comment_array + [ line ]
        elif line.startswith("$ns_ at "):
            timestamp = float(line[len("$ns_ at "):line.find('"')])
            heappush(move_array, (timestamp, line))
        else:
            unknown = unknown + 1
    for pos in pos_array:
        of.write(pos)
    while len(move_array) > 0:
        line = heappop(move_array)
        of.write(line[1])
    of.close()
    
def mkdir(path_name):
    os.makedirs(path_name)

def chmodx(path_name):
    cmd_str = "chmod +x %s" % path_name
    commands.getoutput(cmd_str)
    #subprocess.Popen(cmd_str, shell=True, executable="/bin/bash")

def create_test(        \
    json_str,           \
    test_name,          \
    duration,           \
    warmup,             \
    num_pub,            \
    pub_freq_s,         \
    sub_s,              \
    N,                  \
    K,                  \
    config_path,        \
    pub_size_kb,        \
    seed,               \
    area_width,         \
    area_height,        \
    bandwidth_bps,      \
    latency_ms,         \
    pkt_error_rate,     \
    rwp_v_min,          \
    rwp_v_max,          \
    rwp_p_max,          \
    cpu_limit,          \
    test_output_path,   \
    result_output_path, \
    test_template):

    test_output_path = test_output_path + '/'
    mkdir(test_output_path)


    json_file = open(test_output_path + 'test.json', 'w')
    json_file.write(json_str)
    json_file.close()

    replace_file(test_template + 'echo_duration.sh', test_output_path + 'echo_duration.sh', [ ( '%%duration%%', duration + 50 ) ])
    chmodx(test_output_path + 'echo_duration.sh')
    replace_file(test_template + 'echo_output_path.sh', test_output_path + 'echo_output_path.sh', [ ( '%%output_path%%', result_output_path ) ])
    chmodx(test_output_path + 'echo_output_path.sh')

    pub_file='/tmp/'+now()

    replace_file(test_template + 'start_up.sh', test_output_path + 'start_up.sh', [ ('%%pub_file%%', pub_file), ('%%size_kb%%', pub_size_kb) ])
    chmodx(test_output_path + 'start_up.sh')
    replace_file(test_template + 'tear_down.sh', test_output_path + 'tear_down.sh', [ ('%%pub_file%%', pub_file) ])
    chmodx(test_output_path + 'tear_down.sh')
    replace_file(test_template + 'cpulimit.sh', test_output_path + 'cpulimit.sh', [ ('%%cpu_limit%%', cpu_limit) ])
    chmodx(test_output_path + 'cpulimit.sh')

    output_bash_to_file(test_output_path + 'app.sh', K, N, duration, warmup, num_pub, pub_file, pub_freq_s, sub_s)
    chmodx(test_output_path + 'app.sh')

    cp(test_template + 'echo_test_appname.sh', test_output_path + 'echo_test_appname.sh')
    chmodx(test_output_path + 'echo_test_appname.sh')
    cp(test_template + 'echo_testscenario.sh', test_output_path + 'echo_testscenario.sh')
    chmodx(test_output_path + 'echo_testscenario.sh')
    cp(test_template + 'validate.sh', test_output_path + 'validate.sh')
    chmodx(test_output_path + 'validate.sh')
    cp(config_path, test_output_path + 'config.xml')
    generate_scen_file(test_output_path + 'mobile', N, duration + 50, area_width, area_height, rwp_v_min, rwp_v_max, rwp_p_max, seed)
    generate_imn_file(test_output_path + 'mobile.imn.template', N, area_width, area_height, bandwidth_bps, latency_ms, pkt_error_rate)
    mkdir(test_output_path + "CoreService")
    replace_file(test_template + 'CoreService/haggle.py', test_output_path + 'CoreService/haggle.py', [ ('%%kill_soft%%', duration + 10), ('%%kill_hard%%', 30 ) ])
    cp(test_template + 'CoreService/__init__.py', test_output_path + 'CoreService/__init__.py')
    cp(test_template + 'CoreService/utils.py', test_output_path + 'CoreService/utils.py')
    print "Test was created: %s" % test_output_path

def main():

    if len(sys.argv) != 2:
        print "need testcase file"
        sys.exit(2)

    tests=json.loads(open(sys.argv[1]).read())

    for test in tests:
        create_test(json.dumps(test), **test)

if __name__ == "__main__":
    main()
