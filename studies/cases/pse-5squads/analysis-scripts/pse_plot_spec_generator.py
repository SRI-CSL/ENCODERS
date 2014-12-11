# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import json
import docopt
import os
import re
import copy

BASE_SPEC = json.loads("""
{
    "base_configs": {
        "base_plot_line_per_category_cumulative_linear" : {
            "colors": ["b", "g", "r", "c", "m", "y", "k"],
            "linestyles": ["-", "--", "-.", ":"],
            "markers": [".", "o", "v", "^", "<", ">", "1", "2", "3", "4", "s", "p", "*", "|", "_", "x", "+", "h", "H", "D"],
            "markersize": 4,
            "grid": true,
            "xlim_min": 0,
            "xlim_max": 1800,
            "xlim_tick": 200,
            "outputsettings": {
                "bbox_inches": "tight"
            },
            "legend": {
                "loc": "lower center",
                "bbox_to_anchor": [1.15, 0.6],
                "ncol": 1,
                "fancybox": true,
                "shadow": true,
                "columnspacing": 1.0,
                "labelspacing": 0.0,
                "handletextpad": 0.0,
                "handlelength": 1.5 
            }
        },
        "base_plot_bar_per_category_two_axes_log_linear" : {
            "colors": ["b", "g", "r", "c", "m", "y", "k"],
            "y1scale": "log",
            "y2scale": "linear",
            "y2lim_min": 0.0,
            "y2lim_max": 1.0,
            "barwidth": 0.12,
            "offset": 0.24,
            "gap": 2,
            "ygap": 1,
            "align": "center",
            "outputsettings": {
                "bbox_inches": "tight"
            },
            "legend": {
                "loc": "lower center",
                "bbox_to_anchor": [0.5, -0.3],
                "ncol": 8,
                "fancybox": true,
                "shadow": true,
                "columnspacing": 1.0,
                "labelspacing": 0.0,
                "handletextpad": 0.0,
                "handlelength": 1.5 
            }
        }
    },
    "plots": []
}
""")

