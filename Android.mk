# Copyright 2016 Phicomm Power Management Team

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

lua_src_files := $(call all-c-files-under, lua51/)
LOCAL_SRC_FILES := $(lua_src_files) tester.c wakealarm.c monitor.c pon_charging.c discharge.c utils.c tester.h

LOCAL_MODULE := battst

LOCAL_CFLAGS := -O2 -Wall -Wno-unused-parameter -Wno-missing-braces

LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
#LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)

LOCAL_STATIC_LIBRARIES := libutils libstdc++ libcutils liblog libm libc

include $(BUILD_EXECUTABLE)
