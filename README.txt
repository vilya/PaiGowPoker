This is my entry for the Intel Threading Challenge, Phase 2, Problem 3,
Amateur Level: Reduced-deck Pai Gow Poker


Pre-requisites
==============

- Threading Building Blocks (TBB). Tested with 3.0, may work(?) with earlier
  versions.
- g++. Tested with 4.2.1 on OS X and 4.4.1 on Linux. Should work with other
  versions too.


To build it
===========

- Make sure the environment variables for 64-bit TBB are set up (e.g. by
  running the tbbvars.sh script that ships with TBB).
- Run make

That should be it. The executable it generates is called 'paigow'. It
compiles cleanly on the OS X (Snow Leopard, 64-bit) and Linux (Ubuntu 9.10,
64-bit) machines that I've tested.


To run it
=========

  ./paigow <infile>

(as in the problem description).

The output includes a print out of the time taken. This includes the time to
read the input file and to perform all the calculations, but not the time to
print out the results (I hope that's correct for this problem).

