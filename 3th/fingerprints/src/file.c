/*#############################################################################
 * 文件名：file.c
 * 功能：  实现了指纹相关文件的操作
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

#include "vos.h"

#include "ffconf.h"
#include "ff.h"

/* 对象的这些接口实现是私有的，不必为用户所知 */
typedef struct iFvsFile_t
{
#if 0
	//FILE	*pf;	/* 文件指针 */
#else
	FIL	*pf;	/* 文件指针 */
#endif
} iFvsFile_t;


/******************************************************************************
  * 功能：创建一个新的文件对象，只有在创建之后，文件对象才能为其它函数所用。
  * 参数：无
  * 返回：若创建失败，返回NULL；否则返回新的对象句柄。
******************************************************************************/
FvsFile_t FileCreate()
{
	iFvsFile_t* p = NULL;
	p = (iFvsFile_t*)vmalloc(sizeof(iFvsFile_t));
	if (p!=NULL)
		p->pf = NULL;
	return (FvsFile_t)p;
}


/******************************************************************************
  * 功能：破坏一个已经存在的文件对象，在毁坏之后，文件对象不能再为其它函数所用。
  * 参数：file  即将删除的文件对象指针
  * 返回：无返回值
******************************************************************************/
void FileDestroy(FvsFile_t file)
{
	iFvsFile_t* p = NULL;
	if (file==NULL)
		return;
	/* 关闭文件，如果它还打开着 */
	(void)FileClose(file);
	p = file;
	vfree(p);
}


/******************************************************************************
  * 功能：打开一个新的文件。一个文件可以读打开，写打开，或者被创建。
  * 参数：file    文件对象
  *       name    待打开文件的名字
  *       flags   打开标志
  * 返回：错误编号
******************************************************************************/
FvsError_t FileOpen(FvsFile_t file, const FvsString_t name, 
						const FvsFileOptions_t flags)
{
	iFvsFile_t* p = (iFvsFile_t*)file;

	int nflags = (int)flags;
	/* 关闭文件，如果已经打开 */
	(void)FileClose(p);

#if 0
	char strFlags[10];
	strcpy(strFlags, "");
	if ( (nflags & FvsFileRead)!=0   &&
		 (nflags & FvsFileWrite)!=0 )
		strcat(strFlags, "rw");
	else
	{
		if ((nflags & FvsFileRead)!=0)
			strcat(strFlags, "r");
		if ((nflags & FvsFileWrite)!=0)
			strcat(strFlags, "w");
	}    
	strcat(strFlags, "b");
	if ((nflags & FvsFileCreate)!=0)
		strcat(strFlags, "+");

	p->pf = fopen(name, strFlags);

	if (FileIsOpen(file)==FvsTrue)
		return FvsOK;
#else
	FRESULT ret;
	int file_flags = 0;
	if ( (nflags & FvsFileRead)!=0 && (nflags & FvsFileWrite)!=0 ) {
		file_flags = FA_READ|FA_WRITE;
	}
	else {
		if ((nflags & FvsFileRead)!=0) {
			file_flags = FA_READ;
		}
		if ((nflags & FvsFileWrite)!=0) {
			file_flags = FA_WRITE;
		}
	}
	if ((nflags & FvsFileCreate)!=0) {
		file_flags |= FA_OPEN_APPEND | FA_CREATE_NEW;
	}
	else {
		file_flags |= FA_OPEN_ALWAYS;
	}
	p->pf = vmalloc(sizeof(FIL));
	ret = f_open(p->pf, name, file_flags);
	if (ret == FR_OK) {
		kprintf("file open ok!\r\n");
	}
	else {
		vfree(p->pf);
		p->pf = 0;
		kprintf("file open failed!\r\n");
	}
#endif
	if (FileIsOpen(file)==FvsTrue)
		return FvsOK;
	return FvsFailure;
}


/******************************************************************************
  * 功能：关闭一个文件对象，文件关闭之后，文件不再可用。
  * 参数：file    文件对象
  * 返回：错误编号
******************************************************************************/
FvsError_t FileClose(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	int nerr = -1;
	if (p->pf!=NULL)
	{
#if 0
		nerr = fclose(p->pf);
#else
		nerr = f_close(p->pf);
		if (p->pf) {
			vfree(p->pf);
		}
#endif
		p->pf = NULL;
	}
	if (nerr==0)
		return FvsOK;
	return FvsFailure;
}


/******************************************************************************
  * 功能：测试一个文件是否打开
  * 参数：file    文件对象
  * 返回：文件打开，则返回true；否则返回false
******************************************************************************/
FvsBool_t FileIsOpen(const FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	return (p->pf!=NULL)?FvsTrue:FvsFalse;
}


