#!/usr/bin/perl -w

use strict;

my $n = 1;

my $num_dataobjects = 0;
my $num_nodes = 0;
my $num_dataobject_attrs = 0;
my $num_node_attrs = 0;
my $attr_pool_size = 0;

foreach(@ARGV) {
    my %timestamps;
    my @query_result;
    my @query_sql_time;
    my @query_result_time;
    my @query_end_time;
	my @time1;
	my @time2;
	my @time3;
	my @time4;

    open F, "<$_" or die "Could not open: $_\n";

    while (<F>) {
	if (/\# (.+)$/) {
	    my @params = split();

	    (undef, $num_dataobject_attrs) = split('=', $params[1]);
	    (undef, $num_node_attrs) = split('=', $params[2]);
	    (undef, $attr_pool_size) = split('=', $params[3]);
	    (undef, $num_dataobjects) = split('=', $params[4]);

	} elsif (/\#/) {
	    next;
	}
	my ($time, $type, $res) = split();
	
	$timestamps{$type} = $time;

	if ($type eq 'R') {
	    push @query_result, $res;
	} elsif ($type eq 'F') {
	    push @query_sql_time, $timestamps{'E'} - $timestamps{'S'};  
	    push @query_result_time, $timestamps{'R'} - $timestamps{'S'};  
	    push @query_end_time, $timestamps{'F'} - $timestamps{'I'};
	    push @time1, $timestamps{'S'} - $timestamps{'I'};  
	    push @time2, $timestamps{'E'} - $timestamps{'S'};  
	    push @time3, $timestamps{'R'} - $timestamps{'E'};  
	    push @time4, $timestamps{'F'} - $timestamps{'R'};  
	}
    }
    close F;

    $num_nodes=scalar(@query_result);

    printf("%d %f %f %f %f - %f %f %f %f\n", $num_dataobjects, mean(@query_sql_time), std_dev(@query_sql_time), mean(@query_result), std_dev(@query_result), mean(@time1)/mean(@query_end_time), mean(@time2)/mean(@query_end_time), mean(@time3)/mean(@query_end_time), mean(@time4)/mean(@query_end_time));
}

print "# attr_pool=$attr_pool_size num_node_attrs=$num_node_attrs num_dataobject_attrs=$num_dataobject_attrs num_nodes=$num_nodes\n";

sub mean {
    my $result;
    if (scalar(@_) == 0) {
	return 0;
    }
    foreach (@_) { $result += $_ }
    return $result / @_;
}

sub std_dev {
    my $mean = mean(@_);
    my @elem_squared;
    foreach (@_) {
	push (@elem_squared, ($_ **2));
    }
    return sqrt( mean(@elem_squared) - ($mean ** 2));
}

sub min {
    my $min;
    foreach (@_) {

	if (!defined($min)) {
	    $min = $_;
	} elsif ($_ < $min) {
	    $min = $_;
	}
    }
    return $min;
}

sub max {
    my $max;
    foreach (@_) {

	if (!defined($max)) {
	    $max = $_;
	} elsif ($_ > $max) {
	    $max = $_;
	}
    }
    return $max;
}

sub median {
    return $_[($#_ /2)];
}
sub sum {
    my $sum = 0;
    
    foreach (@_) {
	$sum += $_;
    }
    return $sum;
}
