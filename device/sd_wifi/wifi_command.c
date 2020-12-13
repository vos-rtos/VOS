#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi.h"

__attribute__((aligned)) static uint8_t wifi_buffer_command[256]; // 命令帧发送缓冲区
static WiFi_CommandStatus wifi_command_status; // 命令发送通道的使用情况

/* 分配指定大小的内存用于回调函数间传递参数, 若分配失败则调用回调函数 */
WiFi_ArgBase *WiFi_AllocateArgument(uint16_t size, const char *func_name, bss_t bss, WiFi_Callback saved_callback, void *saved_arg)
{
  WiFi_ArgBase *p;
  
  if (size < sizeof(WiFi_ArgBase))
    size = sizeof(WiFi_ArgBase);
  
  p = malloc(size);
  if (p == NULL)
  {
    printf("%s: malloc failed!\n", func_name);
    if (saved_callback != NULL)
      saved_callback(saved_arg, NULL, WIFI_STATUS_MEM);
    return NULL;
  }
  
  p->bss = bss;
  p->saved_callback = saved_callback;
  p->saved_arg = saved_arg;
  return p;
}

/* 命令回应超时检测 */
void WiFi_CheckCommandTimeout(void)
{
  uint32_t diff;
  WiFi_CommandHeader *cmd = (WiFi_CommandHeader *)wifi_buffer_command;
  
  if (WiFi_IsCommandBusy())
  {
    diff = WiFi_LowLevel_GetTicks() - wifi_command_status.timestamp;
    if (diff > WIFI_CMDRESP_TIMEOUT)
    {
      // 超时
      printf("Command 0x%04x timeout!\n", cmd->cmd_code);
      wifi_command_status.busy &= ~WIFI_COMMANDSTATUS_BUSY;
      WiFi_ReleaseCommandBuffer();
      
      if (wifi_command_status.callback != NULL)
        wifi_command_status.callback(wifi_command_status.arg, NULL, WIFI_STATUS_NORESP);
    }
  }
}

/* 处理收到的命令回应, 并调用用户设置的回调函数 */
void WiFi_CommandResponseProcess(WiFi_CommandHeader *resp)
{
  WiFi_CommandHeader *cmd = (WiFi_CommandHeader *)wifi_buffer_command;
  WiFi_Status status;
  
  if (resp->seq_num == cmd->seq_num) // 序号相符
  {
#if WIFI_DISPLAY_RESPTIME
    printf("CMDRESP 0x%04x at %dms\n", resp->cmd_code, WiFi_LowLevel_GetTicks() - wifi_command_status.timestamp);
#endif
    if (resp->result == CMD_STATUS_SUCCESS)
      status = WIFI_STATUS_OK; // 命令执行成功
    else if (resp->result == CMD_STATUS_UNSUPPORTED)
    {
      status = WIFI_STATUS_UNSUPPORTED; // Wi-Fi模块不支持此命令
      printf("Command 0x%04x is unsupported!\n", cmd->cmd_code);
    }
    else
      status = WIFI_STATUS_FAIL; // 命令执行失败
    
    // 调用回调函数
    if (WiFi_IsCommandBusy()) // 确保回调函数只调用一次
    {
      wifi_command_status.busy &= ~WIFI_COMMANDSTATUS_BUSY;
      WiFi_ReleaseCommandBuffer();
      
      if (wifi_command_status.callback != NULL)
        wifi_command_status.callback(wifi_command_status.arg, resp, status);
    }
  }
  else
    printf("Different command sequence number: %d!=%d\n", resp->seq_num, cmd->seq_num);
}

/* 丢弃收到但因程序错误一直未处理的数据或命令 */
void WiFi_DiscardData(void)
{
  int ret;
  port_t port;
  uint16_t len;
  uint32_t addr;
  
  for (port = 0; port <= WIFI_DATAPORTS_RX_NUM; port++)
  {
    len = WiFi_GetDataLength(port);
    if (len != 0)
    {
      addr = WiFi_GetPortAddress(port);
      ret = WiFi_LowLevel_ReadData(1, addr, NULL, len, 0, WIFI_RWDATA_ALLOWMULTIBYTE);
      if (ret == 0)
        printf("Discarded %d bytes of data on port %d\n", len, port);
      else
        printf("Failed to read port %d\n", port);
    }
  }
}

/* 释放存放参数的内存并调用保存的回调函数 */
void WiFi_FreeArgument(void **parg, void *data, WiFi_Status status)
{
  WiFi_ArgBase *base = *parg;
  
  if (base != NULL)
  {
    if (base->saved_callback != NULL)
      base->saved_callback(base->saved_arg, data, status);
    free(base);
    *parg = NULL;
  }
}

/* 获取命令发送缓冲区 */
// 如果之前的命令尚未执行完就请求执行新的命令, 则直接调用回调函数报告错误
// 获取发送缓冲区成功后, 如果执行了SendCommand函数, 则不用释放缓冲区, 否则一定要调用ReleaseCommandBuffer函数释放缓冲区
WiFi_CommandHeader *WiFi_GetCommandBuffer(WiFi_Callback callback, void *arg)
{
  WiFi_Status status = WIFI_STATUS_OK;
  
  // 发送新命令前必须确保之前的命令已经发送完毕并收到回应
  // See 4.2 Protocol: The command exchange protocol is serialized, the host driver must wait until 
  // it has received a command response for the current command request before it can send the next command request.
  if (WiFi_IsCommandBusy())
  {
    status = WIFI_STATUS_BUSY;
    printf("%s: The previous command is in progress!\n", __FUNCTION__);
  }
  else if (WiFi_IsCommandLocked())
  {
    status = WIFI_STATUS_MEM;
    printf("%s: The command buffer is not available!\n", __FUNCTION__);
  }
  
  if (status != WIFI_STATUS_OK)
  {
    if (callback != NULL)
      callback(arg, NULL, status);
    return NULL;
  }
  
  wifi_command_status.busy |= WIFI_COMMANDSTATUS_LOCKED;
  memset(wifi_buffer_command, 0, sizeof(WiFi_CommandHeader));
  return (WiFi_CommandHeader *)wifi_buffer_command;
}

