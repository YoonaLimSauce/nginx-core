.PHONY: all clean

MKDIR = mkdir
RM = rm
RMFLAGS = -fr

CC = gcc
AR = ar
ARFLAGS = crs

DIR_OBJS = objs
DIR_DEPS = deps
DIR_EXES = ../../../build/exes
DIR_LIBS = ../../../build/inc
DIRS = $(DIR_OBJS) $(DIR_DEPS) $(DIR_EXES) $(DIR_LIBS)
RMS = $(DIR_OBJS) $(DIR_DEPS)

EXE = 
ifneq ($(EXE), "")
RMS += $(EXE)
endif

LIB = libfoo.a
ifneq ($(LIB), "")
LIB := $(addprefix $(DIR_LIBS)/, $(LIB))
RMS += $(LIB)
endif

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
OBJS := $(addprefix $(DIR_OBJS)/, $(OBJS))
DEPS = $(SRCS:.c=.dep)
DEPS := $(addprefix $(DIR_DEPS)/, $(DEPS))

ifneq ($(EXE), "")
all: $(EXE)
endif

ifneq ($(LIB), "")
all: $(LIB)
endif

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
ifeq ("$(wildcard $(DIR_DEPS))", "")
DIR_DEPS_C := $(DIR_DEPS)
endif
endif

$(DIRS): 
	$(MKDIR) $@

$(EXE): $(DIR_EXES) $(OBJS)
	$(CC) -o $@ $(filter %.o, $^)

$(LIB): $(DIR_LIBS) $(OBJS)
	$(AR) $(ARFLAGS) $@ $(filter %.o, $^)

$(DIR_OBJS)/%.o: $(DIR_OBJS) %.c
	$(CC) -o $@ -c $(filter %.c, $^)

$(DIR_DEPS)/%.dep: $(DIR_DEPS) %.c
	@echo "Making $@..."
	@set -e; \
	$(RM) $(RMFLAGS) $@.tmp; \
	$(CC) -E -MM $(filter %.c, $^) > $@.tmp; \
	sed 's, \(\.*\)\.o[ :]*, objs/\1.o $@: , g' < $@.tmp > $@; \
	$(RM) $(RMFLAGS) $@.tmp

clean:
	$(RM) $(RMFLAGS) $(RMS)
