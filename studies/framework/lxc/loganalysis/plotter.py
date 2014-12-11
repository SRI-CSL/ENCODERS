# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

import numpy
import json
import collections
import copy
import docopt
import contextlib
import os
import multiprocessing

import matplotlib
import matplotlib.pyplot as plt

class PlotLinePerNode(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.nodes = collections.OrderedDict()
        with open(self.data['filename']) as input:
            self.markevery = self.data.get('markevery')
            lines = input.readlines()[self.data['header']:]
            for line in lines:
                node, x, y = line.split(',')
                x = float(x)
                y = float(y)

                if node not in self.nodes:
                    self.nodes[node] = {'x': [], 'y': []}
                self.nodes[node]['x'].append(x)
                self.nodes[node]['y'].append(y)

    def plot(self):
        fig, ax1 = plt.subplots()
        fig.gca().set_color_cycle(self.colors)
        plt.title(self.title)
        ax1.set_xlabel(self.xlabel)
        ax1.set_ylabel(self.ylabel)
        lines = []
        i = 0
        for name, node in self.nodes.items():
            line = ax1.plot(node['x'], node['y'], label=name, marker=self.markers[i%len(self.markers)], markersize=self.markersize, linestyle=self.linestyles[i%len(self.linestyles)])
            if self.markevery:
                line[0].set_markevery(self.markevery)
            lines.append(line[0])
            i = i + 1
        ax1.grid(self.grid)
        ax1.set_xlim(self.xlim_min, self.xlim_max)
        ax1.set_xticks([t for t in xrange(self.xlim_min, self.xlim_max + self.xlim_tick, self.xlim_tick)])
        labels = [line.get_label() for line in lines]
        plt.legend(lines, labels, **self.legend)
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

class PlotLinePerNodeWithSum(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.nodes = collections.OrderedDict()
        with open(self.data['filename']) as input:
            self.markevery = self.data.get('markevery')
            lines = input.readlines()[self.data['header']:]
            for line in lines:
                node, x, y = line.split(',')
                x = float(x)
                y = float(y)

                if node not in self.nodes:
                    self.nodes[node] = {'x': [], 'y': []}
                self.nodes[node]['x'].append(x)
                self.nodes[node]['y'].append(y)

        sumdict = {'x': [], 'y': []}

        self.xrange = []
        for name, node in self.nodes.items():
            xs = node['x']
            if len(xs) > len(self.xrange):
                self.xrange = xs[:]

        sumdict['x'] = self.xrange[:]
        for i,t in enumerate(self.xrange):
            sm = 0
            for name, node in self.nodes.items():
                if i < len(node['y']):
                    sm = sm + node['y'][i]
                else:
                    sm = sm + node['y'][-1]
            sumdict['y'].append(sm)

        self.sum = sumdict

    def plot(self):
        fig, ax1 = plt.subplots()
        fig.gca().set_color_cycle(self.colors)
        plt.title(self.title)
        ax1.set_xlabel(self.xlabel)
        ax1.set_ylabel(self.y1label)
        lines = []
        i = 0
        for name, node in self.nodes.items():
            line = ax1.plot(node['x'], node['y'], label=name, marker=self.markers[i%len(self.markers)], markersize=self.markersize, linestyle=self.linestyles[i%len(self.linestyles)])
            if self.markevery:
                line[0].set_markevery(self.markevery)
            lines.append(line[0])
            i = i + 1
        ax1.grid(self.grid)
        ax1.set_xlim(self.xlim_min, self.xlim_max)
        ax1.set_xticks([t for t in xrange(self.xlim_min, self.xlim_max + self.xlim_tick, self.xlim_tick)])
        ax2 = ax1.twinx()
        ax2.set_xlim(self.xlim_min, self.xlim_max)
        ax2.set_ylabel(self.y2label)
        line = ax2.plot(self.sum['x'], self.sum['y'], label='sum', marker=self.markers[i%len(self.markers)], markersize=self.markersize, linestyle=self.linestyles[i%len(self.linestyles)])
        lines.append(line[0])
        labels = [line.get_label() for line in lines]
        plt.legend(lines, labels, **self.legend)
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

class PlotLinePerCategoryCumulative(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.cats = collections.OrderedDict()
        self.markeverys = []
        for entry in self.data:
            title = entry['title']
            filename = entry['filename']
            header = entry['header']
            self.cats[title] = {'filename': filename, 'title': title}
            self.markeverys.append(entry.get('markevery'))
            with open(filename) as input:
                lines = input.readlines()[header:]
                self.cats[title]['x'] = [float(line) for line in lines]
                self.cats[title]['y'] = [x for x in xrange(1, len(lines)+1)]

    def plot(self):
        fig, ax = plt.subplots()
        fig.gca().set_color_cycle(self.colors)
        plt.title(self.title)
        ax.set_xlabel(self.xlabel)
        ax.set_ylabel(self.ylabel)
        lines = []
        i = 0
        for title, cat in self.cats.items():
            line = ax.plot(cat['x'], cat['y'], label=title, marker=self.markers[i%len(self.markers)], markersize=self.markersize, linestyle=self.linestyles[i%len(self.linestyles)])
            if self.markeverys[i]:
                line[0].set_markevery(self.markeverys[i])
            lines.append(line[0])
            i = i + 1
        ax.grid(self.grid)
        ax.set_xlim(self.xlim_min, self.xlim_max)
        ax.set_xticks([t for t in xrange(self.xlim_min, self.xlim_max + self.xlim_tick, self.xlim_tick)])
        labels = [line.get_label() for line in lines]
        plt.legend(lines, labels, **self.legend)
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

class PlotBarPerFile(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.cats = collections.OrderedDict()
        self.means = []
        self.stds = []
        self.labels = []
        for entry in self.data:
            title = entry['title']
            filename = entry['filename']
            header = entry['header']
            self.cats[title] = {'filename': filename, 'title': title}
            with open(filename) as input:
                lines = input.readlines()[header:]
                data = [float(line) for line in lines]
                if len(data) > 0:
                    mean = numpy.mean(data)
                    std = numpy.std(data)
                else:
                    mean = 0
                    std = 0
                self.means.append(mean)
                self.stds.append(std)
                self.labels.append(title)

    def plot(self):
        fig, ax1 = plt.subplots()
        plt.title(self.title)
        ax1.set_yscale(self.yscale)
        ax1.set_ylabel(self.ylabel)
        ax1.set_xlabel(self.xlabel)
        ax1.set_xticks(numpy.arange(len(self.cats))*self.barwidth)
        ax1.set_ylim(self.ylim_min, self.ylim_max)
        rects = []
        for x in xrange(0, len(self.cats)):
            rect = ax1.bar(x*self.barwidth, self.means[x], self.barwidth, color=self.colors[x%len(self.colors)], label=self.labels[x], yerr=self.stds[x], align=self.align, log=(self.yscale=='log'), ecolor='k')
            rects.append(rect)
        ax1.set_xticklabels(self.cats.keys())
        labels = [rect.get_label() for rect in rects]
        plt.legend(rects, labels, **self.legend)
        ax1.xaxis_date()
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()


class PlotBarPerCategoryTwoAxes(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.cats = collections.OrderedDict()
        self.means = []
        self.stds = []
        for entry in self.data:
            title = entry['title']
            filename = entry['filename']
            header = entry['header']
            self.cats[title] = {'filename': filename, 'title': title}
            with open(filename) as input:
                lines = input.readlines()[header:]
                data = []
                for line in lines:
                    data.append(map(float, line.split(',')))
                transposed = zip(*data)
                means = [numpy.mean(d) for d in transposed]
                stds = [numpy.std(d) for d in transposed]
                self.means.append(means)
                self.stds.append(stds)

    def plot(self):
        fig, ax1 = plt.subplots()
        plt.title(self.title)
        ax1.set_yscale(self.y1scale)
        ax1.set_ylabel(self.y1label)
        ax1.set_xlabel(self.xlabel)
        mid = (self.y1len + self.y2len) / 2
        ax1.set_xticks(numpy.arange(len(self.cats))*self.barwidth*(self.y1len+self.y2len+self.gap+self.ygap) + self.barwidth * mid)
        lrects = []
        rects = []
        for x in xrange(0, len(self.cats)):
            rects.append([])
            for y in xrange(0, self.y1len):
                rect = ax1.bar(x*(self.barwidth*(self.y1len+self.y2len+self.gap+self.ygap))+self.barwidth*(y), self.means[x][y], self.barwidth, color=self.colors[y%len(self.colors)], label=self.labels[y], yerr=self.stds[x][y], align=self.align, log=(self.y1scale=='log'))
                rects[x].append(rect)
                if x == 0:
                    lrects.append(rect)
        ax2 = ax1.twinx()
        ax2.set_yscale(self.y2scale)
        ax2.set_ylabel(self.y2label)
        ax2.set_xlabel(self.xlabel)
        ax2.set_xlim(-(self.offset*1.5), len(self.cats)*self.barwidth*(self.y1len+self.y2len) + (len(self.cats)-1)*(self.gap+self.ygap)*self.barwidth + self.offset)
        ax2.set_xticks(numpy.arange(len(self.cats))*self.barwidth*(self.y1len+self.y2len+self.gap+self.ygap) + self.barwidth*mid)
        ax2.set_ylim(self.y2lim_min, self.y2lim_max)
        for x in xrange(0, len(self.cats)):
            for y in xrange(self.y1len, self.y1len+self.y2len):
                rect = ax2.bar(x*(self.barwidth*(self.y1len+self.y2len+self.gap+self.ygap))+self.barwidth*(y+self.ygap), self.means[x][y], self.barwidth, color=self.colors[y%len(self.colors)], label=self.labels[y], yerr=self.stds[x][y], align=self.align, log=(self.y2scale=='log'))
                rects[x].append(y)
                if x == 0:
                    lrects.append(rect)
        ax1.set_xticklabels(self.cats.keys())
        labels = [rect.get_label() for rect in lrects]
        plt.legend(lrects, labels, **self.legend)
        ax1.xaxis_date()
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

class PlotHistogram(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.x = []
        with open(self.data['filename']) as input:
            lines = input.readlines()[self.data['header']:]
            self.x = [float(line) for line in lines]
        if len(self.x) > 0:
            maxv = numpy.array(self.x).flatten().max()
            if maxv > self.xlim_max:
                self.xlim_max = int(maxv * 1.01)

    def plot(self):
        fig, ax = plt.subplots()
        fig.gca().set_color_cycle(self.colors)
        plt.title(self.title)
        ax.set_xlabel(self.xlabel)
        ax.set_ylabel(self.ylabel)
        ax.grid(self.grid)
        ax.set_xlim(self.xlim_min, self.xlim_max)
        ax.set_xticks([t for t in xrange(self.xlim_min, self.xlim_max + self.xlim_tick, self.xlim_tick)])
        if self.log:
            ax.set_yscale('symlog')
        if len(self.x) > 0:
            ax.hist(self.x, bins=[b for b in xrange(self.xlim_min, self.xlim_max + self.bin_width, self.bin_width)], normed=self.normed, cumulative=self.cumulative, histtype=self.histtype, align=self.align)
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

class PlotHistogramMulti(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.x = []
        self.labels = []
        for entry in self.data:
            filename = entry['filename']
            header = entry['header']
            title = entry['title']
            with open(filename) as input:
                lines = input.readlines()[header:]
                dat = [float(line) for line in lines]
                if len(dat) > 0:
                    self.x.append(dat)
                    self.labels.append(title)
        if len(self.x) > 0:
            maxv = max([item for sublist in self.x for item in sublist])
            if maxv > self.xlim_max:
                self.xlim_max = int(maxv * 1.01)

    def plot(self):
        fig, ax = plt.subplots()
        fig.gca().set_color_cycle(self.colors)
        plt.title(self.title)
        if self.log:
            ax.set_yscale('symlog')
        ax.set_xlabel(self.xlabel)
        ax.set_ylabel(self.ylabel)
        ax.grid(self.grid)
        ax.set_xlim(self.xlim_min, self.xlim_max)
        ax.set_xticks([t for t in xrange(self.xlim_min, self.xlim_max + self.xlim_tick, self.xlim_tick)])
        if len(self.x) > 0:
            ax.hist(self.x, bins=[b for b in xrange(self.xlim_min, self.xlim_max + self.bin_width, self.bin_width)], normed=self.normed, cumulative=self.cumulative, histtype=self.histtype, align=self.align, label=self.labels)
        plt.legend(**self.legend)
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

class PlotHistogramLine(object):

    def __init__(self, config):
        self.config = config
        self.__dict__.update(config)

    def load_data(self):
        self.x = []
        with open(self.data['filename']) as input:
            lines = input.readlines()[self.data['header']:]
            self.x = [float(line) for line in lines]
        if len(self.x) > 0:
            maxv = numpy.array(self.x).flatten().max()
            if maxv > self.xlim_max:
                self.xlim_max = int(maxv * 1.01)

    def plot(self):
        fig, ax = plt.subplots()
        fig.gca().set_color_cycle(self.colors)
        plt.title(self.title)
        ax.set_xlabel(self.xlabel)
        ax.set_ylabel(self.ylabel)
        ax.grid(self.grid)
        ax.set_xlim(self.xlim_min, self.xlim_max)
        ax.set_xticks([t for t in xrange(self.xlim_min, self.xlim_max + self.xlim_tick, self.xlim_tick)])
        if len(self.x) > 0:
            ys, xs = numpy.histogram(self.x, bins=[b for b in xrange(self.xlim_min, self.xlim_max + self.bin_width, self.bin_width)])
            xs = (xs[1:] + xs[:-1]) / 2 
            line = ax.plot(xs, ys, label=self.title, marker=self.markers[0], markersize=self.markersize, linestyle=self.linestyles[0])
        plt.savefig(self.outputfile, **self.outputsettings)
        plt.close()

def handle_plot(config):
    baseConfigs = config.get('base_configs', [])
    plots = config.get('plots', [])

    plotHandlers = {
        'plot_line_per_node': PlotLinePerNode,
        'plot_line_per_node_with_sum': PlotLinePerNodeWithSum,
        'plot_line_per_category_cumulative': PlotLinePerCategoryCumulative,
        'plot_bar_per_file': PlotBarPerFile,
        'plot_bar_per_category_two_axes': PlotBarPerCategoryTwoAxes,
        'plot_histogram': PlotHistogram,
        'plot_histogram_multi': PlotHistogramMulti,
        'plot_histogram_line': PlotHistogramLine
    }

    for plot in plots:

        if plot.get('skip', False):
            continue

        plotConfig = {}
        if plot.get('base_config') is not None:
            plotConfig = copy.deepcopy(baseConfigs[plot['base_config']])
        plotConfig.update(plot['config'])

        plotter = plotHandlers[plot['type']](plotConfig)
        plotter.load_data()
        plotter.plot()

@contextlib.contextmanager
def cd(path):
    old_dir = os.getcwd()
    os.chdir(path)
    yield
    os.chdir(old_dir)

def handle_plot_wrapper(args):
    (config, dirname) = args
    with cd(dirname):
        handle_plot(config)
    print 'Plotted %s' % dirname

DOCOPT_STRING = """Graph Plotter

Usage:
  plotter.py [options] <config_path> [<dir>...]
  plotter.py -h | --help
  plotter.py --version

Options:
  -p --parallel=<n>   Parse data in parallel using n processes [default: 1].
  -h --help           Show this screen.
  --version           Show version.

"""


def main():
    arguments = docopt.docopt(DOCOPT_STRING, version='Plotter 0.1')
    with open(arguments['<config_path>']) as input:
        config = json.load(input)

    dirs = arguments['<dir>']
    if len(dirs) == 0:
        dirs.append(os.getcwd())

    processes = int(arguments['--parallel'])
    if processes > 1:
        pool = multiprocessing.Pool(processes=processes)
        results = pool.map(handle_plot_wrapper, [(config, d) for d in dirs])
    else:
        for d in dirs:
            handle_plot_wrapper((config, d))

if __name__ == '__main__':
    main()

