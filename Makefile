# SPDX-License-Identifier: MIT
# Copyright © 2019 William Budd

NAME = libjgrandson
VMAJOR = 0
VMINOR = 4
SONAME_VNONE = $(NAME).so
SONAME_VSHORT = $(SONAME_VNONE).$(VMAJOR)
SONAME_VLONG = $(SONAME_VSHORT).$(VMINOR)
NAME_PREFIX = jg_
SYSTEM_HEADER = jgrandson.h

INCLUDE_DIR = /usr/include
DLL_DIR = /usr/lib

CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11
CFLAGS_DLL = -fpic
CFLAGS_OPTIM = -O3 -flto -fuse-linker-plugin
LFLAGS_DLL = -shared -Wl,-z,relro,-z,now,-soname,$(SONAME_VSHORT)
LFLAGS_OPTIM = -flto -fuse-linker-plugin -fuse-ld=gold

SRC_DIR = src
SRC_PATHS := $(wildcard $(SRC_DIR)/$(NAME_PREFIX)*.c)
SRC_NAMES = $(SRC_PATHS:$(SRC_DIR)/%=%)

OBJ_DIR = .objects
OBJ_NAMES = $(SRC_NAMES:.c=.o)
OBJ_PATHS = $(addprefix $(OBJ_DIR)/, $(OBJ_NAMES))

.PHONY: optimized
optimized: $(SONAME_VLONG)

$(SONAME_VLONG): $(OBJ_PATHS)
	$(CC) $(LFLAGS_DLL) $(LFLAGS_OPTIM) $(OBJ_PATHS) -o $(SONAME_VLONG)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(CFLAGS_DLL) $(CFLAGS_OPTIM) -c $< -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

.PHONY: clean
clean:
	rm -rf $(SONAME_VLONG) $(OBJ_DIR)

.PHONY: install
install:
	cp $(SRC_DIR)/$(SYSTEM_HEADER) $(INCLUDE_DIR)/ && \
		cp $(SONAME_VLONG) $(DLL_DIR)/ && \
		ln -fs $(SONAME_VLONG) $(DLL_DIR)/$(SONAME_VSHORT) && \
		ln -fs $(SONAME_VSHORT) $(DLL_DIR)/$(SONAME_VNONE)

.PHONY: uninstall
uninstall:
	rm $(INCLUDE_DIR)/$(SYSTEM_HEADER) && rm $(DLL_DIR)/$(NAME).*
