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

// 关于88W8801寄存器说明, 请参阅Linux驱动程序SD-UAPSTA-8801-FC18-MMC-14.76.36.p61-C3X14090_B0-GPL.tar中的
// SD-8801-FC18-MMC-14.76.36.p61-C3X14090_B0-mlan-src.tgz中的
// SD-8801-FC18-MMC-14.76.36.p61-C3X14090_B0-GPL/wlan_src/mlan/mlan_sdio.h文件

// 中断使能寄存器
#define WIFI_INTMASK 0x02 // Host Interrupt Mask
#define WIFI_INTMASK_HOSTINTMASK 0x03 // enable/disable SDU to SD host interrupt

// 中断标志位寄存器
#define WIFI_INTSTATUS 0x03 // Host Interrupt Status
#define WIFI_INTSTATUS_ALL 0x03
#define WIFI_INTSTATUS_DNLD _BV(1) // Download Host Interrupt Status
#define WIFI_INTSTATUS_UPLD _BV(0) // Upload Host Interrupt Status (可随时手动清除, 无论UPLDCARDRDY是否为1)

// 表示哪些通道有数据可以读取
#define WIFI_RDBITMAPL 0x04
#define WIFI_RDBITMAPU 0x05

// 表示哪些通道可以发送数据
#define WIFI_WRBITMAPL 0x06
#define WIFI_WRBITMAPU 0x07

// 通道0是命令和事件通道, 对应的表示数据大小的寄存器是0x08和0x09
#define WIFI_RDLENP0L 0x08 // Read length for port 0
#define WIFI_RDLENP0U 0x09
// 通道1~15是数据通道, 对应的表示数据大小的寄存器是0x0a~0x27

// 卡状态寄存器 (不推荐使用)
#define WIFI_CARDSTATUS 0x30 // Card Status
//#define WIFI_CARDSTATUS_IOREADY _BV(3) // I/O Ready Indicator
//#define WIFI_CARDSTATUS_CISCARDRDY _BV(2) // Card Information Structure Card Ready
//#define WIFI_CARDSTATUS_UPLDCARDRDY _BV(1) // Upload Card Ready (CMD53读写命令均会清除该位)
//#define WIFI_CARDSTATUS_DNLDCARDRDY _BV(0) // Download Card Ready

// 固件数据发送量寄存器
#define WIFI_SQREADBASEADDR0 0x40 // required firmware length
#define WIFI_SQREADBASEADDR1 0x41

// 固件状态寄存器
#define WIFI_SCRATCH0_0 0x60 // 88W8801 firmware status
#define WIFI_SCRATCH0_1 0x61

#define WIFI_SCRATCH0_2 0x62
#define WIFI_SCRATCH0_3 0x63

// 表示通道0的CMD53地址的寄存器
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
