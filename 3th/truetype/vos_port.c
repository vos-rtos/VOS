/*
* true type �����Ҫ�õ����溯�����������Ҳ���������ã��ȱ�д������Ȼ��assertס�����޸ġ�
*/

#include "vos.h"
#include "stdint.h"
int z_verbose = 0;

void z_error(char *m)
{

}
int _fstat(int filedes, struct stat *buf)
{
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
