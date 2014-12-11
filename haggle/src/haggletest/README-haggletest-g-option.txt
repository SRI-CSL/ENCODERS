
This README discusses the new haggletest option '-g <filename>'.
Using '-g' is primarily used by the testing scripts to bypass cli options and refer to a file for its options.
It should be noted, most cli options will still function, but the overal syntax will be different.

haggletest [options] -g inputfilename appname
haggletest -f filelog -g sample.input app1


An included sample.input file contains:
(delay from start in seconds)/action/attributes/file to send
============================================================
0,pub,"jim=awesome:4;sam=hello;","outputfile"
5,pub,"jim=awefull;sam=kitty:2;",""

Thus, at time=0, we publish "jim=awesome:4" and "sam=hello", with file "outputfile"
Again at time=5 (seconds), we publish a single DO "jim=awefull" and "sam=kitty" attributes, but no file (no file attached to DO.

Time can be a float (3.5, 10.01, etc) value.

Since this is to support testing, the following options are always on:
pub - even though specified, we only support publication
-c -d -k:  These options are enabled (always).

-k option uses symbolic links, as large scale simulations with large files drag the system response.

Since we specify one output log for the action, the results look like:
Action,ObjectId,CreateTime,ReqestTime,ArrivalTime,DelayRelToRequest,DelayRelToCreation,DataLen
Published,fc90793c283d4f677f9258c78c88e414d60d7c38,1363634157.930026,0,0,0,0,28330
Published,c74e3898df0dad69b8529949a989bde362df52c3,1363634162.931496,0,0,0,0,0

