# DIR=$(shell pwd)
DIR=.
SRC_DIR=$(DIR)
DEPS_DIR=$(DIR)/deps
OBJ_DIR=$(DIR)/obj
UTIL_DIR=$(DIR)/utils
SERVER_PROGRAM=$(DIR)/server_program
CLIENT_PROGRAM=$(DIR)/client_program

## compile option
CC=g++
CFLAGS=-std=c++17 -g
EXTENSION=cpp
# INCLUDE=

## file path
# SRCS=$(wildcard $(SRC_DIR)/*.$(EXTENSION))
SRCS=ae.cpp anet.cpp buffer.cpp client.cpp server.cpp
OBJS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(OBJ_DIR)/%.o, $(SRCS))
DEPS=$(patsubst $(SRC_DIR)/%.$(EXTENSION), $(DEPS_DIR)/%.d, $(SRCS))

## compile target
.PHONY: all clean rebuild

all:srv cli

srv:$(OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_PROGRAM) $(SRCS) server_main.cpp

cli:$(OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_PROGRAM) $(SRCS) client_main.cpp

# $(DEPS_DIR)/%.d: $(SRC_DIR)/%.$(EXTENSION)
# 	@mkdir -p $(DEPS_DIR)
# 	$(CC) $(CFLAGS) $(INCLUDE) -MM $^ | sed 1's,^,$@ $(OBJ_DIR)/,' > $@

# include $(DEPS)

# $(OBJ_DIR)/%.o: $(SRC_DIR)/%.$(EXTENSION)
# 	@mkdir -p $(OBJ_DIR)
# 	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

# rebuild: clean all

clean:
	rm -rf $(OBJS) $(DEPS)
