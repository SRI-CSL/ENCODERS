gen_data.py - has the python script I use to generate the application's
publish/subscribe patterns.

get_stats.sh - the script used to parse the "apps_output" after the 
test has run, and collect metrics such as bandwidth (tx/rx bandwidth
is parsed from output of ifconfig). 

scengen  - contains a ns-2 mobility generator I use (and slightly tweak)
to work w/ core.

WARNING: 
The test cases expect some files in /tmp:

dd if=/dev/zero of=/tmp/2MB bs=1024 count=2048
dd if=/dev/zero of=/tmp/500KB bs=1024 count=500

Make sure your core.conf is setup correctly:

/etc/core/core.conf:
custom_services_dir = /home/username/.core/myservices

Some more 10 node scenarios:

500x300m^2 area, 250m broadcast range, RWP with
[1,7] m/s velocity, [0,5] s pause time, 300 sec duration*,
120 sec "warm up" time (no publishers/subscribers),
60 second publisher "cool down" time (nobody publishes
any more data objects 60 seconds before the end of
the scenario). The beaconing is set to 1 second. Node
refresh of 15 seconds, 30 seconds purge. Refresh and
purge are disabled for "none" routing. TCP only.
54 Mps lossless link, 20ms delay. 

* where [x,y] means chosen uniformly at random in interval [x,y].

There are N nodes. We vary N=10, 20.

Every 10 seconds, each node publishes a data object
with the node name as the attribute.

Every 45 seconds, each node clears all their subscriptions,
and subscribes to K different nodes (not including
themselves, not 1 of the immediately K previous
nodes subscribed to).

We vary K=1,3

We vary the data object size D=500KB, 2MB.

We vary the routing protocol R=DIRECT, ProphetS, Epidemic, none
(flood is simply epidemic, but I think epidemic is the more correct
terminology to distinguish between a flood that does not traverse
connected components).

thus |N|*|K|*|D|*|R| = 2*2*2*4 = 32 scenarios.

We only run each scenario once (1 run).

If the pub time is before the interest create time then i measure
the delay as from the interest create time and recv. time.
Otherwise, I measure from the pub time to the recv. time.
(i.e the soonest it could possibly be delivered).

N=10,K=1,D=2M
# node name    rx              tx          avg delay   DO recv
DIRECT         1492643209      1495027642  12.286818   187
PROPHET         890312759        891719761  8.755738   122
EPIDEMIC       2599181352      2605215022   5.180840   238 
none            380183367       381018706   7.170248   105

N=10,K=3,D=2M
# node name    rx              tx          avg delay   DO recv
DIRECT         2230269945      2236831859  6.519662    627 
PROPHET        2325031715      2333213084  6.658693    632
EPIDEMIC       2510005025      2516159812  4.245279    695
none           1737378194      1743990525  7.917148    527

N=10,K=1,D=500K
# node name    rx              tx          avg delay   DO recv
DIRECT         429580537       430112736   7.846265    226
PROPHET        196241159       196656728   8.805712    111
EPIDEMIC       640148579       640593773   5.389549    226
none            57697444        57750847   3.180891    64

N=10,K=3,D=500K
# node name    rx              tx          avg delay   DO recv
DIRECT         588484707       588751652   3.524778    702
PROPHET        560157514       560604339   4.352705    644
EPIDEMIC       641757613       642165215   3.366791    741
none           377686301       377758351   3.368557    490

This last result is a re-run, where the previous result had:

Protocol     Total RX      Total TX       Avg delay  DO received
DIRECT     588640679 589050765 3.905290   714
PROPHET 608302325 608707742 4.160718   670
EPIDEMIC 641194503 641473751 3.131699   740
none           376530361 376663592 3.458342   479

So the results do vary across runs, meaning more runs are necessary for
more precise results.

EXPLANATION OF DELAY:

Typically these metrics are applied at the packet granularity, but in
our framework we apply it at the data object granularity. 

When computing packet latency it is standard to not include 
packets that were not delivered (I'm not sure how you would
include them in a meaningful way?) Similarly, when computing
data object delay we do not include data objects that were not
delivered. 

The story is even more complicated because unlike in a traditional
packet based system, here the senders are not necessarily
aware of the receivers, and data objects may be replicated and
cached.  Because of this, compute the delay in a non-standard way 
as follows:

Let C be the time that the data object was created at the publisher.
Let I be the time that the subscriber registers an interest for the data object.
Let T be the time that the data object is received at the subscriber (where 
the data object matches the interest registered at time I).

Then we look at the delay as follows:

(a) if C < I then we compute the delay as T-I.
(b) Otherwise, we compute the delay as T-C.

In other words, we are measuring the time it takes for the data object 
to arrive, starting at the time that both 1) the data object has been created, 
and 2) the request has been created.

This example highlights the two different modes that our system operates 
with, both push and pull. We may want to treat these delays 
separately, where we look at the average delay of only (a) and
average delay of only (b), since they are 2 different modes of operation.

Most systems that I'm aware of don't have both these modes. 


SCENGEN NOTE:

One potential issue w/ the current(now old) way we're generating
RWP mobility is that it is non-standard: typically RWP picks
a destination and a velocity (uniformly from a range), travels
to that destination, and then pauses for some period of time
(uniformly from a range). I believe "genrand.py" has all the nodes
change direction simultaneously, correct?

I was able to use a "scengen" ns2 mobility generator that
I've attached to successfully generate scen files which are
compatible with core. (I had to make 1 slight change to the
cc code to compile in our environment).

You edit the "scen-spec" to define "teams" of which can have
different mobility patterns. For example, you can easily do
the scenario the Srinivas was recommending by having 6
teams randomly move within their own grid.

I've attached the scen-spec file to generate a 1500x300m^2
RWP scenario, [1, 7] m/s velocity, [0,5] s pause time, 20 nodes,
250 m broadcast range.

Note that the scen file generated also specifies the node's initial
positions (core respects this).

I then created a new file in CORE and dropped 20 nodes by hand
(the positions don't matter since the scen file will move them),
added the wireless, and selected the scen file for the wireless
mobility settings (double clicking on the cloud). CORE expects the
nodes to start from n1..n20, but the generated scen file expects
nodes 0..19, so I added a bogus node (scengen creates mobility
for 21 nodes, and I removed the bogus 0 node).

I manually modified the .imn file to have the proper canvas dimensions,
since editing the UI did not seem to change it correctly.

Let me know if you have trouble, but this way you can easily create
RWP mobility at different sizes.

P.S. the CORE that Mingyoung sent out can start core mobility w/o
the need to explicitly press "play".

