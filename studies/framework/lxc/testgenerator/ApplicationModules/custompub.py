#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated and GPC
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Joshua Joy (JJ)
#   Hasnain Lakhani (HL)




from heapq import heappush, heappop
from time import time
import random
import copy

MAX_END_DELAY_S=1 # subcribes must end at least this many seconds 
                  # before app duration

def output_bash_to_file_obj(fo, N, duration, script, seed):
    randomInstance = random.Random()
    randomInstance.seed(seed)
    

    # pre-process random pubs/subs pairs
    new_script = []
    options = range(1, N+1)
    pair_num = 0
    for d in script:
        if d['type'] == 'random-pubsub-pair':
            try: 
                mutual_exclusive = d['mutual_exclusive'] == "true"
                del d['mutual_exclusive']
            except: 
                mutual_exclusive = False
            try:
                number = d['number']
                del d['number']
            except:
                number = 1
            for i in range(0, number):
                n1_pub_copy = copy.deepcopy(d)
                n1 = randomInstance.choice(options)
                if mutual_exclusive:
                    options = list(set(options) - set([n1]))
                    n2 = randomInstance.choice(options)
                else: 
                    # make sure sub is different than pub
                    n2 = randomInstance.choice(list(set(options) - set([n1])))
                n2_sub_copy = copy.deepcopy(d)
                if mutual_exclusive:
                    options = list(set(options) - set([n2]))
                pair_num += 1
                n1_pub_copy['type'] = 'pub'
                n1_pub_copy['node_num'] = n1
                n1_pub_copy["attributes"] = {
                    'random-pubsub-pair-id' : pair_num
                }
                n2_sub_copy['type'] = 'sub'
                n2_sub_copy['node_num'] = n2
                n2_sub_copy['attributes'] = {
                    'random-pubsub-pair-id' : pair_num
                }
                new_script += [ n1_pub_copy, n2_sub_copy ]
        else:
            new_script += [ d ]

    script = new_script

    # replace random nodes w/ actual nodes
    for d in script:
        node_num = d['node_num']
        try:
            if "rand" == str(node_num):
                rand_min = 1
                try:
                    rand_min = d['rand_min']
                except:
                    None
                rand_max = N
                try:
                    rand_max = d['rand_max']
                except:
                    None
                d['node_num'] = randomInstance.choice(range(rand_min, rand_max+1))
        except:
            new_script = [ d ]

    new_script = []

    # unroll pause/rate for pubs
    for d in script:
        if d['type'] == 'pub':
            try: 
                num_pub = d['num_pub']
            except:
                new_script += [ d ]
                continue
            if num_pub <= 1:
                new_script += [ d ]
                continue
            actual_pause = 0
            try: 
                actual_pause = float(d['pause'])
            except: 
                None
            try: 
                actual_pause = 1/float(d['rate'])
            except:
                None
            current_time = d['time']
            # we unroll the pause/rates to individual pubs to integrate w/ the single
            # haggletest pub script
            for i in range(num_pub):
                d_copy = copy.deepcopy(d)
                d_copy['time'] = current_time
                d_copy['num_pub'] = 1
                try: 
                    del d_copy['pause']
                    del d_copy['rate']
                except:
                    None
                new_script += [ d_copy ]
                current_time += actual_pause
        else:
            new_script += [ d ]

    script = new_script

    nodes = {}

    # pre-process random nodes
    for d in script:
        node_num = d['node_num']
        try:
            h = nodes[node_num]
        except:
            h = []
        heappush(h, (d['time'], d))
        nodes[node_num] = h

    now_ms = int(time()*1000)
    tmp_output_file="/tmp/test_output.%d" % now_ms

    fo.write("#!/bin/bash\n")
    fo.write("TFILE=\"$(mktemp)\"\n")
    fo.write("NOW=\"$(date '+%s')\"\n")
    fo.write("NODE=\"$1\"\n")
    fo.write("APPS_OUTPUT=\"$2\"\n")
    fo.write("FAILLOG=\"$3\"\n")
    fo.write("rm -f ${TFILE}\n")
    fo.write("if [ \"${NODE}\" == \"n0\" ]; then\n")
    fo.write("    echo \"bogus node\"\n")
    for i in range(1, N+1):
        app_count = 0
        fo.write("elif [ \"${NODE}\" == \"n%d\" ]; then\n" % i)
        try: 
            node_script = nodes[i]
        except:
            fo.write("    sleep %.2f\n" % duration)
            my_outputfile = tmp_output_file + ".n%d.%d" % (i, app_count)
            fo.write("    echo \"%s\" >> ${TFILE}\n" % my_outputfile)
            fo.write("    /sbin/ifconfig eth0 >> ${TFILE}\n")
            continue
        cmds = []
        sleeps = []
        prev_time = 0
        last_delay = 0
        my_outputfile_prefix = tmp_output_file + ".n%d." % (i,)
        while len(node_script) > 0:
            action = heappop(node_script)[1]
            action_type = action['type']
            action_time = action['time']
            if action_time > (duration - MAX_END_DELAY_S):
                continue
            my_outputfile = my_outputfile_prefix + "%d" % (app_count,)
            action_attributes = action['attributes']
            # we allow both [ ["k", "v" ] ], and { "k" : "v" } forms
            if isinstance(action_attributes, dict):
                action_attributes = action_attributes.items()
            action_attributes_string = " ".join(map(lambda(x,y) : str(x) + "=" + str(y), action_attributes))
            if action_type == "pub":
                try:
                    action_file = "%s_n%s" % (action['file'], i)
                except:
                    action_file = ""
                custom_args = " -l "
                custom_args = custom_args + " -p %d " % action['pause']
                try:
                    custom_args = custom_args + (" -k " if action['copy'] == "true" else "")
                except:
                    None
                if action_file:
                    cpy_cmd = "cp -n %s %s.temp || true" % (action_file, action_file)
                    ht_cmd = "haggletest -j tag -f %s.houtput %s app%d pub file %s.temp %s" % (my_outputfile, custom_args, app_count, action_file, action_attributes_string)
                    my_err_outputfile = my_outputfile + ".errcode"
                    last_delay = action_time - prev_time
                    cmd = "( (%s) && [ \"$(%s &> %s; echo $?)\" != \"0\" ] && (echo \"${NODE}: $(date) \\\"%s\\\" failed, code: $(cat %s)!\" >> ${FAILLOG}) )&" % (cpy_cmd, ht_cmd, my_err_outputfile, ht_cmd, my_err_outputfile)
                    cmds += [ cmd ]
                    sleeps += [ last_delay ]
                    prev_time = action_time
                else:
                    ht_cmd = "haggletest -j tag -f %s.houtput %s app%d pub %s" % (my_outputfile, custom_args, app_count, action_attributes_string)
                    my_err_outputfile = my_outputfile + ".errcode"
                    last_delay = action_time - prev_time
                    cmd = "( [ \"$(%s &> %s; echo $?)\" != \"0\" ] && (echo \"${NODE}: $(date) \\\"%s\\\" failed, code: $(cat %s)!\" >> ${FAILLOG}) )&" % (ht_cmd, my_err_outputfile, ht_cmd, my_err_outputfile)
                    cmds += [ cmd ]
                    sleeps += [ last_delay ]
                    prev_time = action_time
            elif action_type == "sub":
                custom_args = ""
                try:
                    custom_args = custom_args + " -s %d " % max(0, min(action['duration'], duration-action_time-MAX_END_DELAY_S))
                except:
                    None
                try:
                    custom_args = custom_args + " -w %d " % action['stop_after']
                except:
                    None
                try:
                    io_redirect = "> /dev/null"
                    if action['print_output']:
                        sub_output = my_outputfile + ".appout"
                        io_redirect = ">> %s" % (sub_output,)
                except:
                    None
                ht_cmd = "haggletest -j tag -a -f %s.houtput -c %s app%d sub %s %s" % (my_outputfile, custom_args, app_count, action_attributes_string, io_redirect)
                last_delay = action_time - prev_time
                cmd = "( [ \"$(%s %s; echo $?)\" != \"0\" ] && (echo \"${NODE}: $(date) \\\"%s\\\" failed!\" >> ${FAILLOG}) )&" % (ht_cmd, io_redirect, ht_cmd)
                cmds += [ cmd ]
                sleeps += [ last_delay ]
                prev_time = action_time
            app_count = app_count + 1
        j = 0
        for cmd in cmds:
            fo.write("    cmds[%i]='%s'\n" % (j, cmd))
            j += 1
        j = 0
        for sleeper in sleeps:
            fo.write("    sleeps[%i]='%.4f'\n" % (j, sleeper + randomInstance.uniform(0, 1)))
            j += 1
        fo.write("    (i=0; while [ $i -lt %d ]; do sleep ${sleeps[$i]}; eval ${cmds[$i]}; i=$((i+1)); done)&\n" % (j,))
        fo.write("    sleep %.2f\n" % duration)
        fo.write("    tail -n +1 %s*{houtput,appout} >> ${TFILE}\n" % my_outputfile_prefix)
        fo.write("    /sbin/ifconfig eth0 >> ${TFILE}\n")
        fo.write("    rm -f %s*\n" % my_outputfile_prefix)
    fo.write("else\n")
    fo.write("    exit 0\n")
    fo.write("fi\n")
    fo.write("cat ${TFILE} >> ${APPS_OUTPUT}\n")
    fo.write("rm -f ${TFILE}\n")

class Application:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_app_file(self, app_file):
        try:
            app = open(app_file, 'w')
            N = int(self.cfg['num_nodes'])
            duration = float(self.cfg['duration'])
            script = self.cfg['script']
            seed = int(self.cfg['seed'])
        except:
            print "Invalid parameters to application configuration."
            return False
        try:
            output_bash_to_file_obj(app, N, duration, script, seed)
            app.close()
            return True
        except Exception, e:
            print "Could not create application."
            print e
            return False

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "cust"
        pretty += "-"
        pretty += "s%d" % self.cfg['seed']
        return pretty
