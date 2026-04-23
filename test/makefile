CXX = g++
STD_VERSION = -std=c++17

CXXFLAGS = ${STD_VERSION} -DBOOST_LOG_DYN_LINK -Wall -Wextra -g

BOOST_LIBS = -lboost_system -lboost_filesystem -lboost_thread \
             -lboost_log -lboost_log_setup \
             -lboost_program_options -lboost_json -lpthread

GTEST_LIBS = -lgtest -lgtest_main

ROOT = ..

EXEC_DIR = $(ROOT)/test/exec

# 源文件路径
SRC_UTILS   = $(ROOT)/src/utils.cpp
SRC_ROUTER  = $(ROOT)/src/router.cpp
SRC_CONFIG  = $(ROOT)/src/config.cpp

# ============================================================
# 确保 exec 目录存在
# ============================================================
$(EXEC_DIR):
	@mkdir -p $(EXEC_DIR)

# ============================================================
# 各测试构建目标
# ============================================================

# logger — 纯头文件模块，无需链接 src/*.cpp
test_logger: test_logger.cpp $(ROOT)/includes/logger.hpp | $(EXEC_DIR)
	$(CXX) $(CXXFLAGS) $< $(BOOST_LIBS) $(GTEST_LIBS) -o $(EXEC_DIR)/$@

# utils
test_utils: test_utils.cpp $(SRC_UTILS) | $(EXEC_DIR)
	$(CXX) $(CXXFLAGS) $^ $(BOOST_LIBS) $(GTEST_LIBS) -o $(EXEC_DIR)/$@

# router
test_router: test_router.cpp $(SRC_ROUTER) | $(EXEC_DIR)
	$(CXX) $(CXXFLAGS) $^ $(BOOST_LIBS) $(GTEST_LIBS) -o $(EXEC_DIR)/$@

# config
test_config: test_config.cpp $(SRC_CONFIG) | $(EXEC_DIR)
	$(CXX) $(CXXFLAGS) $^ $(BOOST_LIBS) $(GTEST_LIBS) -o $(EXEC_DIR)/$@

# ============================================================
# 运行单个测试（构建 + 执行）
# ============================================================

test_logger.run: test_logger
	$(EXEC_DIR)/test_logger

test_utils.run: test_utils
	$(EXEC_DIR)/test_utils

test_router.run: test_router
	$(EXEC_DIR)/test_router

test_config.run: test_config
	$(EXEC_DIR)/test_config

# ============================================================
# 批量目标
# ============================================================

.PHONY: all-test all-test-run clean-test

all-test: test_logger test_utils test_router test_config

all-test-run: all-test
	@echo "=== Running all tests ==="
	@$(EXEC_DIR)/test_logger
	@$(EXEC_DIR)/test_utils
	@$(EXEC_DIR)/test_router
	@$(EXEC_DIR)/test_config
	@echo "=== All tests passed ==="

clean-test:
	rm -f $(EXEC_DIR)/test_logger
	rm -f $(EXEC_DIR)/test_utils
	rm -f $(EXEC_DIR)/test_router
	rm -f $(EXEC_DIR)/test_config
