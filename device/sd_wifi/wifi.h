#ifndef __WIFI_H
#define __WIFI_H

#define WIFI_CLOCK_FREQ 25000000 // 时钟频率 (最高25MHz, 不使用DMA传输时请适当降低频率)
#define WIFI_CMDRESP_TIMEOUT 30000 // WiFi命令帧回应的超时时间(ms)
#define WIFI_DATAPORTS_RX_NUM 15 // 数据接收通道个数
#define WIFI_DATAPORTS_TX_NUM 11 // 数据发送通道个数
#define WIFI_DISPLAY_ASSOCIATE_RESULT 1 // 显示关联热点的结果值
#define WIFI_DISPLAY_FIRMWARE_DNLD 0 // 显示固件下载过程
#define WIFI_DISPLAY_RESPTIME 1 // 显示命令帧和数据帧从发送到收到确认和回应所经过的时间
#define WIFI_DISPLAY_SCAN_DETAILS 1 // 扫描热点时显示其他详细信息
#define WIFI_LOWLEVEL_MAXRETRY 3 // 读写数据最大尝试次数
#define WIFI_LOWLEVEL_NOINTPIN 0 // SDIO中断引脚是否不可用
#define WIFI_USEDMA 1 // SDIO采用DMA方式收发数据 (推荐)
                      // 关闭DMA后, SDIO频率应该设为1MHz以下

//#define WIFI_FIRMWAREAREA_ADDR 0x08040000 // 固件存储区首地址

#ifdef WIFI_FIRMWAREAREA_ADDR
// 固件存储区的格式: 固件大小(4B)+固件内容+CRC校验码(4B)
#define WIFI_FIRMWARE_SIZE (*(const uint32_t *)WIFI_FIRMWAREAREA_ADDR)
#define WIFI_FIRMWARE_ADDR ((const uint8_t *)WIFI_FIRMWAREAREA_ADDR + 4)
#else
// 这里sd表示这两个固件是SDIO接口专用的, uap表示micro AP, sta表示station
// 取消WIFI_FIRMWAREAREA_ADDR宏定义后, 需要把sd8801_uapsta.c添加到工程中来, 并确保变量的声明已添加__attribute__((aligned))
extern const unsigned char firmware_sd8801[255536];
#define WIFI_FIRMWARE_SIZE sizeof(firmware_sd8801)
#define WIFI_FIRMWARE_ADDR firmware_sd8801
#endif

#ifndef _BV
#define _BV(n) (1u << (n))
#endif

// 字节序转换函数
#ifndef htons
#define htons(x) ((((x) & 0x00ffUL) << 8) | (((x) & 0xff00UL) >> 8))
#endif
#ifndef ntohs
#define ntohs htons
#endif

#define WIFI_MACADDR_LEN 6

#include "wifi_command.h"
#include "wifi_data.h"
#include "wifi_init.h"
#include "wifi_interrupt.h"
#include "wifi_lowlevel.h"
#include "wifi_sta.h"
#include "wifi_uap.h"

#endif
