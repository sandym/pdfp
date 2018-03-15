
#ifndef H_PDFP_APDFL_CONFIG
#define H_PDFP_APDFL_CONFIG

#include "su/base/platform.h"

#if UPLATFORM_MAC
#	define PLATFORM "MacPlatform.h"
#	define MAC_ENV 1
#else
#error
#endif

#define TOOLKIT 1
#define PRODUCT "HFTLibrary.h"

#endif
