#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32-doom

include $(IDF_PATH)/make/project.mk

CFLAGS += -Wno-nonnull
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -Wno-misleading-indentation
CFLAGS += -Wno-format-overflow
CFLAGS += -Wno-unused-const-variable
CFLAGS += -Wno-sizeof-pointer-div
CFLAGS += -Wno-duplicate-decl-specifier
CFLAGS += -Wno-dangling-else
