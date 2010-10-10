# Note that you need to have the TBB environment variables set up before
# calling Make. You do this by sourcing the tbbvars.sh script that ships with
# TBB. For example, on 64-bit linux:
# 
#   source /opt/intel/tbb/3.0/bin/tbbvars.sh intel64

LIBS = -ltbb

CXXFLAGS = -O3 -Wall
CXX = g++


paigow: paigow.cpp
	$(CXX) $(CXXFLAGS) $(LIBS) -o $@ $<


clean:
	rm -f paigow *.o

