ICE_DIR = /opt/homebrew/opt/ice

CXX = c++
CXXFLAGS = -std=c++14 -I. -I$(ICE_DIR)/include -DICE_CPP11_MAPPING
LDFLAGS = -L$(ICE_DIR)/lib -lIce++11 -lpthread

all: build_slice build_system build_passenger build_tram

comp: build_system build_passenger build_tram

build_slice:
	slice2cpp mpk.ice

build_system:
	$(CXX) $(CXXFLAGS) -c mpk.cpp system.cpp SystemI.cpp
	$(CXX) -o system mpk.o system.o SystemI.o $(LDFLAGS)

build_passenger:
	$(CXX) $(CXXFLAGS) -c mpk.cpp passenger.cpp PassengerI.cpp
	$(CXX) -o passenger mpk.o passenger.o PassengerI.o $(LDFLAGS)

build_tram:
	$(CXX) $(CXXFLAGS) -c mpk.cpp tram.cpp TramI.cpp
	$(CXX) -o tram mpk.o tram.o TramI.o $(LDFLAGS)

clean:
	rm -f *.o system passenger tram mpk.cpp mpk.h
