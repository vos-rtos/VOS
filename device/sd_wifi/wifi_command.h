#ifndef __WIFI_COMMAND_H
#define __WIFI_COMMAND_H

#include "vos.h"

// 命令通道状态
#define WIFI_COMMANDSTATUS_LOCKED _BV(0)
#define WIFI_COMMANDSTATUS_BUSY _BV(1)

// 确定bss字段的值
// type: STA=0, uAP=1
// number: 0~15
#define WIFI_BSS(type, number) ((((type) & 1) << 4) | ((number) & 15))
#define WIFI_BSSTYPE(bss) (((bss) >> 4) & 1)
#define WIFI_BSSNUM(bss) ((bss) & 15)

// 已知结构体大小sizeof(tlv), 求数据域的大小, 一般用于给header.length赋值
// 例如定义一个MrvlIETypes_CfParamSet_t param变量, 赋值param.header.length=TLV_PAYLOADLEN(param)
#define TLV_PAYLOADLEN(tlv) (sizeof(tlv) - sizeof((tlv).header))

// 已知数据域大小, 求整个结构体的大小
// 例如定义一个很大的buffer, 然后定义一个IEEEType *的指针p指向该buffer
// buffer接收到数据后, 要求出接收到的IEEEType数据的实际大小显然不能用sizeof(IEEEType), 因为定义IEEEType结构体时data的长度定义的是1
// 此时就应该使用TLV_STRUCTLEN(*p)
#define TLV_STRUCTLEN(tlv) (sizeof((tlv).header) + (tlv).header.length)

// 已知本TLV的地址和大小, 求下一个TLV的地址
#define TLV_NEXT(tlv) ((u8 *)(tlv) + TLV_STRUCTLEN(*(tlv)))

// 部分WiFi命令的action字段
typedef enum
{
  WIFI_ACT_GET = 0,
  WIFI_ACT_SET = 1,
  WIFI_ACT_ADD = 2,
  WIFI_ACT_BITWISE_SET = 2,
  WIFI_ACT_BITWISE_CLR = 3,
  WIFI_ACT_REMOVE = 4
} WiFi_CommandAction;

// WiFi命令列表
typedef enum
{
  CMD_802_11_SCAN = 0x0006, // Starts the scan process
  CMD_MAC_MULTICAST_ADR = 0x0010, // Gets/sets MAC multicast filter table
  CMD_802_11_ASSOCIATE = 0x0012, // Initiate an association with the AP
  //CMD_802_11_SET_WEP = 0x0013, // Configures the WEP keys
  CMD_802_11_DEAUTHENTICATE = 0x0024, // Starts de-authentication process with the AP
  CMD_MAC_CONTROL = 0x0028, // Controls hardware MAC
  CMD_802_11_MAC_ADDR = 0x004d, // WLAN MAC address
  CMD_802_11_KEY_MATERIAL = 0x005e, // Gets/sets key material used to do Tx encryption or Rx decryption
  CMD_802_11_BG_SCAN_CONFIG = 0x006b, // Gets/sets background scan configuration
  CMD_802_11_BG_SCAN_QUERY = 0x006c, // Gets background scan results
  CMD_802_11_SUBSCRIBE_EVENT = 0x0075, // Subscribe to events and set thresholds
  CMD_TX_PKT_STATS = 0x008d, // Reports the current Tx packet status,
  CMD_CFG_DATA = 0x008f, // Facilitates downloading of calibration data
  APCMD_SYS_INFO = 0x00ae, // Returns system information
  APCMD_SYS_RESET = 0x00af, // Resets the uAP to its initial state
  APCMD_SYS_CONFIGURE = 0x00b0, // Gets/sets AP configuration parameters
  APCMD_BSS_START = 0x00b1, // Starts BSS
  APCMD_BSS_STOP = 0x00b2, // Stops active BSS
  APCMD_STA_LIST = 0x00b3, // Gets list of client stations currently associated with the AP
  APCMD_STA_DEAUTH = 0x00b5, // De-authenticates a client station
  CMD_SUPPLICANT_PMK = 0x00c4, // Configures embedded supplicant information
  CMD_CHAN_REPORT_REQUEST = 0x00dd // Performs channel load, channel noise histrogram, basic measurement on a given channel
} WiFi_CommandList;

// WiFi命令执行结果
typedef enum
{
  CMD_STATUS_SUCCESS = 0x0000, // No error
  CMD_STATUS_ERROR = 0x0001, // Command failed
  CMD_STATUS_UNSUPPORTED = 0x0002 // Command is not supported (result=2表示WiFi模块不支持此命令)
} WiFi_CommandResult;

