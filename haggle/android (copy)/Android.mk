LOCAL_PATH:= $(call my-dir)

subdirs := $(addprefix $(LOCAL_PATH)/../../,$(addsuffix /Android.mk, \
		Haggle \
		PhotoShare \
		LuckyMe \
        ))

include $(call first-makefiles-under, $(subdirs))
