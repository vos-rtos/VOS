/*#############################################################################
 * �ļ�����file.c
 * ���ܣ�  ʵ����ָ������ļ��Ĳ���
 * modified by  PRTsinghua@hotmail.com
#############################################################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

#include "vos.h"

#include "ffconf.h"
#include "ff.h"

/* �������Щ�ӿ�ʵ����˽�еģ�����Ϊ�û���֪ */
typedef struct iFvsFile_t
{
#if 0
	//FILE	*pf;	/* �ļ�ָ�� */
#else
	FIL	*pf;	/* �ļ�ָ�� */
#endif
} iFvsFile_t;


/******************************************************************************
  * ���ܣ�����һ���µ��ļ�����ֻ���ڴ���֮���ļ��������Ϊ�����������á�
  * ��������
  * ���أ�������ʧ�ܣ�����NULL�����򷵻��µĶ�������
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
  * ���ܣ��ƻ�һ���Ѿ����ڵ��ļ������ڻٻ�֮���ļ���������Ϊ�����������á�
  * ������file  ����ɾ�����ļ�����ָ��
  * ���أ��޷���ֵ
******************************************************************************/
void FileDestroy(FvsFile_t file)
{
	iFvsFile_t* p = NULL;
	if (file==NULL)
		return;
	/* �ر��ļ�������������� */
	(void)FileClose(file);
	p = file;
	vfree(p);
}


/******************************************************************************
  * ���ܣ���һ���µ��ļ���һ���ļ����Զ��򿪣�д�򿪣����߱�������
  * ������file    �ļ�����
  *       name    �����ļ�������
  *       flags   �򿪱�־
  * ���أ�������
******************************************************************************/
FvsError_t FileOpen(FvsFile_t file, const FvsString_t name, 
						const FvsFileOptions_t flags)
{
	iFvsFile_t* p = (iFvsFile_t*)file;

	int nflags = (int)flags;
	/* �ر��ļ�������Ѿ��� */
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
  * ���ܣ��ر�һ���ļ������ļ��ر�֮���ļ����ٿ��á�
  * ������file    �ļ�����
  * ���أ�������
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
  * ���ܣ�����һ���ļ��Ƿ��
  * ������file    �ļ�����
  * ���أ��ļ��򿪣��򷵻�true�����򷵻�false
******************************************************************************/
FvsBool_t FileIsOpen(const FvsFile_t file)
{
	iFvsFile_t* p = (iFvsFile_t*)file;
	return (p->pf!=NULL)?FvsTrue:FvsFalse;
}


/******************************************************************************
  * ���ܣ������Ƿ����ļ���β
  * ������file    �ļ�����
  * ���أ����˽�β������true�����򷵻�false
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
  * ���ܣ��ύ���ļ������ĸ���
  * ������file    �ļ�����
  * ���أ�������
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
  * ���ܣ������ļ��Ŀ�ͷ
  * ������file    �ļ�����
  * ���أ�������
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
  * ���ܣ������ļ��Ľ�β
  * ������file    �ļ�����
  * ���أ�������
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
  * ���ܣ��õ���ǰ���ļ�ָ��λ��
  * ������file    �ļ�����
  * ���أ���ǰ��ָ��λ��
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
  * ���ܣ������ļ���ָ��λ��
  * ������file     �ļ�����
  *       position ָ�����ļ�λ��
  * ���أ�������
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
  * ���ܣ����ļ��ж����ݣ�����ȡ���ֽ�����length��������ȡ�����ݱ�����ָ��data��
  * ������file    �ļ�����
  *       data    ָ��洢���ݵ�����
  *       length  Ҫ��ȡ���ֽ���
  * ���أ�ʵ�ʶ�ȡ���ֽ���
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
  * ���ܣ����ļ���д���ݣ���д���ֽ�����length������Ҫд������ݱ�����ָ��data��
  * ������file    �ļ�����
  *       data    ָ��洢���ݵ�����
  *       length  Ҫд����ֽ���
  * ���أ�ʵ��д����ֽ���
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
  * ���ܣ����ļ��еõ�һ���ֽ�
  * ������file    �ļ�����
  * ���أ���ȡ���ֽ�
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
  * ���ܣ����ļ��ж�ȡһ����
  * ������file    �ļ�����
  * ���أ���ȡ����
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

