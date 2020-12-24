/*
* true type 框架需要用到下面函数，但是我找不到哪里调用，先编写上来，然后assert住，再修改。
*/

#include "vos.h"
#include "stdint.h"
#include <sys/stat.h>
int z_verbose = 0;

void z_error(char *m)
{
	while(1) ;
	return 0;
}
int _fstat(int filedes, struct stat *buf)
{
//	  buf->st_mode = S_IFCHR;
//	  return 0;
	while(1) ;
	return 0;
}
int _close(int fd)
{
	while(1) ;
	return 0;
}
short _isatty(int fd)
{
//	return fd == 0 || fd == 1 || fd == 2;
	while(1) ;
	return 0;
}
s32 _lseek(int handle, s32 offset, int fromwhere)
{
	while(1) ;
	return 0;
}

u32 _read(int fd, void *buf, u32 count)
{
	while(1) ;
	return 0;
}

int _write(int fd, const void *buffer, unsigned int count)
{
	while(1) ;
	return 0;
}

void *_sbrk(intptr_t increment)
{
	while(1) ;
	return 0;
}
