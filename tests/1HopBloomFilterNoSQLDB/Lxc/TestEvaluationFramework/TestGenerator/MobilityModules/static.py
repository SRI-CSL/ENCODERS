# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# Static mobility topology. Creates a mobility based on CORE's
# .imn format.
#

import io
import re

def generate_scen_file_helper(scen_output, N, duration, area_width, area_height, static_file):
    of = open(scen_output + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (N, duration, area_width, area_height))
    node_num = 0
    p1 = re.compile("node n(\d+) {")
    p2 = re.compile("iconcoords \{(\d+\.\d+) (\d+\.\d+)\}")
    for line in io.open(static_file, 'r'):
        m = p1.search(line)
        if m != None:
            node_num = int(m.group(1))
            continue
        m = p2.search(line)
        if m != None:
            x = float(m.group(1))
            y = float(m.group(2))
            if node_num == 0:
                print "wrong format!"
                raise Exception()
            of.write("$node_(%d) set X_ %.2f\n" % (node_num, x))
            of.write("$node_(%d) set Y_ %.2f\n" % (node_num, y))
    of.close()

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            num_nodes = int(self.cfg['num_nodes'])
            duration = int(self.cfg['duration'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            static_file = self.cfg['static_file']
        except:
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, static_file)
            return True
        except:
            print "Invalid mobility configuration"
            return False

    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "static"
        return pretty
