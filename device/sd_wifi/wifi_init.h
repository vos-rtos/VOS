#ifndef __WIFI_INIT_H
#define __WIFI_INIT_H

// 6.9 Card Common Control Registers (CCCR)
#define SDIO_CCCR_IOEN 0x02
#define SDIO_CCCR_IOEN_IOE1 _BV(1)

#define SDIO_CCCR_IORDY 0x03
#define SDIO_CCCR_IORDY_IOR1 _BV(1)

#define SDIO_CCCR_INTEN 0x04
#define SDIO_CCCR_INTEN_IENM _BV(0)
#define SDIO_CCCR_INTEN_IEN1 _BV(1)

#define SDIO_CCCR_BUSIFCTRL 0x07 // Bus Interface Control
#define SDIO_CCCR_BUSIFCTRL_BUSWID_1Bit 0
#define SDIO_CCCR_BUSIFCTRL_BUSWID_4Bit 0x02
#define SDIO_CCCR_BUSIFCTRL_BUSWID_8Bit 0x03

// ����88W8801�Ĵ���˵��, �����Linux��������SD-UAPSTA-8801-FC18-MMC-14.76.36.p61-C3X14090_B0-GPL.tar�е�
// SD-8801-FC18-MMC-14.76.36.p61-C3X14090_B0-mlan-src.tgz�е�
// SD-8801-FC18-MMC-14.76.36.p61-C3X14090_B0-GPL/wlan_src/mlan/mlan_sdio.h�ļ�

// �ж�ʹ�ܼĴ���
#define WIFI_INTMASK 0x02 // Host Interrupt Mask
#define WIFI_INTMASK_HOSTINTMASK 0x03 // enable/disable SDU to SD host interrupt

// �жϱ�־λ�Ĵ���
#define WIFI_INTSTATUS 0x03 // Host Interrupt Status
#define WIFI_INTSTATUS_ALL 0x03
#define WIFI_INTSTATUS_DNLD _BV(1) // Download Host Interrupt Status
#define WIFI_INTSTATUS_UPLD _BV(0) // Upload Host Interrupt Status (����ʱ�ֶ����, ����UPLDCARDRDY�Ƿ�Ϊ1)

// ��ʾ��Щͨ�������ݿ��Զ�ȡ
#define WIFI_RDBITMAPL 0x04
#define WIFI_RDBITMAPU 0x05

// ��ʾ��Щͨ�����Է�������
#define WIFI_WRBITMAPL 0x06
#define WIFI_WRBITMAPU 0x07

// ͨ��0��������¼�ͨ��, ��Ӧ�ı�ʾ���ݴ�С�ļĴ�����0x08��0x09
#define WIFI_RDLENP0L 0x08 // Read length for port 0
#define WIFI_RDLENP0U 0x09
// ͨ��1~15������ͨ��, ��Ӧ�ı�ʾ���ݴ�С�ļĴ�����0x0a~0x27

// ��״̬�Ĵ��� (���Ƽ�ʹ��)
#define WIFI_CARDSTATUS 0x30 // Card Status
//#define WIFI_CARDSTATUS_IOREADY _BV(3) // I/O Ready Indicator
//#define WIFI_CARDSTATUS_CISCARDRDY _BV(2) // Card Information Structure Card Ready
//#define WIFI_CARDSTATUS_UPLDCARDRDY _BV(1) // Upload Card Ready (CMD53��д������������λ)
//#define WIFI_CARDSTATUS_DNLDCARDRDY _BV(0) // Download Card Ready

// �̼����ݷ������Ĵ���
#define WIFI_SQREADBASEADDR0 0x40 // required firmware length
#define WIFI_SQREADBASEADDR1 0x41

// �̼�״̬�Ĵ���
#define WIFI_SCRATCH0_0 0x60 // 88W8801 firmware status
#define WIFI_SCRATCH0_1 0x61

#define WIFI_SCRATCH0_2 0x62
#define WIFI_SCRATCH0_3 0x63

// ��ʾͨ��0��CMD53��ַ�ļĴ���
#define WIFI_IOPORT0 0x78
#define WIFI_IOPORT1 0x79
#define WIFI_IOPORT2 0x7a

#define WIFI_FIRMWARESTATUS_OK 0xfedc

// 16.5 SDIO Card Metaformat
typedef enum
{
  CISTPL_NULL = 0x00, // Null tuple
  CISTPL_VERS_1 = 0x15, // Level 1 version/product-information
  CISTPL_MANFID = 0x20, // Manufacturer Identification String Tuple
  CISTPL_FUNCID = 0x21, // Function Identification Tuple
  CISTPL_FUNCE = 0x22, // Function Extensions
  CISTPL_END = 0xff // The End-of-chain Tuple
} WiFi_SDIOTupleCode;

void WiFi_DownloadFirmware(void);
uint16_t WiFi_GetFirmwareStatus(void);
uint32_t WiFi_GetPortAddress(port_t port);
void WiFi_Init(void);
void WiFi_ShowCIS(void);

#endif
