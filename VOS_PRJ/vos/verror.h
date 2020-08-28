/********************************************************************************************************
* ��    Ȩ: Copyright (c) 2020, VOS Open source. All rights reserved.
* ��    ��: vos.c
* ��    ��: 156439848@qq.com; vincent_cws2008@gmail.com
* ��    ��: VOS V1.0
* ��    ʷ��
* --20200801�������ļ�
* --20200828�����ע��
*********************************************************************************************************/


#ifndef __VERROR_H__
#define __VERROR_H__

#define VERROR_OK				( 0)


#define VERROR_NO_ERROR			( 0) //�޴���
#define VERROR_PARAMETER		(-1) //��������
#define VERROR_TIMEOUT			(-2) //��ʱ
#define VERROR_NO_RESOURCES		(-3) //��Դ����
#define VERROR_WAIT_RESOURCES	(-4) //��Ҫ�����ȴ�
#define VERROR_DELETE_RESOURCES	(-5) //��Ҫ�����ȴ�
#define VERROR_INT_CORTEX		(-6) //�ж������Ĳ���ִ�д���
#define VERROR_MUTEX_RELEASE	(-7) //������ֻ��ӵ���߲����ͷ�
#define VERROR_NO_SCHEDULE		(-8) //ϵͳ��û��ʼ����
#define VERROR_EMPTY_RESOURCES	(-9) //��ԴΪ��
#define VERROR_FULL_RESOURCES	(-10) //��ԴΪ��
#define VERROR_TIMER_RUNNING	(-11) //��ʱ��������
#define VERROR_UNKNOWN			(-100) //��������


#endif
