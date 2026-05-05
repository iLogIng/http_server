CXX = g++
STD_VERSION = -std=c++17

CXXFLAGS = ${STD_VERSION} -DBOOST_LOG_DYN_LINK -Wall -Wextra -g

BOOST_LIBS_LINK = -lboost_system -lboost_filesystem -lboost_thread \
					-lboost_log -lboost_log_setup \
					-lboost_program_options -lboost_json \
					-lpthread

TARGET = http_server

SRCS = src/*.cpp
INCLUDES = includes/*.hpp

$(TARGET): $(INCLUDES) $(SRCS)
	$(CXX) $(CXXFLAGS) $^ $(BOOST_LIBS_LINK) -o $@

test:
	$(MAKE) -C test all-test-run

# = = = = = = =

.PHONY:
	test clean clean-test execute

execute:
	@echo 'this server is running on 0.0.0.0:8080'
	./http_server --address 0.0.0.0 --port 8080 --doc_root . --threads 4 --timeout_seconds 30 --max_body_size 10485760
	

clean: clean-test
	rm -f http_server

clean-test:
	$(MAKE) -C test clean-test
