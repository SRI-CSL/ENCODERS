#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


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

SUB_MAX_END_DELAY_S=1 # subcribes must end at least this many seconds 
                      # before app duration

import random
import sys
from time import time
from ApplicationModules.custom import output_bash_to_file_obj

def get_subs(current, k, n, m, randomInstance):
    options = range(1, m) + range(m+1, n+1)
    if len(options) == 1:
        return options
    options = list(set(options) - set(current))
    subs = []
    while k > 0:
        new_sub = randomInstance.choice(options)
        subs = subs + [new_sub]
        options = list(set(options) - set([new_sub]))
        k = k - 1
    return subs

def get_sub_scripts(N, K, sub_s, T, seed):

    randomInstance = random.Random()
    randomInstance.seed(seed)

    scripts = []
    for i in range(1, N+1):
        time_elapsed=0
        current = []
        while (time_elapsed + sub_s) < (T-SUB_MAX_END_DELAY_S):
            current = get_subs(current, K, N, i, randomInstance) 
            current_s = map(lambda x: [ "n%d" % x, "true" ] , current)
            script = {
                "node_num"   : i,
                "type"       : "sub",
                "time"       : time_elapsed,
                "duration"   : sub_s,
                "attributes" : current_s
            }
            scripts += [ script ]
            time_elapsed += sub_s
    return scripts

def get_pub_scripts(N, num_pub, pub_freq_s, pub_file, extra_attributes):
    
    scripts = []
    for i in range(1, N+1):
        attributes = [ [ "n%d" % i , "true" ] ]
        attributes.extend(extra_attributes)
        script = {
            "node_num"   : i,
            "time"       : 0,
            "type"       : "pub",
            "file"       : pub_file,
            "num_pub"    : num_pub,
            "rate"       : pub_freq_s,
            "attributes" : attributes
        }
        scripts += [ script ]

    return scripts

class Application:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_app_file(self, app_file):
        try:
            app = open(app_file, 'w')
            K = int(self.cfg['K'])
            N = int(self.cfg['num_nodes'])
            duration = int(self.cfg['duration'])
            seed = int(self.cfg['seed'])

            if len(self.cfg['files']) != 1:
                print "Uniform expects only 1 file"
                raise Exception()

            num_pub = int(self.cfg['num_pub'])
            pub_freq_s = int(self.cfg['pub_freq_s'])
            pub_file = self.cfg['files'][0][0]

            sub_s = int(self.cfg['sub_s'])

            extra_attributes = []
            try:
                extra_attributes = self.cfg['attributes'][0]
            except:
                None

            if len(extra_attributes) != 0:
                if isinstance(extra_attributes, dict):
                    extra_attributes = extra_attributes.items()

            script = []
            pub_scripts = get_pub_scripts(N, num_pub, pub_freq_s, pub_file, extra_attributes)
            script += pub_scripts
            sub_scripts = get_sub_scripts(N, K, sub_s, duration, seed)
            script += sub_scripts
        except Exception, e:
            print "Invalid parameters to application configuration."
            print e
            return False
        try:
            output_bash_to_file_obj(app, N, duration, script, 1)
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
        pretty += "uniform"
        pretty += "-"
        pretty += "k%d" % int(self.cfg['K'])
        pretty += "pf%d" % int(self.cfg['pub_freq_s'])
        pretty += "sf%d" % int(self.cfg['sub_s'])
        pretty += "s%d" % int(self.cfg['seed'])
        return pretty
