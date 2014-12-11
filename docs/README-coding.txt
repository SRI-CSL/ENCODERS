Network Coding and Fragmentation
================================

The coding branch contains the implementation of network coding
(inspired by CodeTorrent) and fragmentation. For the network coding
and fragmentation implementation, event proxying is used to intercept
when a data object SEND event occurs. A decision is made whether to
network code, fragment, or perform neither upon the DataObject. This
then results in either a specialized SEND NETWORKCODING or SEND
FRAGMENTATION event based upon content and context. A network-coded
block or fragment is sent to the peer and upon successful send the
DataObject SEND event is added again so that the block or fragment is
continuously sent until the peer cancels its interest by sending a
REJECT or REJECT2 code to indicate the peer has enough network coded
blocks or fragments to reconstruct the DataObject.

In phase 2, context aware network coding based on loss rate estimation
is added. With context aware network coding, fragmentation is used by 
default for all data objects targeting nodes with low loss rate. 
Network coding is enabled for a data object in two conditions: (1) 
sending side: when the loss rate of the link to the target node is 
higher than a given threshold, (2) receiving side: when a coded block 
of the data object is received. The remaining fragments of fragmentation
is automatically converted to coding blocks when network coding is 
enabled. The loss rate estimation is maintained by a newly added manager
LossEstimateManager, which monitors beacon events that are raised by
ConnectivityEthernet when a Ethernet beacon is received and perform 
periodic loss estimation for all known links accordingly.

Configuration Options
=====================

Network Coding
--------------

Network Coding provides itself useful in several scenarios, though it
is not always advantageous to have network-coded data objects. To this
end network coding is content-aware, currently in a limited sense
making the decision whether to network-code based on file size
only. Additionally, a thin caching layer is provided to speed up
network coding operations for encoding and decoding for the encoding
and decoding services as well as the network coded blocks.


Below is a typical excerpt from config.xml which enables network coding:

        <NetworkCodingManager enable_network_coding="true" 
            enable_forwarding="true" node_desc_update_on_reconstruction="true"
            max_age_decoder="300" max_age_block="300"
	    resend_delay="0" resend_reconstructed_delay="10.0"
	    delay_delete_networkcodedblocks="300.0" delay_delete_reconstructed_networkcodedblocks="10.0" 
	    min_network_coding_file_size="32769" block_size="32768"
	    number_blocks_per_dataobject="1">
        </NetworkCodingManager>

* enable_network_coding - setting to false disables network coding no
  matter the content or context.

* enable_forwarding - setting to false disables forwarding of network
  coded blocks, which can reduce the routing overhead of network coding,
  but may make it more difficult for the receiver to reconstruct the content.

* max_age_decoder - max age of network decoder state.

* max_age_block - max age of network coded blocks in the caching layer
  before being discarded.

* resend_delay - determines the max rate at which an event to send a new
  network coded block is inserted into the ForwardingManager. This can
  be set to 0 if the underlying protocols have their own rate
  limit/control.

* resend_reconstructed_delay - refers to the delay for creating send
  events for the parent data object upon reconstruction. This can 
  be used to delay the reencoding process.

* delay_delete_networkcodedblocks - maximum lifetime of blocks. This
  is used by the decoder and by intermediate nodes to purge old blocks.

* delay_delete_reconstructed_networkcodedblocks - maximum lifetime of
  blocks that have been used to reconstruct a data object. This will typically
  be shorter than the previous lifetime because the block is redundant.

* min_network_coding_file_size - the minimum file size for the data
  object for it to be network coded.

* block_size - is the block parameter used when creating network coded
  blocks.

* number_blocks_per_dataobject - refers to the number of blocks that
  will be included in the network-coded block data object. This is an
  experimental parameter, the recommended setting is 1 (default).

Enabling encoding and decoding on specific nodes
------------------------------------------------

Certain nodes can be specified to perform network coding as long as
the other conditions (such as min_network_coding_file_size) are
met. Both source_encoding_whitelist and target_encoding_whitelist
accept a comma-separated list of node names. Nodes matching the names
in the lists are the only approved nodes which can perform encoding
(source_encoding_whitelist) and decoding (target_encoding_whitelist)
respectively.

Below is a typical excerpt from config.xml which selectively enables
selective network coding:

        <NetworkCodingManager enable_network_coding="true" 
            source_encoding_whitelist="n1,n2,n3" target_encoding_whitelist="n5,n7,n8"
            enable_forwarding="true" node_desc_update_on_reconstruction="true"
            max_age_decoder="300" max_age_block="300"
	    resend_delay="0" resend_reconstructed_delay="10.0"
	    delay_delete_networkcodedblocks="300.0" delay_delete_reconstructed_networkcodedblocks="10.0" 
	    min_network_coding_file_size="32769" block_size="32768"
	    number_blocks_per_dataobject="1">
        </NetworkCodingManager>

* source_encoding_whitelist - comma separated values of node names
  which are allowed to encode content

* target_encoding_whitelist - comma separated values of node names
  which are allowed to decode content


Fragmentation
-------------

Fragmentation is symmetric to network coding for the options
available. Similar to network coding there is a thin caching layer for
fragmentation that stores fragments and helps with the encoding and
decoding of fragments.

