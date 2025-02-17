# Makefile

# Define the workspace folder path
WORKSPACE_FOLDER := $(CURDIR)

# Compiler
CXX = /usr/bin/g++
CC = gcc
AR = ar
CXXFLAGS = -g -std=c++11 -I$(WORKSPACE_FOLDER)/src -I$(WORKSPACE_FOLDER)/include -I/usr/include/fastdds -I/usr/include/fastcdr
CFLAGS = -I./include -Wall -Wextra -Wpedantic -g
LDFLAGS_CXX = -lstdc++ -lncurses -ltinfo -lm -lcjson -lfastcdr -lfastdds
LDFLAGS_CC = -L$(BUILD_DIR)
LIBS = -lauxfunc -lncurses -ltinfo -lm -lcjson

# Directory
SRC_DIR = src
BIN_DIR = bin
BUILD_DIR = build
LOG_DIR = log
GENERATED_DIR = $(WORKSPACE_FOLDER)/src/Generated

# Source files
SRC_BLACKBOARD = $(WORKSPACE_FOLDER)/src/blackBoard.cpp $(WORKSPACE_FOLDER)/src/obst_publisher.cpp $(WORKSPACE_FOLDER)/src/obst_subscriber.cpp $(WORKSPACE_FOLDER)/src/targ_publisher.cpp $(WORKSPACE_FOLDER)/src/targ_subscriber.cpp $(WORKSPACE_FOLDER)/src/auxfunc2.cpp $(WORKSPACE_FOLDER)/src/Generated/ObstaclesPubSubTypes.cxx $(WORKSPACE_FOLDER)/src/Generated/ObstaclesTypeObjectSupport.cxx $(WORKSPACE_FOLDER)/src/Generated/TargetsPubSubTypes.cxx $(WORKSPACE_FOLDER)/src/Generated/TargetsTypeObjectSupport.cxx

SRC_TARGET = $(WORKSPACE_FOLDER)/src/target.cpp $(WORKSPACE_FOLDER)/src/obst_publisher.cpp $(WORKSPACE_FOLDER)/src/obst_subscriber.cpp $(WORKSPACE_FOLDER)/src/targ_publisher.cpp $(WORKSPACE_FOLDER)/src/targ_subscriber.cpp $(WORKSPACE_FOLDER)/src/auxfunc2.cpp $(WORKSPACE_FOLDER)/src/Generated/ObstaclesPubSubTypes.cxx $(WORKSPACE_FOLDER)/src/Generated/ObstaclesTypeObjectSupport.cxx $(WORKSPACE_FOLDER)/src/Generated/TargetsPubSubTypes.cxx $(WORKSPACE_FOLDER)/src/Generated/TargetsTypeObjectSupport.cxx

SRC_OBSTACLE = $(WORKSPACE_FOLDER)/src/obstacle.cpp $(WORKSPACE_FOLDER)/src/obst_publisher.cpp $(WORKSPACE_FOLDER)/src/obst_subscriber.cpp $(WORKSPACE_FOLDER)/src/targ_publisher.cpp $(WORKSPACE_FOLDER)/src/targ_subscriber.cpp $(WORKSPACE_FOLDER)/src/auxfunc2.cpp $(WORKSPACE_FOLDER)/src/Generated/ObstaclesPubSubTypes.cxx $(WORKSPACE_FOLDER)/src/Generated/ObstaclesTypeObjectSupport.cxx $(WORKSPACE_FOLDER)/src/Generated/TargetsPubSubTypes.cxx $(WORKSPACE_FOLDER)/src/Generated/TargetsTypeObjectSupport.cxx

# Executables and Objects
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
EXECUTABLES = $(patsubst $(SRC_DIR)/%.c,%,$(filter-out $(SRC_DIR)/auxfunc.c,$(SRC_FILES)))
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(filter-out $(SRC_DIR)/auxfunc.c,$(SRC_FILES)))
AUX_OBJ = $(BUILD_DIR)/auxfunc.o
LIBAUX = $(BUILD_DIR)/libauxfunc.a
BINS = $(addprefix $(BIN_DIR)/,$(EXECUTABLES))

# Output binaries
BIN_BLACKBOARD = $(WORKSPACE_FOLDER)/bin/blackBoard
BIN_TARGET = $(WORKSPACE_FOLDER)/bin/target
BIN_OBSTACLE = $(WORKSPACE_FOLDER)/bin/obstacle

.PHONY: all
all: directories $(LIBAUX) $(BINS) compileBlackboard compileTarget compileObstacle

.PHONY: directories
directories:
	mkdir -p $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR) $(GENERATED_DIR)
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log
	fastddsgen $(WORKSPACE_FOLDER)/src/Obstacles.idl -d $(GENERATED_DIR)
	fastddsgen $(WORKSPACE_FOLDER)/src/Targets.idl -d $(GENERATED_DIR)
	mv $(GENERATED_DIR)/src/* $(GENERATED_DIR)
	rm -rf $(GENERATED_DIR)/src


# Compile static library
$(LIBAUX): $(AUX_OBJ)
	$(AR) rcs $@ $^

# Compile executables
$(BIN_DIR)/%: $(BUILD_DIR)/%.o $(LIBAUX)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_CC) $(LIBS)

# Create object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ targets
compileBlackboard:
	$(CXX) $(CXXFLAGS) $(SRC_BLACKBOARD) -o $(BIN_BLACKBOARD) $(LDFLAGS_CXX)

compileTarget:
	$(CXX) $(CXXFLAGS) $(SRC_TARGET) -o $(BIN_TARGET) $(LDFLAGS_CXX)

compileObstacle:
	$(CXX) $(CXXFLAGS) $(SRC_OBSTACLE) -o $(BIN_OBSTACLE) $(LDFLAGS_CXX)

# Clean up logs
.PHONY: clean-logs
clean-logs:
	rm -f $(LOG_DIR)/*.txt
	rm -f $(LOG_DIR)/*.log

# Clean up all
.PHONY: clean
clean:
	rm -f $(BIN_BLACKBOARD) $(BIN_TARGET) $(BIN_OBSTACLE)
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(LOG_DIR) $(GENERATED_DIR)
	@echo "Cleaned up!"