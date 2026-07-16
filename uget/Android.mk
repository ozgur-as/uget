LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := libuget
LOCAL_CPPFLAGS   += -DNDEBUG
##                  -DNO_RETRY_IF_CONNECT_FAILED

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/../uglib $(LOCAL_PATH)/../curl/include $(LOCAL_PATH)/../openssl/include
LOCAL_SRC_FILES  := \
	UgetSequence.c  \
	UgetRpc.c     \
	UgetOption.c  \
	UgetData.c    \
	UgetFiles.c   \
	UgetNode.c    \
	UgetNode-compare.c  \
	UgetNode-filter.c   \
	UgetTask.c    \
	UgetHash.c    \
	UgetApp.c     \
	UgetEvent.c   \
	UgetPlugin.c  \
	UgetA2cf.c    \
	UgetCurl.c    \
	UgetAria2.c   \
	UgetResolver.c \
	UgetPluginCurl.c   \
	UgetPluginAria2.c  \
	UgetPluginAgent.c  \
	UgetPluginMega.c   \
	UgetPluginYtdlp.c

include $(BUILD_STATIC_LIBRARY)

