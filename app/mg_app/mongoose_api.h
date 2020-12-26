#ifndef __MONGOOSE_API_H__
#define __MONGOOSE_API_H__
#include "vos.h"

extern struct StMgUserInfo;
typedef s32 (*FUN_PTR)(struct StMgUserInfo *priv);

enum {
	MG_STATE_INIT = 0,
	MG_STATE_CONNECTED,
	MG_STATE_CLOSED,
	MG_STATE_ERROR,
};

typedef struct StMgUserInfo {
	s8 	*url;
	s32 state;
	s32 recv_totals; //��ǰ���յ�������
	s32 recv_add; //���һ�ν������ݶδ�С
	FUN_PTR fdo_callback;
	u32 timemark_start; //��¼�����Ŀ�ʼʱ��
	u32 timemark_now; //��¼��ǰ��ʱ��
}StMgUserInfo;

s32 mongoose_download(char *url, FUN_PTR pfun, u32 timeout_ms);

#endif
