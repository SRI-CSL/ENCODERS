import "HaggleNode";

module TwoNodesNetwork
       submodules:
		node0: HaggleNode;
		node1: HaggleNode;
       connections nocheck:
       		node0.out --> node1.in;
		node0.in <-- node1.out;
endmodule

network twonodes : TwoNodesNetwork
endnetwork
