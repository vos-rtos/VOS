#include "mongoose.h"

#include "mongoose_api.h"

extern int kprintf(char* format, ...);

#define RECV_BUF_MAX (1024*8) //增大应用的接收缓冲，可以提高接收速度, 最大下载速度达到510KB/s(ETH)
static void mongoose_download_event(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
	struct mg_str host;
	struct mg_http_message *hm = (struct mg_http_message *) ev_data;
	StMgUserInfo *pUser = 0;
	if (fn_data) {
		pUser = (StMgUserInfo *)fn_data;
	}
	if (pUser == 0) {
		c->is_closing = 1;
		return;
	}

	if (ev == MG_EV_ERROR) {
		c->is_closing = 1;
		pUser->timemark_now = VOSGetTimeMs();
		pUser->state = MG_STATE_ERROR;

	}
	if (ev == MG_EV_CONNECT) {
		host = mg_url_host(pUser->url);
		mg_printf(c, "GET %s HTTP/1.0\r\nHost: %.*s\r\n\r\n", mg_url_uri(pUser->url),
				  (int) host.len, host.ptr);
		pUser->timemark_now = VOSGetTimeMs();
		pUser->state = MG_STATE_CONNECTED;
	}
	else if (ev == MG_EV_READ) {
		//kprintf("info: len=%d, size=%d, totals=%d--\r\n", c->recv.len, c->recv.size, pUser->recv_totals);
		pUser->timemark_now = VOSGetTimeMs();
		if (c->recv.size < RECV_BUF_MAX) {
		  mg_iobuf_resize(&c->recv, RECV_BUF_MAX);
		}
		if (pUser->fdo_callback) {
			pUser->recv_add = c->recv.len;
			pUser->data = c->recv.len ? c->recv.buf : 0;
			if (pUser->fdo_callback(pUser) < 0) { //如果处理错误，立即断开
				pUser->state = MG_STATE_ERROR;
				c->is_closing = 1;
			}
		}
		pUser->recv_totals += c->recv.len;
		mg_iobuf_delete(&c->recv, c->recv.len);
	}
	else if (ev == MG_EV_CLOSE) {
		pUser->timemark_now = VOSGetTimeMs();
		if (pUser->state != MG_STATE_ERROR) {
			pUser->state = MG_STATE_CLOSED;
		}
		u32 time_span = pUser->timemark_now - pUser->timemark_start;
		kprintf("info: mongoose_download_event closed: %d(B), %d(ms), speed: %f(KB/s)!\r\n",
				pUser->recv_totals,  time_span, (float)(pUser->recv_totals)/time_span);
		c->is_closing = 1;
	}
}
/********************************************************************************************************
* 函数：s32 mongoose_download(char *url, FUN_PTR pfun, void *user_data, u32 timeout_ms);
* 描述:  下载文件请求函数，因为文件通常很大，内存没有足够空间一次性存放，只能通过回调函数边下载边写文件系统的方法。
* 参数:
* [1] url: url地址
* [2] pfun: 用户处理html body的数据，参数是StMgUserInfo结构体的指针
* [3] user_data: 用户需要获取body中某一项的内容，这个内容自己提供和自己在回调函数里处理。
* [4] timeout_ms: 执行整个http请求的超时时间，注意这里还包括dns的处理时间。
* 返回：ret >= 0, 获取数据成功， ret < 0, 获取数据失败（包括超时，或者内容没有自己想要的)
* 注意：无
*********************************************************************************************************/
s32 mongoose_download(char *url, FUN_PTR pfun, void *user_data, u32 timeout_ms)
{
	s32 ret = 0;
	struct mg_mgr mgr;
	StMgUserInfo user;
	mg_log_set("0");
	mg_mgr_init(&mgr);
	memset(&user, 0, sizeof(user));
	user.fdo_callback = pfun;
	user.timemark_now = user.timemark_start = VOSGetTimeMs();
	user.state = MG_STATE_INIT;
	user.url = url;
	mg_connect(&mgr, url, mongoose_download_event, &user);
	while (1) {
		if (user.state == MG_STATE_CLOSED) {
			kprintf("info: download ok!\r\n");
			ret = 0;
			break;
		}
		if (user.state == MG_STATE_ERROR) {
			kprintf("info: download ERROR!\r\n");
			ret = -1;
			break;
		}
		if (user.timemark_now - user.timemark_start > timeout_ms) {
			kprintf("info: download timeout!\r\n");
			ret = -2;
			break;
		}
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
	return ret;
}

static void mongoose_http_event(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
	struct mg_str host;
	struct mg_http_message *hm = (struct mg_http_message *) ev_data;
	StMgUserInfo *pUser = 0;
	if (fn_data) {
		pUser = (StMgUserInfo *)fn_data;
	}
	if (pUser == 0) {
		c->is_closing = 1;
		return;
	}

	if (ev == MG_EV_ERROR) {
		c->is_closing = 1;
		pUser->timemark_now = VOSGetTimeMs();
		pUser->state = MG_STATE_ERROR;

	}
	if (ev == MG_EV_CONNECT) {
		host = mg_url_host(pUser->url);
		mg_printf(c, "GET %s HTTP/1.0\r\nHost: %.*s\r\n\r\n", mg_url_uri(pUser->url),
				  (int) host.len, host.ptr);
		pUser->timemark_now = VOSGetTimeMs();
		pUser->state = MG_STATE_CONNECTED;

	}
	else if (ev == MG_EV_HTTP_MSG) {
	    if (hm) {
	    	//kprintf("%.*s", (int) hm->message.len, hm->message.ptr);
	    	kprintf("%.*s", (int) hm->body.len, hm->body.ptr);
			pUser->recv_add = hm->body.len;
			pUser->data = hm->body.len ? hm->body.ptr : 0;
			pUser->state = MG_STATE_CLOSED;
	    	if (pUser->fdo_callback) {
	    		if (pUser->fdo_callback(pUser) < 0) {
	    			pUser->state = MG_STATE_ERROR;
	    		}
	    	}
	    	pUser->recv_totals += hm->body.len;
	    }
	    c->is_closing = 1;
		u32 time_span = pUser->timemark_now - pUser->timemark_start;
		kprintf("info: mongoose_http_event closed: %d(B), %d(ms), speed: %f(KB/s)!\r\n",
				pUser->recv_totals,  time_span, (float)(pUser->recv_totals)/time_span);
	}
	else if (ev == MG_EV_CLOSE) {
		pUser->timemark_now = VOSGetTimeMs();
		if (pUser->state != MG_STATE_ERROR) {
			pUser->state = MG_STATE_CLOSED;
		}
		c->is_closing = 1;
	}
}

/********************************************************************************************************
* 函数：s32 mongoose_http(char *url, FUN_PTR pfun, void *user_data, u32 timeout_ms);
* 描述:  http请求函数
* 参数:
* [1] url: url地址
* [2] pfun: 用户处理html body的数据，参数是StMgUserInfo结构体的指针
* [3] user_data: 用户需要获取body中某一项的内容，这个内容自己提供和自己在回调函数里处理。
* [4] timeout_ms: 执行整个http请求的超时时间，注意这里还包括dns的处理时间。
* 返回：ret >= 0, 获取数据成功， ret < 0, 获取数据失败（包括超时，或者内容没有自己想要的)
* 注意：无
*********************************************************************************************************/
s32 mongoose_http(char *url, FUN_PTR pfun, void *user_data, u32 timeout_ms)
{
	s32 ret = 0;
	struct mg_mgr mgr;
	StMgUserInfo user;
	mg_log_set("0");
	mg_mgr_init(&mgr);
	memset(&user, 0, sizeof(user));
	user.fdo_callback = pfun;
	user.timemark_now = user.timemark_start = VOSGetTimeMs();
	user.state = MG_STATE_INIT;
	user.url = url;
	user.priv = user_data;
	mg_http_connect(&mgr, url, mongoose_http_event, &user);
	while (1) {
		if (user.state == MG_STATE_CLOSED) {
			kprintf("info: download ok!\r\n");
			ret = 0;
			break;
		}
		if (user.state == MG_STATE_ERROR) {
			kprintf("info: download ERROR!\r\n");
			ret = -1;
			break;
		}
		if (user.timemark_now - user.timemark_start > timeout_ms) {
			kprintf("info: download timeout!\r\n");
			ret = -2;
			break;
		}
		mg_mgr_poll(&mgr, 1000);
	}
	mg_mgr_free(&mgr);
	return ret;
}


s32 write_disk(StMgUserInfo *pUser)
{
	s32 ret = 0;
	if (pUser) {
		ret = pUser->recv_add;
		kprintf("info: write back disk recv_add=%d, totals=%d!\r\n",
				pUser->recv_add, pUser->recv_totals);
	}
	return ret;
}

void test_mg_download()
{
	mongoose_download("http://bmob-cdn-26324.bmobpay.com/2020/12/26/e2745ea64022dcac801f6f46bdeb15df.zip", write_disk, 0, 20*1000);
}

#include "cJSON.h"
s32 json_parser(StMgUserInfo *pUser)
{
	cJSON* root = 0;
	cJSON* json = 0;
	s32 ret = -1;
	if (pUser) {
		kprintf("info: write back disk recv_add=%d, totals=%d!\r\n",
				pUser->recv_add, pUser->recv_totals);
		if (pUser->data) {
			root = cJSON_Parse(pUser->data);
			if (root) {
				json = cJSON_GetObjectItem(root, "data");
				if (json) {
					json = cJSON_GetObjectItem(json, "t");
					if (json) {
						kprintf("==>%s<==\r\n", json->valuestring);
						if (pUser->priv) {
							strcpy(pUser->priv, json->valuestring);
							ret = 1; //获取到想要的数据，返回>=0
						}
					}
				}
			}
		}
	}
    if (root) {
        cJSON_Delete(root);
    }
	return ret;
}

void test_mg_http()
{
	s32 ret = 0;
	s8 timestamp[20] = {0};
	ret = mongoose_http("http://api.m.taobao.com/rest/api3.do?api=mtop.common.getTimestamp", json_parser, timestamp, 20*1000);
	if (ret >= 0) {
		kprintf("=====>timestamp=%s<=====!\r\n", timestamp);
	}
}
