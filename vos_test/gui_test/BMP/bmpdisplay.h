#ifndef _BMPDISPLAY_H
#define _BMPDISPLAY_H
#include "vos.h"


//ʹ��GUI_BMP_Draw()��������BMPͼƬ�Ļ�
//ͼƬ�Ǽ��ص�RAM�еģ���˲��ܴ���BMPMEMORYSIZE
//ע�⣺��ʾBMPͼƬʱ�ڴ�����ʹ�õ�EMWIN���ڴ����뺯�������
//BMPMEMORYSIZE���ܴ������Ǹ�EMWIN������ڴ�ش�С
#define BMPMEMORYSIZE	500*1024	//ͼƬ��С������500kb

//����������ص�RAM�е�BMPͼƬʱ��ͼƬÿ�е��ֽ���
#define BMPPERLINESIZE	2*1024		

int dispbmp(u8 *BMPFileName,u8 mode,u32 x,u32 y,int member,int denom);
int dispbmpex(u8 *BMPFileName,u8 mode,u32 x,u32 y,int member,int denom);
void create_bmppicture(u8 *filename,int x0,int y0,int Xsize,int Ysize);
void bmpdisplay_demo(void);
#endif
