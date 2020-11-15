#include "stm32f4xx.h"
//#include "EmWinHZFont.h"
#include "GUI.h"
#include "vheap.h"
#include "ff.h"
#include "lcd_dev/lcd.h"
#include "jpegdisplay.h"
#include "vmisc.h"

#if SYSTEM_SUPPORT_OS
#include "includes.h"					//ucos ʹ��	  
#endif

static FIL JPEGFile;
static char jpegBuffer[JPEGPERLINESIZE];
/*******************************************************************
*
*       Static functions
*
********************************************************************
*/
/*********************************************************************
*
*       _GetData
*
* Function description
*   This routine is called by GUI_JPEG_DrawEx(). The routine is responsible
*   for setting the data pointer to a valid data location with at least
*   one valid byte.
*
* Parameters:
*   p           - Pointer to application defined data.
*   NumBytesReq - Number of bytes requested.
*   ppData      - Pointer to data pointer. This pointer should be set to
*                 a valid location.
*   StartOfFile - If this flag is 1, the data pointer should be set to the
*                 beginning of the data stream.
*
* Return value:
*   Number of data bytes available.
*/
static int JpegGetData(void * p, const U8 ** ppData, unsigned NumBytesReq, U32 Off) 
{
	static int readaddress=0;
	FIL * phFile;
	UINT NumBytesRead;
	#if SYSTEM_SUPPORT_OS
		CPU_SR_ALLOC();
	#endif

	phFile = (FIL *)p;
	
	if (NumBytesReq > sizeof(jpegBuffer)) 
	{
		NumBytesReq = sizeof(jpegBuffer);
	}

	//�ƶ�ָ�뵽Ӧ�ö�ȡ��λ��
	if(Off == 1) readaddress = 0;
	else readaddress=Off;
	
	#if SYSTEM_SUPPORT_OS
		OS_CRITICAL_ENTER();	//�����ٽ���
	#endif
		
	f_lseek(phFile,readaddress); 
	
	//��ȡ���ݵ���������
	f_read(phFile,jpegBuffer,NumBytesReq,&NumBytesRead);
	
	#if SYSTEM_SUPPORT_OS
		OS_CRITICAL_EXIT();//�˳��ٽ���
	#endif
	
	*ppData = (U8 *)jpegBuffer;
	return NumBytesRead;//���ض�ȡ�����ֽ���
}

//��ָ��λ����ʾ���ص�RAM�е�JPEGͼƬ
//JPEGFileName:ͼƬ��SD�����������洢�豸�е�·��(���ļ�ϵͳ֧�֣�)
//mode:��ʾģʽ
//		0 ��ָ��λ����ʾ���в���x,yȷ����ʾλ��
//		1 ��LCD�м���ʾͼƬ����ѡ���ģʽ��ʱ�����x,y��Ч��
//x:ͼƬ���Ͻ���LCD�е�x��λ��(������modeΪ1ʱ���˲�����Ч)
//y:ͼƬ���Ͻ���LCD�е�y��λ��(������modeΪ1ʱ���˲�����Ч)
//member:  ���ű����ķ�����
//denom:���ű����ķ�ĸ��
//����ֵ:0 ��ʾ����,���� ʧ��
int displyjpeg(u8 *JPEGFileName,u8 mode,u32 x,u32 y,int member,int denom)
{
	u16 bread;
	char *jpegbuffer;
	char result;
	int XSize,YSize;
	GUI_JPEG_INFO JpegInfo;
	float Xflag,Yflag;
	u32 fsize;
	
	#if SYSTEM_SUPPORT_OS
		CPU_SR_ALLOC();
	#endif

	result = f_open(&JPEGFile,(const TCHAR*)JPEGFileName,FA_READ);	//���ļ�

	//�ļ��򿪴�������ļ�����JPEGMEMORYSIZE
	if((result != FR_OK) || (f_size(&JPEGFile)>JPEGMEMORYSIZE)) 	return 1;
	
	jpegbuffer=vmalloc(f_size(&JPEGFile));	//�����ڴ�
	if(jpegbuffer == NULL) return 2;
	
	#if SYSTEM_SUPPORT_OS
		OS_CRITICAL_ENTER();		//�ٽ���
	#endif
		
	result = f_read(&JPEGFile,jpegbuffer,f_size(&JPEGFile),(UINT *)&bread); //��ȡ����
	if(result != FR_OK) return 3;
	
	#if SYSTEM_SUPPORT_OS
		OS_CRITICAL_EXIT();	//�˳��ٽ���
	#endif
	
	GUI_JPEG_GetInfo(jpegbuffer,f_size(&JPEGFile),&JpegInfo); //��ȡJEGPͼƬ��Ϣ
	XSize = JpegInfo.XSize;	//��ȡJPEGͼƬ��X���С
	YSize = JpegInfo.YSize;	//��ȡJPEGͼƬ��Y���С

	if (XSize > LCD_GetXSize() || YSize > LCD_GetYSize()) {
		f_close(&JPEGFile);		//�ر�BMPFile�ļ�
		vfree(jpegbuffer);	//�ͷ��ڴ�
		return 0;
	}
	switch(mode)
	{
		case 0:	//��ָ��λ����ʾͼƬ
			if((member == 1) && (denom == 1)) //�������ţ�ֱ�ӻ���
			{
				GUI_JPEG_Draw(jpegbuffer,f_size(&JPEGFile),x,y);	//��ָ��λ����ʾJPEGͼƬ
			}else //����ͼƬ��Ҫ����
			{
				GUI_JPEG_DrawScaled(jpegbuffer,f_size(&JPEGFile),x,y,member,denom);
			}
			break;
		case 1:	//��LCD�м���ʾͼƬ
			if((member == 1) && (denom == 1)) //�������ţ�ֱ�ӻ���
			{
				//��LCD�м���ʾͼƬ
				GUI_JPEG_Draw(jpegbuffer,f_size(&JPEGFile),(lcddev.width-XSize)/2-1,(lcddev.height-YSize)/2-1);
			}else //����ͼƬ��Ҫ����
			{
				Xflag = (float)XSize*((float)member/(float)denom);
				Yflag = (float)YSize*((float)member/(float)denom);
				XSize = (lcddev.width-(int)Xflag)/2-1;
				YSize = (lcddev.height-(int)Yflag)/2-1;
				GUI_JPEG_DrawScaled(jpegbuffer,f_size(&JPEGFile),XSize,YSize,member,denom);
			}
			break;
	}
	f_close(&JPEGFile);			//�ر�JPEGFile�ļ�
	vfree(jpegbuffer);	//�ͷ��ڴ�
	return 0;
}