Below is a typical excerpt from config.xml which enables fragmentation:

        <FragmentationManager enable_fragmentation="true" 
            enable_forwarding="true" node_desc_update_on_reconstruction="true"
	    max_age_fragment="300" max_age_decoder="300"
            resend_delay="0" resend_reconstructed_delay="60.0" 
            delay_delete_fragments="300.0" delay_delete_reconstructed_fragments="60.0"
            min_fragmentation_file_size="32769" fragment_size="32768"
            number_fragments_per_dataobject="1">        
        </FragmentationManager>


* enable_fragmentation - setting to false disables fragmentation no
  matter the content or context

* enable_forwarding - setting to false disables forwarding of fragments, 
  which can reduce the routing overhead, but may make it more difficult 
  for the receiver to reconstruct the content.

* max_age_fragment - max age of fragmentation decoder state.

* max_age_decoder - max age of network coded blocks in the caching layer
  before being discarded.

* delay_delete_fragments - maximum lifetime of fragments. This
  is used by the decoder and by intermediate nodes to purge old fragments.

* delay_delete_reconstructed_fragments - maximum lifetime of
  blocks that have been used to reconstruct a data object. This will typically
  be shorter than the previous lifetime because the fragment is redundant,
  and can be regenerated with a relative small cost.

* resend_delay - determines the rate at which an event to send a new
  fragment is inserted into the ForwardingManager. This can
  be set to 0 if the underlying protocols have their own rate
  limit/control.

* resend_reconstructed_delay - refers to the delay for creating
  send events for the parent data object. This can be quite long
  because refragmenting doe not generate new information (as opposed to
  random reencoding).

* min_fragmentation_file_size - the minimum file size for the
  DataObject for it to be fragmented

* fragment_size - is the fragment size when creating fragments

* number_fragments_per_dataobject - refers to the number of fragments
  that will be included in the fragment data object.  This is an
  experimental parameter, the recommended setting is 1 (default).


Enabling fragmenting and reassembly on specific nodes
-----------------------------------------------------

As in the case of network coding, certain nodes can be specified to
perform fragmentation as long as the other conditions (such as
min_fragmentation_file_size ) are met. Again, both
source_fragmentation_whitelist and target_fragmentation_whitelist
accept a comma-separated list of node names. Nodes matching the names
in the lists are the only approved nodes which can perform
fragmentation (source_fragmentation_whitelist) and assembly
(target_fragmentation_whitelist) respectively.


Below is a typical excerpt from config.xml which selectively enables
fragmentation:

        <FragmentationManager enable_fragmentation="true" 
            source_fragmentation_whitelist="*" target_fragmentation_whitelist="*"
            enable_forwarding="true" node_desc_update_on_reconstruction="true"
	    max_age_fragment="300" max_age_decoder="300"
            resend_delay="0" resend_reconstructed_delay="60.0" 
            delay_delete_fragments="300.0" delay_delete_reconstructed_fragments="60.0"
            min_fragmentation_file_size="32769" fragment_size="32768"
            number_fragments_per_dataobject="1">        
        </FragmentationManager>

* source_fragmentation_whitelist - comma separated values of node
  names which are allowed to fragment content

* target_fragmentation_whitelist - comma separated values of node
  names which are allowed to assemble content


Combining Fragmentation and Network Coding
------------------------------------------

The combination of fragmentation and network coding allows data
objects to consist of multiple generations which are then network
coded. The data object becomes split into fragments and each fragment
is then network-coded. To reconstruct the data object each generation
must first be decoded then each generation assembled forming the
original DataObject.


The parameters are the same as above. Below is a typical excerpt from
config.xml which enables fragmentation and network coding:

        <FragmentationManager enable_fragmentation="true" 
            enable_forwarding="true" node_desc_update_on_reconstruction="true"
	    max_age_fragment="300" max_age_decoder="300"
            resend_delay="0" resend_reconstructed_delay="60.0" 
            delay_delete_fragments="300.0" delay_delete_reconstructed_fragments="60.0"
            min_fragmentation_file_size="1048577" fragment_size="1048576"
            number_fragments_per_dataobject="1">        
        </FragmentationManager>
        <NetworkCodingManager enable_network_coding="true"
            enable_forwarding="true" node_desc_update_on_reconstruction="true"
            max_age_decoder="300" max_age_block="300"
	    resend_delay="0" resend_reconstructed_delay="1.0"
	    delay_delete_networkcodedblocks="300.0" delay_delete_reconstructed_networkcodedblocks="10.0" 
	    min_network_coding_file_size="32769" block_size="32768"
	    number_blocks_per_dataobject="1">
        </NetworkCodingManager>

With this configuration a 10MB file is broken down into 1MB fragments
which are in turn network-coded into 32K blocks.

Loss Estimation 
---------------

The loss estimation manager periodically assesses the loss rate of a link. 
A sample config is listed below.

	<LossEstimateManager>
            <PeriodicLossEstimate interval="5"/>
            <NetworkCodingTrigger loss_rate_threshold="0.1"/>
    	</LossEstimateManager>

The above indicates that the loss rate is estimated every 5 seconds. The
loss rate threshold for enabling network coding is 0.1, which means when the
loss rate for a given link is higher than 0.1 network coding will be enabled.
This setting is compatible with pure network coding or pure fragmentation. 
To use pure fragmentation, the loss rate threshold should be set to 1.0. To
use pure network coding, the loss rate threshold should be set to 0.0.

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/CodingAndFragmentation
