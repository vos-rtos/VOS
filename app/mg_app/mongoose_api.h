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
	s8  *data;  //回调函数里每次添加数据的地址位置
	s32 state;
	s32 recv_totals; //当前接收的总数据
	s32 recv_add; //最后一次接收数据段大小，就是data指针的长度
	FUN_PTR fdo_callback;
	u32 timemark_start; //记录启动的开始时间
	u32 timemark_now; //记录当前的时间
	void *priv;
}StMgUserInfo;

s32 mongoose_download(char *url, FUN_PTR pfun, void *user_data, u32 timeout_ms);
s32 mongoose_http(char *url, FUN_PTR pfun, void *user_data, u32 timeout_ms);
#endif
