Relevant files & directories:

    - ../studies/framework/lxc/loganalysis/analyze.py : The automated log analysis framework.
    - ../studies/framework/lxc/loganalysis/plotter.py : The automated plot generation framework.

Log Analysis:
=============

The log analysis script parses the output from a test run and allows
users to specify custom queries to run on the output and produce
summary reports in order to gather statistics from a test run.

It can also optionally parse the output of the observer application
for more detailed analysis of log output.

Usage options
-------------

To run, just run the analyze.py file and it will print out 
a set of options that can be used.

Specifying a db path is helpful when one has a large test
run to analyze, as then subsequent analysis run on the same
data (with newer runs) can be faster since the log files
will not have to be parsed again. The db will be
created inside the test output folder.

The parallel option allows parsing of multiple test
cases in parallel if one is analyzing a run with
multiple test cases. This may speed up analysis
significantly on large test runs.

Database Querying
-----------------

The ER diagram is available in loganalysis_ER_diagram.png.

This details the structure of the database which contains
parsed log file data, as well as parsed observer data.

Queries in the next section will be run against this database.

Analysis Specification
----------------------

The analysis script expects to be given a JSON file which specifies
the queries to be run and the output to be produced. 

For a sample, look at analysis.json in ../studies/cases/pse/analysis-scripts

Each option is described below.

    parse_haggle_dbs - if true, parse the haggle database files at each node
    parse_observer_files - if true, parse the observer files at each node

    squads
    ------

    A dictionary of squads and the nodes in each squad. For example:

        "squads": {
            "s1": ["n1", "n2"],
            "s2": ["n3"]
        }

    Would create two squads, one with n1 and n2, and one with n3.

    These entries will be present in the database for further querying.

    custom_tables
    -------------

    A list of dictionaries, each specifying a specification for a custom table
    to be created in the database. This can help when one requires custom
    join tables to simplify queries.

    The following entries are required when specifying a custom table:

        name - The name of the table
        columns - A list of columns, specified as dictionaries containing a name and a type entry
        indices - A list of column names to create indices for
        unique_indices - A list of column names to create unique indexes for
        values - A list of lists, one per row to be inserted into the table

    custom_views
    ------------

    A list of SQL create view commands, as strings.

    These can help when one requires custom views to simplify queries.

    reports
    -------

    A list of reports. Each report is a dictionary
    which contains the following entries:

        skip - (optional) boolean, whether to skip producing this report
        title - The title of the report
        format - Can be either "pretty" (default) or "csv"
        repeat - Repeat the query for each of the arguments in the list (detailed below)
        logfile - The file to store the output in, if specified. Can contain "^repeat"
        type - A list of strings describing the settings of the report
               Can also be just a single string if there is only one type
               The following types are possible:

                    summary - normal report type
                    silent - do not print output to stdout for this report
                    nodes - repeat the queries for each node
                    squads - repeat the queries for each squad
                    fetchall - if each query will return multiple rows, not just one
                    transpose - transpose query results
        fields - A list of dictionaries, one per column in the report. Each dictionary
                 must have the following:

                    type - (optional) either "single" or "multi" if the query returns multiple columns
                    name - The name of the column
                    format - The string formatting specifier to apply
                    default - Default value in case of NULL
                    query - The SQL query to execute
                    args - Query arguments to be used for this query
                           "id" will be available as the squad/node id if applicable
                           "^repeat" will be available as the repeat value if applicable

                    Note that if the "type" is multi, "name", "format", and "default" should be lists




Plotting:
=========

The plotting script takes a plot specification and produces
various plots. It is intended to be used in conjunction
with the analysis script to produce a visual representation
of the results of a test run.

Usage options
-------------

To run, just run the plotter.py file and it will print out 
a set of options that can be used.

The parallel option allows parsing of multiple test
cases in parallel if one is analyzing a run with
multiple test cases. This may speed up analysis
significantly on large test runs.

Plot Specification
------------------

The plot script expects to be given a JSON file which specifies
the graphs to be generated.

For a sample, look at plot_per_case.json in ../studies/cases/pse/analysis-scripts

Each option is described below.

    base_configs
    ------------

    A dictionary of dictionaries, where each sub-dictionary
    contains plot-specific options (described later).
    These can be used to factor out common layout options
    for related plots.

    plots
    -----

    A list of dictionaries, where each sub-dictionary describes a plot.
    Each sub-dictionary contains the following settings:

        skip - (optional) boolean, whether to skip producing this graph
        type - The plot type
        base_config - (optional) The base configuration to use
        config - A dictionary containing the configuration settings.
                 It will inherit options from base_config and will
                 add/override any options given here.

            data - A dictionary describing the input data (varies per plot type)
            outputfile - The output file name

Base Plot Layout
----------------

Each plot has a base set of settings that is
common amongst all plot types. The names mostly
correspond to matplotlib names. An example
base config is below:

    {
        "colors": ["b", "g", "r", "c", "m", "y", "k"],
        "linestyles": ["-", "--", "-.", ":"],
        "markers": [".", "o", "v", "^", "<", ">", "1", "2", "3", "4", "s", "p", "*", "|", "_", "x", "+", "h", "H", "D"],
        "markersize": 4,
        "grid": true,
        "outputsettings": {
            "bbox_inches": "tight"
        },
        "legend": {
            "loc": "lower center",
            "bbox_to_anchor": [0.5, -0.3],
            "ncol": 10,
            "fancybox": true,
            "shadow": true,
            "columnspacing": 1.0,
            "labelspacing": 0.0,
            "handletextpad": 0.0,
            "handlelength": 1.5 
        }
    }

Colors, linestyles, and markers will be drawn in increasing order per line/bar in the plot.
The markersize specifies the size of the markers.

