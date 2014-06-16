LOCAL_PATH := $(call my-dir)/..

include $(CLEAR_VARS)

LOCAL_MODULE := mikmod

MY_MIKMOD_PATH := $(LOCAL_PATH)/..
LOCAL_C_INCLUDES := $(LOCAL_PATH) $(MY_MIKMOD_PATH)/include
LOCAL_LDLIBS += -lOpenSLES
LOCAL_CFLAGS := -DHAVE_CONFIG_H

LOCAL_SRC_FILES := \
$(MY_MIKMOD_PATH)/drivers/drv_nos.c \
$(MY_MIKMOD_PATH)/drivers/drv_osles.c \
$(MY_MIKMOD_PATH)/loaders/load_669.c \
$(MY_MIKMOD_PATH)/loaders/load_amf.c \
$(MY_MIKMOD_PATH)/loaders/load_asy.c \
$(MY_MIKMOD_PATH)/loaders/load_dsm.c \
$(MY_MIKMOD_PATH)/loaders/load_far.c \
$(MY_MIKMOD_PATH)/loaders/load_gdm.c \
$(MY_MIKMOD_PATH)/loaders/load_gt2.c \
$(MY_MIKMOD_PATH)/loaders/load_imf.c \
$(MY_MIKMOD_PATH)/loaders/load_it.c \
$(MY_MIKMOD_PATH)/loaders/load_m15.c \
$(MY_MIKMOD_PATH)/loaders/load_med.c \
$(MY_MIKMOD_PATH)/loaders/load_mod.c \
$(MY_MIKMOD_PATH)/loaders/load_mtm.c \
$(MY_MIKMOD_PATH)/loaders/load_okt.c \
$(MY_MIKMOD_PATH)/loaders/load_s3m.c \
$(MY_MIKMOD_PATH)/loaders/load_stm.c \
$(MY_MIKMOD_PATH)/loaders/load_stx.c \
$(MY_MIKMOD_PATH)/loaders/load_ult.c \
$(MY_MIKMOD_PATH)/loaders/load_umx.c \
$(MY_MIKMOD_PATH)/loaders/load_uni.c \
$(MY_MIKMOD_PATH)/loaders/load_xm.c \
$(MY_MIKMOD_PATH)/mmio/mmalloc.c \
$(MY_MIKMOD_PATH)/mmio/mmerror.c \
$(MY_MIKMOD_PATH)/mmio/mmio.c \
$(MY_MIKMOD_PATH)/playercode/mdreg.c \
$(MY_MIKMOD_PATH)/playercode/mdriver.c \
$(MY_MIKMOD_PATH)/playercode/mdulaw.c \
$(MY_MIKMOD_PATH)/playercode/mloader.c \
$(MY_MIKMOD_PATH)/playercode/mlreg.c \
$(MY_MIKMOD_PATH)/playercode/mlutil.c \
$(MY_MIKMOD_PATH)/playercode/mplayer.c \
$(MY_MIKMOD_PATH)/playercode/munitrk.c \
$(MY_MIKMOD_PATH)/playercode/mwav.c \
$(MY_MIKMOD_PATH)/playercode/npertab.c \
$(MY_MIKMOD_PATH)/playercode/sloader.c \
$(MY_MIKMOD_PATH)/playercode/virtch.c \
$(MY_MIKMOD_PATH)/playercode/virtch_common.c \
$(MY_MIKMOD_PATH)/playercode/virtch2.c \
$(MY_MIKMOD_PATH)/posix/memcmp.c \
$(MY_MIKMOD_PATH)/posix/strcasecmp.c \
$(MY_MIKMOD_PATH)/posix/strdup.c \
$(MY_MIKMOD_PATH)/posix/strstr.c

include $(BUILD_SHARED_LIBRARY)
