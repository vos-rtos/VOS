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
			pUser->fdo_callback(pUser);
		}
		pUser->recv_totals += c->recv.len;
		mg_iobuf_delete(&c->recv, c->recv.len);
	}
	else if (ev == MG_EV_CLOSE) {
		pUser->timemark_now = VOSGetTimeMs();
		pUser->state = MG_STATE_CLOSED;
		u32 time_span = pUser->timemark_now - pUser->timemark_start;
		kprintf("info: mongoose_download_event closed: %d(B), %d(ms), speed: %f(KB/s)!\r\n",
				pUser->recv_totals,  time_span, (float)(pUser->recv_totals)/time_span);
		c->is_closing = 1;
	}
}

s32 mongoose_download(char *url, FUN_PTR pfun, u32 timeout_ms)
{
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
		  break;
	  }
	  if (user.state == MG_STATE_ERROR) {
		  kprintf("info: download ERROR!\r\n");
		  break;
	  }
	  if (user.timemark_now - user.timemark_start > timeout_ms) {
		  kprintf("info: download timeout!\r\n");
		  break;
	  }
	  mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);
  return 0;
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
static const char *url = "http://bmob-cdn-26324.bmobpay.com/2020/12/26/e2745ea64022dcac801f6f46bdeb15df.zip";
void test_mg_download()
{
	mongoose_download(url, write_disk, 20*1000);
}


