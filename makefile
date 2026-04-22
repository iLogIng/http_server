CXX = g++
STD_VERSION = -std=c++17

CXXFLAGS = ${STD_VERSION} -DBOOST_LOG_DYN_LINK -Wall -Wextra -g

# BOOST_INCLUDE_DIR = /usr/include
# BOOST_LIB_DIR = /usr/lib

BOOST_LIBS_LINK = -lboost_system -lboost_filesystem -lboost_thread \
					-lboost_log -lboost_log_setup \
					-lboost_program_options -lboost_json \
					-lpthread

TARGET = http_server

SRCS = src/*.cpp
INCLUDES = includes/*.hpp

$(TARGET): $(INCLUDES) $(SRCS)
	$(CXX) $(CXXFLAGS) $^ $(BOOST_LIBS_LINK) -o $@
# $(CXX) $(CXXFLAGS) $^ -I$(BOOST_INCLUDE_DIR) -L$(BOOST_LIB_DIR) $(BOOST_LIBS_LINK) -o $@

test: logger.test

logger.test: src/logger.hpp test/logger.test.cpp
	$(CXX) $(CXXFLAGS) $^ $(BOOST_LIBS_LINK) -lgtest -lgtest_main -o ./test/exec/logger.test
	./test/exec/logger.test

# = = = = = = =

.PHONY:
	execute
	clean

execute:
	@echo 'this server is running on 0.0.0.0:8080'
	./http_server --address 0.0.0.0 --port 8080 --doc_root . --threads 4 --timeout_seconds 30 --max_body_size 10485760
	

clean: clean-test
	rm http_server

clean-test:
	rm ./test/exec/logger_test
