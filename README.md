<!--
[![Build Status](https://travis-ci.org/SRI-CSL/ENCODERS.svg?branch=master)](https://travis-ci.org/SRI-CSL/ENCODERS)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/7492/badge.svg)](https://scan.coverity.com/projects/7492)
[![Coverage Status](https://coveralls.io/repos/github/SRI-CSL/ENCODERS/badge.svg?branch=master)](https://coveralls.io/github/SRI-CSL/ENCODERS?branch=master)
--> 
ENCODERS (Edge Networking with Content-Oriented Declarative Enhanced Routing and Storage) is SRI’s content-based networking solution that provides network services and transport architectures required for efficient, transparent distribution of content in mobile ad hoc networks. These services are application-independent and network-agnostic with the goal of reducing latency and increasing the effective throughput of content for warfighters at the tactical edge. ENCODERS is a search-based data-dissemination framework designed for mobile, opportunistic communication environments. ENCODERS software was developed for DARPA’s Content-Based Mobile Edge Networking (CBMEN) Program.

SRI started with the Haggle open-source code base, which provides underlying functionality for neighbor discovery and basic protocols, among other things. We made major improvements in it, including improving performance in mobile networks and extensions for utility-based dissemination and cache management, semantic annotations, and security.

Applications communicate with ENCODERS via data objects that contain both metadata and content. Metadata includes both content description and description of application interest. This separation of metadata from content allows the selective distribution of content based on interest matching and is a key feature supporting the efficient use of bandwidth and low latency in ENCODERS (because content is much larger in size than the metadata that describes it).

Below the Haggle framework is the network stack. This includes a traditional UDP/TCP/IP network layer as well as MAC and physical layers. ENCODERS uses UDP to connect to local applications and UDP/TCP to connect to peers.

Like Haggle, ENCODERS is a search-based data-dissemination framework designed for mobile, opportunistic communication environments. This search-based approach is used for resolution (mapping data to interested receivers) and prioritization of sending and receiving data during encounters between nodes. Haggle provides underlying functionality for neighbor discovery, basic protocols, data object representation and storage, and basic interest resolution, thus removing the need to implement such features in applications.

The ENCODERS architecture is event-driven, modular, and layer-less, which provides flexibility and scalability. Central in the architecture is the kernel. It implements an event queue, over which managers that implement the functional logic communicate. Managers are responsible for specific tasks such as managing communication interfaces, encapsulating a set of protocols, and forwarding content. The managers interact only by producing and consuming events. This makes it easy to add, replace, or remove managers, as they do not directly interact with each other.

This git repository contains the CBMEN ENCODERS code and related
documents under development by the SRI team. For integration testing,
it also contains the CBMEN code and documents from Drexel University. 

We have added various pieces of functionality to haggle. For documentation and changelogs related to each feature, please browse the following files:
```
    docs/README-feature.txt
    docs/changes-feature.txt
```
Where 'feature' is one of the following:
```
    master (changes to vanilla haggle)
    caching
    ucaching
    coding
    direct
    security
    semantics
    udp
    1hopf
    memdb
    memdb-constraint
    sendpriority
    udissemination
    ucaching
```

SRI Team
--------

* Dr. Mark-Oliver Stehr (Project Leader)
* Dr. Carolyn Talcott
* Dr. Minyoung Kim
* Dr. Ashish Gehani
* Hasnain Lakhani
* Tim McCarthy
* Dr. David Wilkins

Collaborators 
-------------

* Prof. J.J. Gercia-Luna-Aceves, UCSC
* Prof. Hamid Sadjadpour, UCSC
* Prof. Mario Gerla, UCLA
* Sam Wood, UCSC
* James Mathewson, UCSC
* Joshua Joy, UCLA
* Yu-Ting Yu, UCLA
* David Anhalt, SET Corp.
* Ralph Costantini, SET Corp.
* Dr. Hua Li, SET Corp.

Students and Visitors  
---------------------

* Je-Min Kim, Sungkyunkwan University
* Jong-Seok Choi, Kyungpook National University
* Sathiya Prabhu Kumar, CNAM
* Sylvain Lefebvre, CNAM
* Françoise Sailhan, CNAM
* Dawood Tariq, Lahore University
* Rizwan Asghar, University of Trento

Acknowledgements  
----------------

* We would like to thank the entire SAIC team (led by Dr. William Merrill and George Weston) for providing the development platform and successfully demonstrating an integrated CBMEN system based on the SRI ENCODERS architecture at Ft. AP Hill, VA, and MIT Lincoln Labs (the team led by Dr. Andrew Worthen) for their independent evaluation of the performance in testbed experiments and in the field.

Papers
------

* Hasanat Kazmi, Hasnain Lakhani, Ashish Gehani, Rashid Tahir, Fareed Zaffar, To Route or To Secure: Tradeoffs in ICNs over MANETs, 15th IEEE International Symposium on Network Computing and Applications (NCA), IEEE Computer Society, 2016. 
* J. Joy, M. Gerla, Y-T. Yu, A. Gehani, H. Lakhani, M. Kim, “Context-Aware Cache Coding for Mobile Information-Centric Networks”, 10th ACM International Conference on Distributed and Event-Based Systems (DEBS’16), Jun. 2016, Irvine, CA, USA.  
* Mariana Raykova, Hasnain Lakhani, Hasanat Kazmi, and Ashish Gehani, Decentralized Authorization and Privacy-Enhanced Routing for Information-Centric Networks, 31st Annual Computer Security Applications Conference (ACSAC), 2015.  
* S. Kumar, S. Lefebvre, M. Kim, M-O. Stehr, “Priority Register: Application-defined Replacement Orderings for Ad Hoc Reconciliation”, 3rd Workshop on Scalable Cloud Data Management (SCDM’15), in conjunction with IEEE BigData Conference, Oct. 2015, Santa Clara, CA, USA.  
* S. Wood, J. Mathewson, J. Joy, M-O. Stehr, M. Kim, A. Gehani, M. Gerla, H. Sadjadpour, J.J. Garcia-Luna-Aceves, “ICEMAN: A Practical Architecture for Situational Awareness at the Network Edge”, Logic, Rewriting, and Concurrency, LNCS Volume 9200, pp 617-631, Sept. 2015, Urbana, IL, USA.  
* H. Lakhani, T. McCarthy, M. Kim, D. Wilkins, S. Wood, “Evaluation of a Delay-Tolerant ICN Architecture”, 7th Int. Conf. Ubiquitous and Future Networks (ICUFN’15), Jul. 2015, Sapporo, Japan.
* H. Li, R. Costantini, D. Anhalt, R. Alonso, M-O. Stehr, C. Talcott, M. Kim, T. McCarthy, S. Wood, “Adaptive Interest Modeling Improves Content Services at the Network Edge”, 33rd IEEE Military Communications Conference (MILCOM’14), Oct. 2014, Baltimore, MD, USA.  
* S. Wood, J. Mathewson, J. Joy, M-O. Stehr, M. Kim, A. Gehani, M. Gerla, H. Sadjadpour, J.J. Garcia-Luna-Aceves, “ICEMAN: A System for Efficient, Robust and Secure Situational Awareness at the Network Edge”, 32nd IEEE Military Communications Conference (MILCOM’13), Nov. 2013, San Diego, CA, USA.  
* M. Kim, J-M. Kim, M-O. Stehr, A. Gehani, D. Tariq, J-S. Kim, “Maximizing Availability of Content in Disruptive Environments by Cross-Layer Optimization”, 28th ACM Symposium on Applied Computing (SAC’13), Mar. 2013, Coimbra, Portugal.  
* Françoise Sailhan, Mark-Oliver Stehr: Folding and Unfolding Bloom Filters: An Off-Line Planning and On-Line Optimization Problem. GreenCom 2012: 34-41

Reports
-------

Please refer to [documentation](../../wiki/documentation) for final report, configuration manual, and design descriptions. 



