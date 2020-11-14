#include "vmisc.h"

s8 *GB2312_TO_UTF8_LOCAL(s8 *gb2312)
{
	static s8 *OutPtr = 0;
	s32 needlen = 0;
	s32 len = strlen(gb2312);
	if (gb2312==0) return 0;
	if (OutPtr) {
		vfree(OutPtr);
	}
	if (OutPtr = (s8*)vmalloc(len*2+1)) {
		memset(OutPtr, 0, len*2+1);
		if (Gb2312ToUtf8(gb2312, strlen(gb2312), OutPtr, len*2, &needlen) <= 0) {
			vfree(OutPtr);
			OutPtr = 0;
		}
	}
	return OutPtr;
}

int dumphex(const unsigned char *buf, int size)
{
	int i;
	for(i=0; i<size; i++)
		kprintf("%02x,%s", buf[i], (i+1)%16?"":"\r\n");
	if (i%16) kprintf("\r\n");
	return 0;
}