/******************************************************************************
  * 功能：测试是否到了文件结尾
  * 参数：file    文件对象
  * 返回：到了结尾，返回true；否则返回false
******************************************************************************/
FvsBool_t FileIsAtEOF(const FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	if (FileIsOpen(p)==FvsFalse)
		return FvsFalse;
#if 0
	return (feof(p->pf)!=0)?FvsTrue:FvsFalse;
#else
	return (f_eof(p->pf)!=0)?FvsTrue:FvsFalse;
#endif
}


/******************************************************************************
  * 功能：提交对文件所作的更改
  * 参数：file    文件对象
  * 返回：错误编号
******************************************************************************/
FvsError_t FileCommit(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
#if 0
	return (fflush(p->pf)==0)?FvsOK:FvsFailure;
#else
	return (f_sync(p->pf)==0)?FvsOK:FvsFailure;
#endif
}


/******************************************************************************
  * 功能：跳到文件的开头
  * 参数：file    文件对象
  * 返回：错误编号
******************************************************************************/
FvsError_t FileSeekToBegin(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	if (FileIsOpen(p)==FvsTrue)
	{
#if 0
		if (fseek(p->pf, 0, SEEK_SET)!=0)
#else
		if (f_lseek(p->pf, 0)!=0)
#endif
			return FvsFailure;
		return FvsOK;
	}
	return FvsFailure;
}


/******************************************************************************
  * 功能：跳到文件的结尾
  * 参数：file    文件对象
  * 返回：错误编号
******************************************************************************/
FvsError_t FileSeekToEnd(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	if (FileIsOpen(p)==FvsTrue)
	{
#if 0
		if (fseek(p->pf, 0, SEEK_END)!=0)
#else
		if (f_lseek(p->pf, f_size(p->pf))!=0)
#endif
			return FvsFailure;
		return FvsOK;
	}
	return FvsFailure;
}


/******************************************************************************
  * 功能：得到当前的文件指针位置
  * 参数：file    文件对象
  * 返回：当前的指针位置
******************************************************************************/
FvsUint_t FileGetPosition(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	if (FileIsOpen(p)==FvsTrue)
#if 0
		return (FvsUint_t)ftell(p->pf);
#else
		return (FvsUint_t)f_tell(p->pf);
#endif
	return 0;
}


/******************************************************************************
  * 功能：跳到文件的指定位置
  * 参数：file     文件对象
  *       position 指定的文件位置
  * 返回：错误编号
******************************************************************************/
FvsError_t FileSeek(FvsFile_t file, const FvsUint_t position)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	if (FileIsOpen(p)==FvsTrue)
	{
#if 0
		if (fseek(p->pf, (long int)position, SEEK_SET)!=0)
#else
		if (f_lseek(p->pf, (long int)position)!=0)
#endif
			return FvsFailure;
		return FvsOK;
	}
	return FvsFailure;
}


/******************************************************************************
  * 功能：从文件中读数据，所读取的字节数由length决定。读取的数据保存于指针data。
  * 参数：file    文件对象
  *       data    指向存储数据的数组
  *       length  要读取的字节数
  * 返回：实际读取的字节数
******************************************************************************/
FvsUint_t FileRead(FvsFile_t file, FvsPointer_t data, const FvsUint_t length)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
#if 0
	return (FvsUint_t)fread(data, (size_t)1, (size_t)length, p->pf);
#else
	u32 num = 0;
	if (FR_OK == f_read (p->pf, data, length, &num)) {
		return num;
	}
	else
		return 0;
#endif
}


/******************************************************************************
  * 功能：往文件中写数据，所写的字节数由length决定。要写入的数据保存于指针data。
  * 参数：file    文件对象
  *       data    指向存储数据的数组
  *       length  要写入的字节数
  * 返回：实际写入的字节数
******************************************************************************/
FvsUint_t FileWrite(FvsFile_t file, const FvsPointer_t data, const FvsUint_t length)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
#if 0
	return (FvsUint_t)fwrite(data, (size_t)1, (size_t)length, p->pf);
#else
	u32 num = 0;
	if (FR_OK == (FvsUint_t)f_write(p->pf, data, (size_t)length, &num)) {
		return num;
	}
	return 0;
#endif
}


/******************************************************************************
  * 功能：从文件中得到一个字节
  * 参数：file    文件对象
  * 返回：读取的字节
******************************************************************************/
FvsByte_t FileGetByte(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
#if 0
	return (FvsByte_t)fgetc(p->pf);
#else
	char c;
	if (FR_OK == f_putc (&c, p->pf)) {
		return c;
	}
	return 0;
#endif
}


/******************************************************************************
  * 功能：从文件中读取一个字
  * 参数：file    文件对象
  * 返回：读取的字
******************************************************************************/
FvsWord_t FileGetWord(FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
#if 0
	FvsWord_t w = (FvsWord_t)fgetc(p->pf);
	return (w<<8)+fgetc(p->pf);
#else
	FvsWord_t w;
	if (FR_OK == f_putc ((char*)&w, p->pf)) {
		w = w<<8;
		if (FR_OK == f_putc ((char*)&w, p->pf)) {
			return w;
		}
	}
	return 0;
#endif
}

