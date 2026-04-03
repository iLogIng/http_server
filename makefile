
CXX = g++
STD_VERSION = -std=c++17
BOOST_INCLUDE = /usr/include/
BOOST_LIB = /usr/bin/
FLAGES = -Wall -g

http_server: http_server.cpp
	$(CXX) $(STD_VERSION) $^ -L $(BOOST_LIB) -o $@ -lpthread

# = = = = = = =

.PHONY:
	execute
	clean

execute:
	@echo 'this server is running on 0.0.0.0:8080'
	./http_server 0.0.0.0 8080 . 1;
	

clean:
	rm http_server

