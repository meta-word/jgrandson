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

INCLUDE_DIR = /usr/include
LIB_DIR = /usr/lib

CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
CFLAGS_SHARED = -fpic
CFLAGS_OPTIM = -O3 -flto -fuse-linker-plugin
LFLAGS_SHARED = -shared -Wl,-z,relro,-z,now,-soname,$(SONAME_VSHORT)
LFLAGS_OPTIM = -flto -fuse-linker-plugin -fuse-ld=gold
ARFLAGS = rcs

SRC_DIR = src
SRC_PATHS := $(wildcard $(SRC_DIR)/$(NAME_PREFIX)*.c)
SRC_NAMES = $(SRC_PATHS:$(SRC_DIR)/%=%)

OBJ_DIR = .objects
OBJ_NAMES = $(SRC_NAMES:.c=.o)
OBJ_PATHS = $(addprefix $(OBJ_DIR)/, $(OBJ_NAMES))

.PHONY: shared
shared: $(SONAME_VLONG)

$(SONAME_VLONG): $(OBJ_PATHS)
	$(CC) $(LFLAGS_SHARED) $(LFLAGS_OPTIM) $(OBJ_PATHS) -o $(SONAME_VLONG)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(CFLAGS_SHARED) $(CFLAGS_OPTIM) -c $< -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

.PHONY: static
static: $(ANAME)

$(ANAME): $(OBJ_PATHS)
	$(AR) $(ARFLAGS) $(ANAME) $(OBJ_PATHS)

.PHONY: clean
clean:
	rm -rf $(SONAME_VLONG) $(ANAME) $(OBJ_DIR)

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
