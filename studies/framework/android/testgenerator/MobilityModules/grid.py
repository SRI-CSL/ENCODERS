#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# A basic grid topology. If not all the parameters are specified,
# we try to fit the nodes evenly within the space.
#

import io
import re

def generate_scen_file_helper(scen_output, N, duration, area_width, area_height, rows, cols, max_spacing_m):
    of = open(scen_output + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (N, duration, area_width, area_height))
    y_space = min(max_spacing_m, area_height/rows)
    y_init = 0
    if y_space * (rows - 1) < area_height:
        y_init = (area_height - y_space * (rows - 1)) / 2
    x_space = min(max_spacing_m, area_width/cols)
    x_init = 0
    if x_space * (cols - 1) < area_width:
        x_init = (area_width - x_space * (cols - 1)) / 2
    nn = 0
    for i in range (1, rows+1):
        y = y_init + y_space*(i-1)
        for j in range(1, cols+1):
            x = x_init + x_space*(j-1)
            nn = nn + 1
            of.write("$node_(%d) set X_ %.2f\n" % (nn, x))
            of.write("$node_(%d) set Y_ %.2f\n" % (nn, y))
    of.close()

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            duration  = int(self.cfg['duration'])
            num_nodes = int(self.cfg['num_nodes'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            rows = int(self.cfg['rows'])
            cols = int(self.cfg['cols'])
            try:
                max_spacing_m = int(self.cfg['max_spacing_m'])
            except:
                max_spacing_m = 200
            if rows * cols != num_nodes:
                print "Wrong number of rows/cols for num_nodes."
                return False
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, rows, cols, max_spacing_m)
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
        pretty += "grid"
        pretty += "-"
        pretty += "%dx%d" % (int(self.cfg['rows']), int(self.cfg['cols']))
        return pretty
