# SPDX-License-Identifier: MIT
# Copyright © 2019 William Budd

NAME = libjgrandson
VMAJOR = 1
VMINOR = 0
SONAME_VNONE = $(NAME).so
SONAME_VSHORT = $(SONAME_VNONE).$(VMAJOR)
SONAME_VLONG = $(SONAME_VSHORT).$(VMINOR)
ANAME = $(NAME).a
NAME_PREFIX = jg_
SYSTEM_HEADER = jgrandson.h

UNAME := $(shell uname)

INCLUDE_DIR = /usr/local/include
LIB_DIR = /usr/local/lib

CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
CFLAGS_SHARED = -fpic
CFLAGS_OPTIM = -O3 -flto
ifeq ($(UNAME), Linux)
CFLAGS_OPTIM += -fuse-linker-plugin
endif
ifeq ($(UNAME), Darwin)
LFLAGS_SHARED = -shared -Wl,-install_name,$(SONAME_VSHORT)
else
LFLAGS_SHARED = -shared -Wl,-z,relro,-z,now,-soname,$(SONAME_VSHORT)
endif
LFLAGS_OPTIM = -flto
ifeq ($(UNAME), Linux)
LFLAGS_OPTIM += -fuse-linker-plugin -fuse-ld=gold
endif
ARFLAGS = rcs

SRC_DIR = src
SRC_PATHS := $(wildcard $(SRC_DIR)/$(NAME_PREFIX)*.c)
SRC_NAMES = $(SRC_PATHS:$(SRC_DIR)/%=%)

OBJ_NAMES = $(SRC_NAMES:.c=.o)
SHARED_DIR = .shared_objects
STATIC_DIR = .static_objects
SHARED_PATHS = $(addprefix $(SHARED_DIR)/, $(OBJ_NAMES))
STATIC_PATHS = $(addprefix $(STATIC_DIR)/, $(OBJ_NAMES))

.PHONY: shared
shared: $(SONAME_VLONG)

$(SONAME_VLONG): $(SHARED_PATHS)
	$(CC) $(LFLAGS_SHARED) $(LFLAGS_OPTIM) $(SHARED_PATHS) -o $(SONAME_VLONG)

$(SHARED_DIR)/%.o: $(SRC_DIR)/%.c | $(SHARED_DIR)
	$(CC) $(CFLAGS) $(CFLAGS_SHARED) $(CFLAGS_OPTIM) -c $< -o $@

$(SHARED_DIR):
	mkdir $(SHARED_DIR)

.PHONY: static
static: $(ANAME)

$(ANAME): $(STATIC_PATHS)
	$(AR) $(ARFLAGS) $(ANAME) $(STATIC_PATHS)

$(STATIC_DIR)/%.o: $(SRC_DIR)/%.c | $(STATIC_DIR)
	$(CC) $(CFLAGS) $(CFLAGS_OPTIM) -c $< -o $@

$(STATIC_DIR):
	mkdir $(STATIC_DIR)

.PHONY: clean
clean:
	rm -rf $(SONAME_VLONG) $(ANAME) $(SHARED_DIR) $(STATIC_DIR)

.PHONY: install
install:
	cp $(SRC_DIR)/$(SYSTEM_HEADER) $(INCLUDE_DIR)/ && \
		cp $(SONAME_VLONG) $(LIB_DIR)/ && \
		ln -fs $(SONAME_VLONG) $(LIB_DIR)/$(SONAME_VSHORT) && \
		ln -fs $(SONAME_VSHORT) $(LIB_DIR)/$(SONAME_VNONE)

.PHONY: static_install
static_install:
	cp $(SRC_DIR)/$(SYSTEM_HEADER) $(INCLUDE_DIR)/ && \
		cp $(ANAME) $(LIB_DIR)/

.PHONY: uninstall
uninstall:
	rm $(INCLUDE_DIR)/$(SYSTEM_HEADER) && rm $(LIB_DIR)/$(NAME).*
