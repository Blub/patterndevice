#ifndef PATTERNDEVICE_H__
#define PATTERNDEVICE_H__

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

enum {
	FIOC_GET_KEEP_OFFSET = _IOR('E', 1, char),
	FIOC_SET_KEEP_OFFSET = _IOW('E', 1, char)
};

#endif
