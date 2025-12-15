CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

all: twmailer-client twmailer-server

twmailer-client: twmailer-client.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o twmailer-client twmailer-client.cpp

twmailer-server: twmailer-server.cpp common.hpp
	$(CXX) $(CXXFLAGS) -o twmailer-server twmailer-server.cpp

clean:
	rm -f twmailer-client twmailer-server
