#include "vtype.h"
#include "stdio.h"
#include "stdarg.h"
#include "snprintf.h"

int c_snprintf(char *buf, size_t buf_size, const char *fmt, ...) {
  int result;
  va_list ap;
  va_start(ap, fmt);
//  result = c_vsnprintf(buf, buf_size, fmt, ap);
  result = rpl_vsnprintf(buf, buf_size, fmt, ap);
  va_end(ap);
  return result;
}

s32 usbd_uart_app_puts(u8* buf, s32 len);
int usb_printf(char* format, ...)
{
	char temp[200];
	int i=0;
	va_list lst;
	va_start (lst, format);
//	i=c_vsnprintf(temp, sizeof(temp), format, lst);
	i = rpl_vsnprintf(temp, sizeof(temp), format, lst);
	if (i > 0) {
		usbd_uart_app_puts(temp, i);
	}
	va_end(lst);
	return i;
}

int kprintf(char* format, ...)
{
	char temp[64];
	char *pnew;
	int new_len;
	int num;

	va_list lst;
	va_start (lst, format);
	num = rpl_vsnprintf(temp, sizeof(temp), format, lst);
	if (num > sizeof(temp)) {
		new_len = num+1;
		pnew = vmalloc(new_len);
		if (pnew) {
			num = rpl_vsnprintf(pnew, new_len, format, lst);
			vputs(pnew, num);
			vfree(pnew);
		}
	}
	else {
		vputs(temp, num);
	}
	va_end(lst);
	return num;
}

int vvsprintf(char *buf, int len, char* format, ...)
{
	int i=0;
	va_list lst;
	va_start (lst, format);
//	i=c_vsnprintf(buf, len, format, lst);
	i = rpl_vsnprintf(buf, len, format, lst);
	va_end(lst);
	return i;
}

void dma_vputs(s8 *str, s32 len);
int dma_printf(char* format, ...)
{
	char temp[200];
	int i=0;
	va_list lst;
	va_start (lst, format);
//	i=c_vsnprintf(temp, sizeof(temp), format, lst);
	i = rpl_vsnprintf(temp, sizeof(temp), format, lst);
	if (i > 0) {
		dma_vputs(temp, i);
	}
	va_end(lst);
	return i;
}


