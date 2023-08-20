ifeq ($(PARAM_FILE), )
     PARAM_FILE := ../../mpp/Makefile.param
     include $(PARAM_FILE)
endif


sensor_lib_sub_dirs := sony_imx307
sensor_lib_sub_dirs += sony_imx415
sensor_lib_sub_dirs += smartsens_sc2310
sensor_lib_sub_dirs += smartsens_sc2210
sensor_lib_sub_dirs += galaxycore_gc2053
sensor_lib_sub_dirs += galaxycore_gc2093
sensor_lib_sub_dirs += galaxycore_gc1603
sensor_lib_sub_dirs += isp_in_0


sub_build := $(patsubst %,%_build,$(sensor_lib_sub_dirs))

sub_clean := $(patsubst %,%_clean,$(sensor_lib_sub_dirs))


all: $(sub_build)

clean: $(sub_clean)

%_build: prepare
	@cd $(patsubst %_build,%,$@); $(MAKE);

%_clean: pre_clean
	@cd $(patsubst %_clean,%,$@); $(MAKE) clean;

prepare:
	@mkdir -p libs

pre_clean:
	@rm -rf libs