The outputsettings is a dictionary containing keyword arguments passed to the matplotlib savefig call.
The legend is a dictionary containing keyword arguments passed to the matplotlib legend call.

Plot Types
----------

plot_line_per_node
------------------

    data
    ----

    Should be a dictionary of the following form:

        {
            "filename": "cpu.csv",
            "header": 2
        },

    The file should be a .csv file with one line per 
    (node_name, entry_number, value) triple.

    The number of header lines at the top of the file is specified
    by "header".

    It will plot entry_number on the X axis versus value on the Y axis,
    with one line per node.

    Layout
    ------

    The following additional layout parameters are expected/supported:

        xlim_min - The minimum value on the x axis
        xlim_max - The maximum value on the x axis
        xlim_tick - The tick size on the x axis
        title - The plot title
        xlabel - The label on the x axis
        ylabel - The label on the y axis

plot_line_per_node_with_sum
---------------------------

This is almost identical to the previous plot,
aside from the fact that a "sum" line is drawn
with a different y axis scale.

    Layout
    ------

    The following additional layout parameters are expected/supported,
    relative to plot_line_per_node:

        y1label - The label on the y axis
        y2label - The label on the sum axis

plot_line_per_category_cumulative
---------------------------------

    data
    ----

    Should be a list containing dictionaries of the following form:

        {
            "filename": "delay_rel_to_request.csv",
            "header": 2,
            "title": "Delay Relative to Request"
        }

    The files should be .csv files containing one number per line
    It will plot a cumulative distribution, i.e. for each
    x value, the corresponding y value will be the count of x
    values seen so far.

    The number of header lines at the top of each file is specified
    by "header".

    Layout
    ------

    The following additional layout parameters are expected/supported:

        xlim_min - The minimum value on the x axis
        xlim_max - The maximum value on the x axis
        xlim_tick - The tick size on the x axis
        title - The plot title
        xlabel - The label on the x axis
        ylabel - The label on the y axis

plot_bar_per_file
-----------------

    data
    ----

    Should be a list containing dictionaries of the following form:

        {
            "filename": "dissemination_stats.csv",
            "header": 2,
            "title": ""
        }

    The files should be .csv files containing one value per line.
    It will compute the means and deviations of each file
    and, for each file, it will plot a bar.

    The number of header lines at the top of each file is specified
    by "header".

    Layout
    ------

    The following additional layout parameters are expected/supported:

        title - The plot title
        xlabel - The label on the x axis
        ylabel - The label on the y axis
        barwidth - The width of the bars
        offset - The offset from the sides of the axis
        align - The alignment of the histograms w.r.t the bars
        yscale - The scale for the first y axis
        ylim_min - The minimum value on the second y axis
        ylim_max - The maximum value on the second y axis

plot_bar_per_category_two_axes
------------------------------

    data
    ----

    Should be a list containing dictionaries of the following form:

        {
            "filename": "dissemination_stats.csv",
            "header": 2,
            "title": ""
        }

    The files should be .csv files containing one row per line.
    It will compute the means and deviations of each column
    and, for each column, it will plot a bar.

    Each file will be plotted as one set of bars.

    The number of header lines at the top of each file is specified
    by "header".

    Layout
    ------

    The following additional layout parameters are expected/supported:

        title - The plot title
        labels - A list of the column labels
        xlabel - The label on the x axis
        barwidth - The width of the bars
        offset - The offset from the sides of the axis
        gap - The gap between sets of bars
        align - The alignment of the histograms w.r.t the bars
        ygap - The gap between bars for the same set but on different axes
        y1scale - The scale for the first y axis
        y2scale - The scale for the second y axis
        y2lim_min - The minimum value on the second y axis
        y2lim_max - The maximum value on the second y axis
        y1len - The number of columns to use on the first y axis
        y2len - The number of columns to use on the second y axis
        y1label - The label on the first y axis
        y2label - The label on the second y axis

plot_histogram
--------------

    data
    ----

    Should be a dictionary of the following form:

        {
            "filename": "cpu.csv",
            "header": 2
        },

    The file should be a .csv file with one value per line.

    The number of header lines at the top of the file is specified
    by "header".

    It will plot a histogram of the values in the file

    Layout
    ------

    The following additional layout parameters are expected/supported:

        xlim_min - The minimum value on the x axis
        xlim_max - The maximum value on the x axis
        xlim_tick - The tick size on the x axis
        title - The plot title
        xlabel - The label on the x axis
        ylabel - The label on the y axis
        bin_width - The width of the bins to use
        normed - Whether to norm the y values to [0,1]
        cumulative - Whether to create a cumulative histogram
        log - Whether to use a log scale
        align - The alignment of the bars
        histtype - The type of the histogram (step, bar, barfilled, stepfilled)

plot_histogram_multi
--------------------

    data
    ----

    Should be a list of dictionaries of the following form:

        {
            "filename": "cpu.csv",
            "header": 2
        },

    The file should be a .csv file with one value per line.

    The number of header lines at the top of the file is specified
    by "header".

    It will plot a histogram of the values in each file.

    Layout
    ------

    The same layout parameters as plot_histogram are used.

plot_histogram
--------------

    data
    ----

    Should be a dictionary of the following form:

        {
            "filename": "cpu.csv",
            "header": 2
        },

    The file should be a .csv file with one value per line.

    The number of header lines at the top of the file is specified
    by "header".

    It will plot a histogram-like line of the values in each file,
    with the y values being plotted at the midpoints of the bars

    Layout
    ------

    The following additional layout parameters are expected/supported:

        xlim_min - The minimum value on the x axis
        xlim_max - The maximum value on the x axis
        xlim_tick - The tick size on the x axis
        title - The plot title
        xlabel - The label on the x axis
        ylabel - The label on the y axis
        bin_width - The width of the bins to use
