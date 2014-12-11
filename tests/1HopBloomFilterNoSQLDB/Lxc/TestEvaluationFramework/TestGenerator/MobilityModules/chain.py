import io
import re
import grid

#
# Copyright (c) 2012 SRI International and Suns-tech Incorporated
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#  Sam Wood (SW) 

#
# A basic chain topology. This class is DEPRECATED in favor of
# grid.
# 

class Mobility:

    def __init__(self, cfg={}):
        self.cfg = cfg

    def generate_scen_file(self, output_path):
        self.cfg['rows'] = 1
        self.cfg['cols'] = self.cfg['num_nodes']
        grid_mobility = grid.Mobility(self.cfg)
        return grid_mobility.generate_scen_file(output_path)
  
    def get_pretty_name(self, verbosity):
        try:
            return self.cfg['pretty_name']
        except:
            None
        pretty = ""
        pretty += "chain"
        return pretty
