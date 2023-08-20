# 文件名 : Makefile
# 日期 : v1.0.0
# 作者 : 郭辰磊
# 单位 : 中国农业大学-国家数字渔业创新中心-数字养殖网箱项目
# 描述 : 网箱智能控制系统主程序
# 其他 : 无
# 日志 : 初版v1.0.0 2023/3/10 郭辰磊创建
include ../Makefile.param

#=============main======================================
#程序入口
CFLAGS += -I./src
CXXFLAGS += -Wno-psabi
SMP_SRCS += $(wildcard ./src/*.c)
SMP_CXXSRCS += $(wildcard ./src/*.cpp)
SMP_SRCS := $(wildcard $(ARM_ARCH)/*.c)
MPI_LDFLAGS += -pthread
MPI_LDFLAGS += -lm

#=============mpp库======================================
#调用智兴公司提供的mpp库
#lib vio common
CFLAGS += -I../vio/smp
SMP_SRCS += $(wildcard ../vio/smp/*.c)

ifeq ($(MPI_SO_FLAG), y)
  ## use libxx.so
  MPI_LDFLAGS += -lnpu
  MPI_LDFLAGS += -lrcu
else
  ## use libxx.a
  MPI_LIBS += $(REL_LIB)/libnpu.a
  MPI_LIBS += $(REL_LIB)/librcu.a
endif

# lib SVP
CFLAGS += -I../svp/common
CFLAGS += -I../svp/npu/sample_npu_software
SMP_SRCS += $(wildcard ../svp/common/*.c)
SMP_SRCS += $(wildcard ../svp/npu/sample_npu_software/*.c)
SMP_CXXSRCS += $(wildcard ../svp/npu/sample_npu_software/*.cpp)
MPI_LIBS += ../svp/npu/libcjson.a

SMP_INC :=
#lib ive
# SMP_INC := -I../ive/sample
# SMP_SRCS += $(wildcard ../ive/sample/*.c)


#=============开源库======================================
#连接第三方开源库
# lib MQTT 
CFLAGS += -I./open_lib/mqtt/include
MPI_LDFLAGS += -L./open_lib/mqtt/lib
MPI_LDFLAGS += -lpaho-mqtt3a
MPI_LDFLAGS += -lpaho-mqtt3as
MPI_LDFLAGS += -lpaho-mqtt3c
MPI_LDFLAGS += -lpaho-mqtt3cs

#lib opensll
CFLAGS += -I./open_lib/openssl/include
MPI_LDFLAGS += -L./open_lib/openssl/lib
MPI_LDFLAGS += -lcrypto
MPI_LDFLAGS += -lssl

#lib jsoncpp
CFLAGS += -I./open_lib/jsoncpp
SMP_CXXSRCS += $(wildcard ./open_lib/jsoncpp/*.cpp)

#lib opencv
CFLAGS += -I./open_lib/opencv/include
MPI_LDFLAGS += -L./open_lib/opencv/lib
MPI_LDFLAGS += -lopencv_calib3d
MPI_LDFLAGS += -lopencv_core
MPI_LDFLAGS += -lopencv_dnn
MPI_LDFLAGS += -lopencv_features2d
MPI_LDFLAGS += -lopencv_flann
MPI_LDFLAGS += -lopencv_highgui
MPI_LDFLAGS += -lopencv_imgcodecs
MPI_LDFLAGS += -lopencv_imgproc
MPI_LDFLAGS += -lopencv_ml
MPI_LDFLAGS += -lopencv_objdetect
MPI_LDFLAGS += -lopencv_photo
MPI_LDFLAGS += -lopencv_shape
MPI_LDFLAGS += -lopencv_stitching
MPI_LDFLAGS += -lopencv_superres
MPI_LDFLAGS += -lopencv_video
MPI_LDFLAGS += -lopencv_videoio
MPI_LDFLAGS += -lopencv_videostab

#lib ffmpeg
CFLAGS += -I./open_lib/ffmpeg/include
MPI_LDFLAGS += -L./open_lib/ffmpeg/lib
MPI_LDFLAGS += -lavcodec
MPI_LDFLAGS += -lavdevice
MPI_LDFLAGS += -lavfilter
MPI_LDFLAGS += -lavformat
MPI_LDFLAGS += -lavutil
MPI_LDFLAGS += -lpostproc
MPI_LDFLAGS += -lswresample
MPI_LDFLAGS += -lswscale
#=============工具库======================================
#编写的统一工具
#统一io系统
CFLAGS += -I./units/io
SMP_CXXSRCS += $(wildcard ./units/io/*.cpp)

#统一串口控制
CFLAGS += -I./units/uart
SMP_CXXSRCS += $(wildcard ./units/uart/*.cpp)

#统一网络处理
CFLAGS += -I./units/netcore
SMP_CXXSRCS += $(wildcard ./units/netcore/*.cpp)
#=============算法库======================================
#编写的算法
#鱼群计数算法
CFLAGS += -I./algorithm/count/postProcess/common/include
SMP_CXXSRCS += $(wildcard ./algorithm/count/postProcess/common/src/*.cpp)
CFLAGS += -I./algorithm/count/src/include
SMP_CXXSRCS += $(wildcard ./algorithm/count/src/*.cpp)


TARGET := test_netcage_control_system

include ../$(ARM_ARCH)_$(OSTYPE).mak
