# Hey Emacs, this is a -*- makefile -*-
# Copyright (C) 2012 Y.Okamura
# https://gist.github.com/2343121
# MIT License

# Install prefix
PREFIX = /usr/local

# Target Executable file
TARGET = avrwrite

# C source code
SRC = debug.c

# CPP source code
CXXSRC = main.cpp avrinfo.cpp serialbootloader.cpp serialport.cpp writer.cpp

# Common Flags
CPPFLAGS += -Wall -Wextra -pipe
COMMON_FLAGS += 

# C compiler
CC = gcc
CFLAGS +=

# C++ compiler
CXX = g++
CXXFLAGS +=

# AR
AR = ar
LIB = lib$(TARGET).a
ARFLAGS = urcv

# Linker
LD = gcc
LDFLAGS += -lstdc++ -lm

# Windows Executable file's extension
ifdef WINDIR
  EXE = $(TARGET).exe
  LDFLAGS += -static
  UNAME = Windows
else

# Unix like OS
UNAME = $(strip $(shell uname))
CPPFLAGS += 
LDFLAGS  += 
EXE = $(TARGET)

ifeq ($(UNAME), Darwin) # Mac OS X
  LDFLAGS += -liconv
endif
ifeq ($(UNAME), Linux) # Linux
  LDFLAGS += 
endif

endif


# For Emacs
ifdef EMACS
ifeq ($(CC),clang)
  CPPFLAGS += -fno-color-diagnostics
endif
endif

# Debug Options
ifdef RELEASE
  CPPFLAGS += -Os -mtune=native
else
  CPPFLAGS += -g -O0
endif

# copy commong flags
CPPFLAGS += $(COMMON_FLAGS)
LDFLAGS += $(COMMON_FLAGS)

# generate dependence file
OBJDIR = ./objs-$(UNAME)
CPPFLAGS += -MMD -MP -MF $(OBJDIR)/$(@F).d
OBJ += $(SRC:%.c=$(OBJDIR)/%.o) $(CXXSRC:%.cpp=$(OBJDIR)/%.o)
DEPENDS = $(SRC:%.c=$(OBJDIR)/%.o.d) $(CXXSRC:%.cpp=$(OBJDIR)/%.o.d)

# Add target to build library
all:exe

exe:$(EXE)
lib:$(LIB)

$(EXE):$(OBJ) $(DEPEND_TARGET)
	$(LD) -o $@ $(OBJ) $(LDFLAGS) 
$(LIB):$(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

$(OBJDIR)/%.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
$(OBJDIR)/%.res : %.rc
	windres $< -O coff -o $@ $(RESFLAGS)

clean:
	-rm $(OBJ)
	-rm $(DEPENDS)
	-rmdir $(OBJDIR)
	-rm $(EXE)
	-rm $(LIB)
#   -rm -r $(OBJDIR)

install:
	install -bdSs $(EXE) $(PREFIX)/bin

.PHONY:clean all exe lib install

-include $(shell mkdir $(OBJDIR) 2>/dev/null) $(wildcard $(OBJDIR)/*.d)
