TBBHOME = /Users/vilya/Code/3rdParty/tbb30_018oss

INCFLAGS = -I$(TBBHOME)/include
LIBFLAGS = -L$(TBBHOME)/lib
LIBS = -ltbb

CXXFLAGS = -O3 -Wall
CXX = g++


paigow: paigow.cpp
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(LIBFLAGS) $(LIBS) -o $@ $<


clean:
	rm -f paigow *.o