WiFi_CommandHeader *WiFi_GetSubCommandBuffer(void **parg)
{
  WiFi_ArgBase *base = *parg;
  WiFi_CommandHeader *cmd;
  
  cmd = WiFi_GetCommandBuffer(base->saved_callback, base->saved_arg);
  if (cmd == NULL)
  {
    free(base);
    *parg = NULL;
  }
  return cmd;
}

/* 命令是否已提交, 但还未收到回应 */
int WiFi_IsCommandBusy(void)
{
  return (wifi_command_status.busy & WIFI_COMMANDSTATUS_BUSY) != 0;
}

/* 命令发送缓冲区是否正在使用中 */
int WiFi_IsCommandLocked(void)
{
  return (wifi_command_status.busy & WIFI_COMMANDSTATUS_LOCKED) != 0;
}

/* 释放命令发送缓冲区 */
int WiFi_ReleaseCommandBuffer(void)
{
  if (!WiFi_IsCommandBusy() && WiFi_IsCommandLocked())
  {
    wifi_command_status.busy &= ~WIFI_COMMANDSTATUS_LOCKED;
    return 0;
  }
  else
    return -1;
}

/* 发送WiFi命令, 当收到回应或超时时调用callback回调函数 */
// 只要调用了此函数, 就无需自己调用WiFi_ReleaseCommandBuffer函数释放缓冲区
int WiFi_SendCommand(WiFi_Callback callback, void *arg)
{
  int i, ret;
  uint32_t addr;
  WiFi_CommandHeader *cmd = (WiFi_CommandHeader *)wifi_buffer_command;
  WiFi_Status status = WIFI_STATUS_OK;
  
  if (cmd->header.length > sizeof(wifi_buffer_command))
  {
    printf("%s: Command buffer overflowed! code=0x%04x, size=%d\n", __FUNCTION__, cmd->cmd_code, cmd->header.length);
    abort();
  }
  
  if (!WiFi_IsCommandLocked())
  {
    // 命令缓冲区中没有放入要发送的命令
    printf("%s: The command body has not been put in the command buffer!\n", __FUNCTION__);
    status = WIFI_STATUS_INVALID;
  }
  else if (WiFi_IsCommandBusy())
  {
    // 发送命令前必须确保之前的命令已经发送完毕
    printf("%s: The command port is busy!\n", __FUNCTION__);
    status = WIFI_STATUS_BUSY;
  }
  
  if (status != WIFI_STATUS_OK)
  {
    if (callback != NULL)
      callback(arg, NULL, status);
    return -1;
  }
  
  i = 0;
  addr = WiFi_GetPortAddress(0);
  do
  {
	//dumphex(wifi_buffer_command, sizeof(wifi_buffer_command));
    ret = WiFi_LowLevel_WriteData(1, addr, wifi_buffer_command, cmd->header.length, sizeof(wifi_buffer_command), 0);
    i++;
  } while (ret == -1 && i < WIFI_LOWLEVEL_MAXRETRY);
  
  if (ret != 0)
  {
    if (callback != NULL)
      callback(arg, NULL, WIFI_STATUS_FAIL);
    return -1;
  }
  
  wifi_command_status.callback = callback;
  wifi_command_status.arg = arg;
  wifi_command_status.busy |= WIFI_COMMANDSTATUS_BUSY;
  wifi_command_status.timestamp = WiFi_LowLevel_GetTicks();
  return 0; // 表示命令提交成功
}

/* 设置命令头部字段 */
void WiFi_SetCommandHeader(WiFi_CommandHeader *cmd, bss_t bss, uint16_t code, uint16_t size, int inc_seqnum)
{
  static uint8_t seq_num = 0; // 88W8801中命令序列号为8位
  
  cmd->header.length = size;
  cmd->header.type = WIFI_SDIOFRAME_COMMAND;
  
  cmd->cmd_code = code;
  cmd->size = size - sizeof(WiFi_SDIOFrameHeader); // 命令大小包括命令头部, 但不包括SDIO帧头部
  cmd->seq_num = seq_num;
  cmd->bss = bss; // 88W8801新增字段
  cmd->result = 0;
  
  if (inc_seqnum)
    seq_num++;
}

/* 将IEEE型的TLV转换成MrvlIE型的TLV */
int WiFi_TranslateTLV(MrvlIEType *mrvlie_tlv, const IEEEType *ieee_tlv, uint16_t mrvlie_payload_size)
{
  mrvlie_tlv->header.type = ieee_tlv->header.type;
  if (ieee_tlv->header.length > mrvlie_payload_size)
    mrvlie_tlv->header.length = mrvlie_payload_size; // 若源数据大小超过缓冲区最大容量, 则丢弃剩余数据
  else
    mrvlie_tlv->header.length = ieee_tlv->header.length;
  memset(mrvlie_tlv->data, 0, mrvlie_payload_size); // 清零
  memcpy(mrvlie_tlv->data, ieee_tlv->data, mrvlie_tlv->header.length); // 复制数据
  
  // 返回值表示缓冲区大小是否足够
  if (mrvlie_tlv->header.length != ieee_tlv->header.length)
    return -1;
  return 0;
}
