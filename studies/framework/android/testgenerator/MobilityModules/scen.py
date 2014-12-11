#
# Copyright (c) 2014 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Sam Wood (SW)
#   Hasnain Lakhani (HL)


#
# scen mobility. Imports a ns2 scen file to be compatible with
# CORE.
#

import io
import re

from mobility_utils import convert_scen_for_core

def generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, scen_file, delay):
    convert_scen_for_core(scen_file, output_path + '.scen', num_nodes, duration, area_width, area_height, delay)
    
class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        try:
            num_nodes = int(self.cfg['num_nodes'])
            duration = int(self.cfg['duration'])
            area_width = int(self.cfg['area_width'])
            area_height = int(self.cfg['area_height'])
            scen_file = self.cfg['scen_file']
            try:
                delay = float(self.cfg['mobility_delay_s'])
            except:
                delay = 0
        except Exception, e:
            print e
            print "Invalid parameters to mobility configuration."
            return False
        try:
            generate_scen_file_helper(output_path, num_nodes, duration, area_width, area_height, scen_file, delay)
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
        pretty += "scen"
        return pretty