CUMULATIVE_TYPES = [
  {
    'skip': False,
    'type': 'plot_line_per_category_cumulative',
    'base_config': 'base_plot_line_per_category_cumulative_linear',
    'config': {
      'base_data': {
        'filename': 'delay_rel_to_request_all.csv',
        'header': 2,
        'markevery': 100
      },
      'title': 'Cumulative Delivery Over Time - ALL',
      'xlabel': 'Time (s)',
      'ylabel': 'Data Objects Delivered by time',
      'outputfile': 'cumulative_delivery_over_time_all_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_line_per_category_cumulative',
    'base_config': 'base_plot_line_per_category_cumulative_linear',
    'config': {
      'base_data': {
        'filename': 'delay_rel_to_request_a1.csv',
        'header': 2,
        'markevery': 100
      },
      'title': 'Cumulative Delivery Over Time - A1',
      'xlabel': 'Time (s)',
      'ylabel': 'Data Objects Delivered by time',
      'outputfile': 'cumulative_delivery_over_time_a1_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_line_per_category_cumulative',
    'base_config': 'base_plot_line_per_category_cumulative_linear',
    'config': {
      'base_data': {
        'filename': 'delay_rel_to_request_a2a3.csv',
        'header': 2,
        'markevery': 100
      },
      'title': 'Cumulative Delivery Over Time - A2+A3',
      'xlabel': 'Time (s)',
      'ylabel': 'Data Objects Delivered by time',
      'outputfile': 'cumulative_delivery_over_time_a2a3_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_line_per_category_cumulative',
    'base_config': 'base_plot_line_per_category_cumulative_linear',
    'config': {
      'base_data': {
        'filename': 'delay_rel_to_creation_all.csv',
        'header': 2,
        'markevery': 100
      },
      'title': 'Delivery Latency - ALL',
      'xlabel': 'Latency (s)',
      'ylabel': 'Data Objects Delivered within latency',
      'outputfile': 'latency_all_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_line_per_category_cumulative',
    'base_config': 'base_plot_line_per_category_cumulative_linear',
    'config': {
      'base_data': {
        'filename': 'delay_rel_to_creation_a1.csv',
        'header': 2,
        'markevery': 100
      },
      'title': 'Delivery Latency - A1',
      'xlabel': 'Latency (s)',
      'ylabel': 'Data Objects Delivered within latency',
      'outputfile': 'latency_a1_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_line_per_category_cumulative',
    'base_config': 'base_plot_line_per_category_cumulative_linear',
    'config': {
      'base_data': {
        'filename': 'delay_rel_to_creation_a2a3.csv',
        'header': 2,
        'markevery': 100
      },
      'title': 'Delivery Latency - A2+A3',
      'xlabel': 'Latency (s)',
      'ylabel': 'Data Objects Delivered within latency',
      'outputfile': 'latency_a2a3_%s.eps'
    }
  }
]

BAR_TYPES = [
  {
    'skip': False,
    'type': 'plot_bar_per_category_two_axes',
    'base_config': 'base_plot_bar_per_category_two_axes_log_linear',
    'config': {
      'base_data':{
        'base_filename': 'dissemination_stats_%s.csv',
        'filename': 'dissemination_stats.csv',
        'header': 2,
        'title': ''
      },
      'title': 'Overall Performance Summary',
      'labels': ['Tx', 'Rx', 'DRx', 'A2+A3', 'A1'],
      'xlabel': 'Composite Policy', 
      'y1len': 3,
      'y1label': 'Total bandwidth (MB)',
      'y2len': 2,
      'y2scale': 'linear',
      'y2label': 'Delivery Fraction (%)',
      'outputfile': 'dissemination_stats_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_bar_per_category_two_axes',
    'base_config': 'base_plot_bar_per_category_two_axes_log_linear',
    'config': {
      'base_data':{
        'base_filename': 'availability_responsiveness_%s.csv',
        'filename': 'availability_responsiveness.csv',
        'header': 2,
        'title': ''
      },
      'title': 'Data Availability Summary',
      'labels': ['Availability', 'Responsiveness'],
      'xlabel': 'Composite Policy', 
      'y1len': 1,
      'y1label': 'Availability (%)',
      'y2len': 1,
      'y1scale': 'linear',
      'y2lim_max': 50,
      'y2label': 'Responsiveness',
      'outputfile': 'availability_responsiveness_%s.eps'
    }
  },
  {
    'skip': False,
    'type': 'plot_bar_per_category_two_axes',
    'base_config': 'base_plot_bar_per_category_two_axes_log_linear',
    'config': {
      'base_data':{
        'base_filename': 'utility_%s.csv',
        'filename': 'utility.csv',
        'header': 2,
        'title': ''
      },
      'title': 'Data Utility Summary',
      'labels': ['Utility'],
      'xlabel': 'Composite Policy', 
      'y1len': 1,
      'y1label': 'Utility',
      'y2len': 0,
      'y1scale': 'linear',
      'y2label': '',
      'outputfile': 'utility_%s.eps'
    }
  }
]

def generate_spec(basename, dirname):

  splitRegex = re.compile(r"""pse5squads-[a-zA-Z0-9]*(s[0-9]*)-[a-zA-Z0-9\.]*-(NONE|PRIORITY|PRIORITY-UDISS|PRIORITY-UDISS-UCACHING|PRIORITY-UDISS-SCACHING)-[0-9]*squads-[0-9A-Za-z\-]*(UDISS1|UDISS2)-security-(off|dynamic|static)-[0-9]*_logs[0-9]*""")

  cases = []
  sort_order = {
    'Phase 1' : 1,
    'SP' : 2,
    'SP+UR1' : 3,
    'SP+UR2' : 4,
    'SP+UC+UR1' : 5,
    'SP+UC+UR2' : 6,
    'SP+SC+UR1' : 7,
    'SP+SC+UR2' : 8,
  }

  dirs = [d for d in os.listdir(dirname) if os.path.isdir(os.path.join(dirname,d))]
  for d in dirs:
    result = splitRegex.match(d)
    seed, strat, diss, security = result.groups()
    name = ''
    if strat == 'NONE':
      name = 'Phase 1'
    elif strat == 'PRIORITY':
      name = 'SP'
    else:
      name = 'SP'
      if 'UCACHING' in strat:
        name += '+UC'
      elif 'SCACHING' in strat:
        name += '+SC'
      if diss == 'UDISS1':
        name += '+UR1'
      elif diss == 'UDISS2':
        name += '+UR2'
    cases.append(dict(dir=os.path.abspath(os.path.join(dirname, d)), seed=seed, strat=strat, diss=diss, security=security, name=name))

  seeds = list(set(c['seed'] for c in cases))
  strats = list(set(c['strat'] for c in cases))
  disss = list(set(c['diss'] for c in cases))
  securitys = list(set(c['security'] for c in cases))
  names = list(set(c['name'] for c in cases))

  plots = []

  for base in CUMULATIVE_TYPES:
    for security in securitys:
      for seed in seeds:
        config = copy.deepcopy(base)
        config['config']['outputfile'] = basename + config['config']['outputfile'] % ('security-%s_%s' % (security, seed))
        datas = []
        casess = [c for c in cases if c['seed'] == seed and c['security'] == security]
        casess = sorted(casess, key=lambda c: sort_order[c['name']])
        for case in casess:
          data = copy.deepcopy(config['config']['base_data'])
          data['filename'] = os.path.join(case['dir'], data['filename'])
          data['title'] = case['name']
          datas.append(data)
        del config['config']['base_data']
        config['config']['data'] = datas
        plots.append(config)

  for base in BAR_TYPES:
    for security in securitys:
      config = copy.deepcopy(base)
      config['config']['outputfile'] = basename + config['config']['outputfile'] % ('security_%s' % security)
      datas = []
      casess = [c for c in cases if c['security'] == security and c['seed'] == seeds[0]]
      casess = sorted(casess, key=lambda c: sort_order[c['name']])
      for case in casess:
        data = copy.deepcopy(config['config']['base_data'])
        filename = config['config']['base_data']['base_filename'] % ('security-%s_%s' % (security, case['name'].replace('+', '').replace(' ', '')))
        filename = os.path.abspath(filename)
        if os.path.exists(filename):
          os.remove(filename)
        for (i, c) in enumerate([k for k in cases if k['security'] == case['security'] and k['name'] == case['name']]):
          inputfile = os.path.abspath(os.path.join(c['dir'], data['filename']))
          if os.path.exists(inputfile):
            with open(inputfile) as input:
              lines = input.readlines()
              if i >= 1:
                lines = lines[data['header']:]
              with open(filename, 'a') as output:
                for line in lines:
                  output.write(line)
          else:
            raise Exception("%s does not exist!" % inputfile)
        data['title'] = case['name'].replace('+', '\n')
        data['filename'] = filename
        del data['base_filename']
        datas.append(data)
      del config['config']['base_data']
      config['config']['data'] = datas
      plots.append(config)

  output = copy.deepcopy(BASE_SPEC)
  output['plots'] = plots
  print json.dumps(output, indent=2)

DOCOPT_STRING = """PSE Plot Spec Generator

Usage:
  pse_plot_spec_generator.py [options] <basename> <directory>
  pse_plot_spec_generator.py -h | --help
  pse_plot_spec_generator.py --version

Options:
  -h --help           Show this screen.
  --version           Show version.

"""


def main():
    arguments = docopt.docopt(DOCOPT_STRING, version='PSE Plot Spec Generator 0.1')
    generate_spec(arguments['<basename>'], arguments['<directory>'])

if __name__ == '__main__':
    main()

