LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := rkcrc.c

LOCAL_MODULE := rkcrc
LOCAL_MODULE_TAGS := eng

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

include $(CLEAR_VARS)

LOCAL_SRC_FILES := rkafpack.c

LOCAL_MODULE := rkafpack
LOCAL_MODULE_TAGS := eng

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

include $(CLEAR_VARS)

LOCAL_SRC_FILES := rkunpack.c

LOCAL_MODULE := rkunpack
LOCAL_MODULE_TAGS := eng

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

include $(CLEAR_VARS)

LOCAL_SRC_FILES := rkdecrypt.c

LOCAL_MODULE := rkdecrypt
LOCAL_MODULE_TAGS := eng

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

include $(CLEAR_VARS)

LOCAL_SRC_FILES := rkflash.c

LOCAL_MODULE := rkflash
LOCAL_MODULE_TAGS := eng

LOCAL_CFLAGS += -DHAS_LIBUSB
LOCAL_LDLIBS += -lusb

include $(BUILD_HOST_EXECUTABLE)

$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := rkdump.c

LOCAL_MODULE := rkdump
LOCAL_MODULE_TAGS := eng

include $(BUILD_EXECUTABLE)
