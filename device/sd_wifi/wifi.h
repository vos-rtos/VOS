#ifndef __WIFI_H
#define __WIFI_H

#define WIFI_CLOCK_FREQ 25000000 // ʱ��Ƶ�� (���25MHz, ��ʹ��DMA����ʱ���ʵ�����Ƶ��)
#define WIFI_CMDRESP_TIMEOUT 30000 // WiFi����֡��Ӧ�ĳ�ʱʱ��(ms)
#define WIFI_DATAPORTS_RX_NUM 15 // ���ݽ���ͨ������
#define WIFI_DATAPORTS_TX_NUM 11 // ���ݷ���ͨ������
#define WIFI_DISPLAY_ASSOCIATE_RESULT 1 // ��ʾ�����ȵ�Ľ��ֵ
#define WIFI_DISPLAY_FIRMWARE_DNLD 0 // ��ʾ�̼����ع���
#define WIFI_DISPLAY_RESPTIME 1 // ��ʾ����֡������֡�ӷ��͵��յ�ȷ�Ϻͻ�Ӧ��������ʱ��
#define WIFI_DISPLAY_SCAN_DETAILS 1 // ɨ���ȵ�ʱ��ʾ������ϸ��Ϣ
#define WIFI_LOWLEVEL_MAXRETRY 3 // ��д��������Դ���
#define WIFI_LOWLEVEL_NOINTPIN 0 // SDIO�ж������Ƿ񲻿���
#define WIFI_USEDMA 1 // SDIO����DMA��ʽ�շ����� (�Ƽ�)
                      // �ر�DMA��, SDIOƵ��Ӧ����Ϊ1MHz����

//#define WIFI_FIRMWAREAREA_ADDR 0x08040000 // �̼��洢���׵�ַ

#ifdef WIFI_FIRMWAREAREA_ADDR
// �̼��洢���ĸ�ʽ: �̼���С(4B)+�̼�����+CRCУ����(4B)
#define WIFI_FIRMWARE_SIZE (*(const uint32_t *)WIFI_FIRMWAREAREA_ADDR)
#define WIFI_FIRMWARE_ADDR ((const uint8_t *)WIFI_FIRMWAREAREA_ADDR + 4)
#else
// ����sd��ʾ�������̼���SDIO�ӿ�ר�õ�, uap��ʾmicro AP, sta��ʾstation
// ȡ��WIFI_FIRMWAREAREA_ADDR�궨���, ��Ҫ��sd8801_uapsta.c��ӵ���������, ��ȷ�����������������__attribute__((aligned))
extern const unsigned char firmware_sd8801[255536];
#define WIFI_FIRMWARE_SIZE sizeof(firmware_sd8801)
#define WIFI_FIRMWARE_ADDR firmware_sd8801
#endif

#ifndef _BV
#define _BV(n) (1u << (n))
#endif

// �ֽ���ת������
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
