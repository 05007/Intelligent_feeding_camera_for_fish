ifeq ($(PARAM_FILE), )
     PARAM_FILE := ../Makefile.param
     include $(PARAM_FILE)
endif


SRC := $(wildcard *.c)

OBJ := $(SRC:%.c=%.o)

INC_FLAGS := -I$(REL_INC)
CFLAGS += -Wall -g $(INC_FLAGS)
CFLAGS += $(LIBS_CFLAGS)

LIBS += $(REL_LIB)/libmpi.a
LIBS += $(HAL_LIBS)
LIBS += -Wall -g -lpthread -dl -lm -lrt -lstdc++
CFLAGS += $(LIBS_CFLAGS)

CFLAGS += -lm

include $(PWD)/Make.$(OSTYPE)
