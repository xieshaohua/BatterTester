# Copyright 2016 Phicomm Power Management Team

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := tester.c wakealarm.c monitor.c pon_charging.c discharge.c tester.h

LOCAL_MODULE := battst

LOCAL_CFLAGS := -O2 -Wall -Wno-unused-parameter

LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
#LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)

LOCAL_STATIC_LIBRARIES := libutils libstdc++ libcutils liblog libm libc

include $(BUILD_EXECUTABLE)
