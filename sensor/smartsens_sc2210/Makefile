ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../../mpp/Makefile.param
     include $(PARAM_FILE)
endif

STATIC_LIBRARY = libsns_sc2210.a
SHARED_LIBRARY = libsns_sc2210.so


# Define files
#------------------------------------------
SRCDIRS   = . ../sc_gpio

INSTALL_HEADERS :=

OUTPUT_APPLICATION_LIB_DIR = ../libs

# Define flags
#------------------------------------------
CFLAGS  += -I../sc_gpio -I../../../mpp/include  -I../../../mpp/src/hal/inc

LDFLAGS += -L../../../mpp/src/hal/lib

LOCAL_LIB = -lhal_gpio


# Include make rule file
#------------------------------------------
include ../usr_lib_rules.mk
