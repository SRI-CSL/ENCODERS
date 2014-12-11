#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# A circle mobility module. The granularity of the node's movement 
# is a parameter (circle_density).
#

import io
import re
import math

def pythag(position_a, position_b):
    width = abs(position_a[0] - position_b[0])
    height =abs(position_a[1] - position_b[1])
    return math.sqrt(width ** 2 + height ** 2)

def generate_scen_file_helper(output_path, duration, area_width, area_height, circle_density, velocity):
    of = open(output_path + '.scen', 'w')
    of.write('# nodes: %d, max time: %2.2f, max x: %2.2f, max y: %2.2f\n' % (1, duration, area_width, area_height))

    fudge = 0
    area_width = area_width - fudge
    area_height = area_height - fudge

    of.write("$node_(%d) set X_ %.2f\n" % (1, area_width + fudge))
    of.write("$node_(%d) set Y_ %.2f\n" % (1, area_height/2 + fudge))

    theta = 2*math.pi/circle_density
    theta_cur = theta
    cur_time = 0
    prev_time = 0
    prev_pos = (area_width + fudge, area_height/2 + fudge)
    while cur_time <= duration:
        x = (area_width/2)*math.cos(theta_cur) + (area_width/2) + fudge
        y = (area_height/2)*math.sin(theta_cur) + (area_height/2) + fudge
        cur_pos = (x, y)
        theta_cur += theta
        distance = pythag(cur_pos, prev_pos)
        time = distance / velocity
        prev_pos = cur_pos
        of.write("$ns_ at %.2f \"$node_(%d) setdest %.2f %.2f %.2f\"\n" % (prev_time, 1, x, y, velocity))
        cur_time += time
        prev_time = cur_time
    of.close()

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            duration  = int(self.cfg['duration'])
            num_nodes = int(self.cfg['num_nodes'])
            if num_nodes != 1:
                raise Exception("invalid number of nodes")
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            circle_density = int(self.cfg['circle_density'])
            velocity = float(self.cfg['velocity'])
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, duration, area_width, area_height, circle_density, velocity)
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
        pretty += "circle"
        pretty += "-"
        pretty += "%d-" % (int(self.cfg['circle_density']),)
        pretty += "%.2f-" % (float(self.cfg['velocity']),)
        pretty += "%dx%d" % (int(self.cfg['area_width']), int(self.cfg['area_height']))
        return pretty