//��ָ��λ����ʾ������ص�RAM�е�BMPͼƬ(���ļ�ϵͳ֧�֣�����СRAM���Ƽ�ʹ�ô˷�����)
//JPEGFileName:ͼƬ��SD�����������洢�豸�е�·��
//mode:��ʾģʽ
//		0 ��ָ��λ����ʾ���в���x,yȷ����ʾλ��
//		1 ��LCD�м���ʾͼƬ����ѡ���ģʽ��ʱ�����x,y��Ч��
//x:ͼƬ���Ͻ���LCD�е�x��λ��(������modeΪ1ʱ���˲�����Ч)
//y:ͼƬ���Ͻ���LCD�е�y��λ��(������modeΪ1ʱ���˲�����Ч)
//member:  ���ű����ķ�����
//denom:���ű����ķ�ĸ��
//����ֵ:0 ��ʾ����,���� ʧ��
int displayjpegex(u8 *JPEGFileName,u8 mode,u32 x,u32 y,int member,int denom)
{
	char result;
	int XSize,YSize;
	float Xflag,Yflag;
	GUI_JPEG_INFO JpegInfo;
	
	result = f_open(&JPEGFile,(const TCHAR*)JPEGFileName,FA_READ);	//���ļ�
	//�ļ��򿪴���
	if(result != FR_OK) 	return 1;
	
	GUI_JPEG_GetInfoEx(JpegGetData,&JPEGFile,&JpegInfo);
	XSize = JpegInfo.XSize;	//JPEGͼƬX��С
	YSize = JpegInfo.YSize;	//JPEGͼƬY��С
	int xx = LCD_GetXSize();
	int yy = LCD_GetYSize();
	if (XSize > LCD_GetXSize() || YSize > LCD_GetYSize()) {
		f_close(&JPEGFile);		//�ر�BMPFile�ļ�
		return 0;
	}
	switch(mode)
	{
		case 0:	//��ָ��λ����ʾͼƬ
			if((member == 1) && (denom == 1)) //�������ţ�ֱ�ӻ���
			{
				GUI_JPEG_DrawEx(JpegGetData,&JPEGFile,x,y);//��ָ��λ����ʾBMPͼƬ
			}else //����ͼƬ��Ҫ����
			{
				GUI_JPEG_DrawScaledEx(JpegGetData,&JPEGFile,x,y,member,denom);
			}
			break;
		case 1:	//��LCD�м���ʾͼƬ
			if((member == 1) && (denom == 1)) //�������ţ�ֱ�ӻ���
			{
				//��LCD�м���ʾͼƬ
				GUI_JPEG_DrawEx(JpegGetData,&JPEGFile,(lcddev.width-XSize)/2-1,(lcddev.height-YSize)/2-1);
			}else //����ͼƬ��Ҫ����
			{
				Xflag = (float)XSize*((float)member/(float)denom);
				Yflag = (float)YSize*((float)member/(float)denom);
				XSize = (lcddev.width-(int)Xflag)/2-1;
				YSize = (lcddev.height-(int)Yflag)/2-1;
				GUI_JPEG_DrawScaledEx(JpegGetData,&JPEGFile,XSize,YSize,member,denom);
			}
			break;
	}
	f_close(&JPEGFile);		//�ر�BMPFile�ļ�
	return 0;
}	
int dumphex(const unsigned char *buf, int size);


#define JPEG_DELAY_MS 1000
void jpegdisplay_demo(void)
{
	char buf[100];
	int num = 0;
	while(1)
	{
		vvsprintf(buf, sizeof(buf), "0:/320x480/320x480_%d.jpg", num%30+1);
		displayjpegex(buf,0,0,0,1,1);
		//displyjpeg(buf,0,0,0,1,1);
		GUI_Delay(100);
		memset(buf, 0, sizeof(buf));
		vvsprintf(buf, sizeof(buf), "��%d��ͼƬ", num++%30+1);
		GUI_DispStringHCenterAt(GB2312_TO_UTF8_LOCAL(buf), LCD_GetXSize()/2, 0);
		GUI_Delay(JPEG_DELAY_MS);
		GUI_Clear();
	}
}
