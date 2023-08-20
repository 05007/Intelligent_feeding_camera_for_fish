
include ../Makefile.param

#=============main======================================
#程序入口
CFLAGS += -I./
CXXFLAGS += -Wno-psabi
SMP_SRCS += $(wildcard ./*.c)
SMP_CXXSRCS += $(wildcard ./*.cpp)
SMP_SRCS := $(wildcard $(ARM_ARCH)/*.c)
MPI_LDFLAGS += -pthread
MPI_LDFLAGS += -lm

# #=============mpp库======================================
# #调用智兴公司提供的mpp库
# #lib vio common
# CFLAGS += -I../vio/smp
# SMP_SRCS += $(wildcard ../vio/smp/*.c)

# ifeq ($(MPI_SO_FLAG), y)
#   ## use libxx.so
#   MPI_LDFLAGS += -lnpu
#   MPI_LDFLAGS += -lrcu
# else
#   ## use libxx.a
#   MPI_LIBS += $(REL_LIB)/libnpu.a
#   MPI_LIBS += $(REL_LIB)/librcu.a
# endif

# # lib SVP
# CFLAGS += -I../svp/common
# CFLAGS += -I../svp/npu/sample_npu_software
# SMP_SRCS += $(wildcard ../svp/common/*.c)
# SMP_SRCS += $(wildcard ../svp/npu/sample_npu_software/*.c)
# SMP_CXXSRCS += $(wildcard ../svp/npu/sample_npu_software/*.cpp)
# MPI_LIBS += ../svp/npu/libcjson.a

# SMP_INC :=
# #lib ive
# # SMP_INC := -I../ive/sample
# # SMP_SRCS += $(wildcard ../ive/sample/*.c)

TARGET := test_uart

include ../$(ARM_ARCH)_$(OSTYPE).mak