// Table 45: IEEE 802.11 Standard IE Translated to Marvell IE
// PDF中的表45有一些拼写错误, MRVIIE应该改为MRVLIE
typedef enum
{
  WIFI_MRVLIETYPES_SSIDPARAMSET = 0x0000,
  WIFI_MRVLIETYPES_RATESPARAMSET = 0x0001,
  WIFI_MRVLIETYPES_PHYPARAMDSSET = 0x0003,
  WIFI_MRVLIETYPES_CFPARAMSET = 0x0004,
  WIFI_MRVLIETYPES_IBSSPARAMSET = 0x0006,
  WIFI_MRVLIETYPES_RSNPARAMSET = 0x0030,
  WIFI_MRVLIETYPES_VENDORPARAMSET = 0x00dd,
  WIFI_MRVLIETYPES_KEYPARAMSET = 0x0100,
  WIFI_MRVLIETYPES_CHANLISTPARAMSET = 0x0101,
  WIFI_MRVLIETYPES_TSFTIMESTAMP = 0x0113,
  WIFI_MRVLIETYPES_AUTHTYPE = 0x011f,
  WIFI_MRVLIETYPES_BSSIDLIST = 0x0123,
  WIFI_MRVLIETYPES_APBCASTSSIDCTRL = 0x0130,
  WIFI_MRVLIETYPES_PKTFWDCTRL = 0x0136,
  WIFI_MRVLIETYPES_WEPKEY = 0x013b,
  WIFI_MRVLIETYPES_WPAPASSPHRASE = 0x013c,
  WIFI_MRVLIETYPES_ENCRYPTIONPROTOCOL = 0x0140,
  WIFI_MRVLIETYPES_AKMP = 0x0141,
  WIFI_MRVLIETYPES_PASSPHRASE = 0x0145,
  WIFI_MRVLIETYPES_PTKCIPHER = 0x0191,
  WIFI_MRVLIETYPES_GTKCIPHER = 0x0192,
  WIFI_MRVLIETYPES_TXDATAPAUSE = 0x0194,
  WIFI_MRVLIETYPES_KEYPARAMSETV2 = 0x019c
} WiFi_MrvlIETypes;

// SDIO帧类型
typedef enum
{
  WIFI_SDIOFRAME_DATA = 0x00,
  WIFI_SDIOFRAME_COMMAND = 0x01,
  WIFI_SDIOFRAME_EVENT = 0x03
} WiFi_SDIOFrameType;

// 回调函数中的状态参数
typedef enum
{
  WIFI_STATUS_OK = 0, // 成功收到了回应
  WIFI_STATUS_FAIL = 1, // 未能完成请求的操作 (例如找到了AP热点但关联失败)
  WIFI_STATUS_BUSY = 2, // 之前的操作尚未完成
  WIFI_STATUS_NORESP = 3, // 没有收到回应
  WIFI_STATUS_MEM = 4, // 内存不足
  WIFI_STATUS_INVALID = 5, // 无效的参数
  WIFI_STATUS_NOTFOUND = 6, // 未找到目标 (如AP热点)
  WIFI_STATUS_INPROGRESS = 7, // 成功执行命令, 但还需要后续的操作 (比如关联AP成功但还需要后续的认证操作)
  WIFI_STATUS_UNSUPPORTED = 8 // 不支持该命令
} WiFi_Status;

typedef u8 bss_t;
typedef int8_t port_t;
typedef void (*WiFi_Callback)(void *arg, void *data, WiFi_Status status);

typedef struct
{
  u8 type;
  u8 length; // 数据域的大小
}__packed IEEEHeader;

// information element parameter
// 所有IEEETypes_*类型的基类型
typedef struct
{
  IEEEHeader header;
  u8 data[1];
}__packed IEEEType;

typedef struct
{
  u16 type;
  u16 length;
}__packed MrvlIEHeader;

// 所有MrvlIETypes_*类型的基类型
typedef struct
{
  MrvlIEHeader header;
  u8 data[1];
}__packed MrvlIEType;

// WiFi模块所有类型的帧的头部
typedef struct
{
  u16 length; // 大小包括此成员本身
  u16 type;
}__packed WiFi_SDIOFrameHeader;

// WiFi模块命令帧的头部
typedef struct
{
  WiFi_SDIOFrameHeader header;
  u16 cmd_code;
  u16 size;
  u8 seq_num;
  u8 bss;
  u16 result;
}__packed WiFi_CommandHeader;

typedef struct
{
  WiFi_Callback callback;
  void *arg;
  u8 busy;
  u32 timestamp;
} WiFi_CommandStatus;

// WiFi回调函数间传递的参数
typedef struct
{
  bss_t bss;
  WiFi_Callback saved_callback;
  void *saved_arg;
} WiFi_ArgBase;

#define WiFi_GetCommandCode(data) (((data) == NULL) ? 0 : (((const WiFi_CommandHeader *)(data))->cmd_code & 0x7fff))
#define WiFi_IsCommandResponse(data) ((data) != NULL && ((const WiFi_CommandHeader *)(data))->cmd_code & 0x8000)

WiFi_ArgBase *WiFi_AllocateArgument(u16 size, const char *func_name, bss_t bss, WiFi_Callback saved_callback, void *saved_arg);
void WiFi_CheckCommandTimeout(void);
void WiFi_CommandResponseProcess(WiFi_CommandHeader *resp);
void WiFi_DiscardData(void);
void WiFi_FreeArgument(void **parg, void *data, WiFi_Status status);
WiFi_CommandHeader *WiFi_GetCommandBuffer(WiFi_Callback callback, void *arg);
WiFi_CommandHeader *WiFi_GetSubCommandBuffer(void **parg);
int WiFi_IsCommandBusy(void);
int WiFi_IsCommandLocked(void);
int WiFi_ReleaseCommandBuffer(void);
int WiFi_SendCommand(WiFi_Callback callback, void *arg);
void WiFi_SetCommandHeader(WiFi_CommandHeader *cmd, bss_t bss, u16 code, u16 size, int inc_seqnum);
int WiFi_TranslateTLV(MrvlIEType *mrvlie_tlv, const IEEEType *ieee_tlv, u16 mrvlie_payload_size);

#endif
