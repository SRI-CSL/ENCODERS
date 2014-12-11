#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Random waypoint mobility. Uses BonnMotion to generate
# the movement.
#

from heapq import heappush, heappop
import re
import commands
import io
import random

from mobility_utils import convert_scen_for_core

def generate_scen_file_helper(scen_output, N, duration, area_width, area_height, rwp_v_min, rwp_v_max, rwp_p_max, seed, delay=0):
    beginning_delay = 3600
    cmd_str = "bm -f %s RandomWaypoint -d %d -i %d -n %d -x %d -y %d -R %d -h %2f -l %2f -p %2f -o 3" % (scen_output, duration, beginning_delay, N, area_width, area_height, seed, rwp_v_max, rwp_v_min, rwp_p_max)
    commands.getoutput(cmd_str)
    cmd_str = "bm Statistics -f %s -r %d" % (scen_output, 250)
    commands.getoutput(cmd_str)
    cmd_str = "bm NSFile -f %s" % scen_output
    commands.getoutput(cmd_str)
    convert_scen_for_core(scen_output + '.ns_movements', scen_output + '.scen', N, duration, area_width, area_height, delay)

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            num_nodes = int(self.cfg['num_nodes'])
            duration = int(self.cfg['duration'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            rwp_v_min = float(self.cfg['rwp_v_min'])
            rwp_v_max = float(self.cfg['rwp_v_max'])
            rwp_p_max = float(self.cfg['rwp_p_max'])
            seed = int(self.cfg['seed'])
            try:
                # this is used to allow parent seed to adjust
                # seeds of all children in mobility overlay
                add_to_parent_seed = int(self.cfg['add_to_parent_seed'])
                # we need to construct a new seed, based on the parent seed
                seed = add_to_parent_seed + seed
            except:
                None

            try:
                delay = float(self.cfg['mobility_delay_s'])
            except:
                delay = 0
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, rwp_v_min, rwp_v_max, rwp_p_max, seed, delay)
            return True
        except Exception, e:
            print e
            print "Invalid mobility configuration"
            return False

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "rwp"
        pretty += "-"
        if verbosity >= 2:
            pretty += "v%.2f-%.2f" % (float(self.cfg['rwp_v_min']), float(self.cfg['rwp_v_max']))
            pretty += "p%.2f" % float(self.cfg['rwp_p_max'])
        pretty += "s%d" % self.cfg['seed']
        return pretty
