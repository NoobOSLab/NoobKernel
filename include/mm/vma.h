#ifndef __MM_VMA_H__
#define __MM_VMA_H__

#include <misc/stddef.h>
#include <misc/list.h>

struct vma{
	u64 start;
	u64 end;
	struct list_head list;
};

#endif