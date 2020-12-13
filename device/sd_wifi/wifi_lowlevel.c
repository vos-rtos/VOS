/* 定义与单片机寄存器操作和模块接口相关的函数, 方便在不同平台间移植 */
// 单片机: STM32F407VE, 模块接口: SDIO

#include <stdio.h>
#include <stdlib.h>
#include <stm32f4xx.h>
#include <string.h>
#include "wifi.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_dma.h"

#define CMD52_WRITE _BV(31)
#define CMD52_READAFTERWRITE _BV(27)
#define CMD53_WRITE _BV(31)
#define CMD53_BLOCKMODE _BV(27)
#define CMD53_INCREMENTING _BV(26)
#define CMD53_TIMEOUT 10000000

static uint8_t WiFi_LowLevel_CalcClockDivider(uint32_t freq, uint32_t *preal);
static int WiFi_LowLevel_CheckError(const char *msg_title);
static uint16_t WiFi_LowLevel_GetBlockNum(uint8_t func, uint32_t *psize, uint32_t flags);
static void WiFi_LowLevel_GPIOInit(void);
static void WiFi_LowLevel_SDIOInit(void);
static void WiFi_LowLevel_SendCMD52(uint8_t func, uint32_t addr, uint8_t data, uint32_t flags);
static void WiFi_LowLevel_SendCMD53(uint8_t func, uint32_t addr, uint16_t count, uint32_t flags);
static int WiFi_LowLevel_SetSDIOBlockSize(uint32_t size);
#ifdef WIFI_FIRMWAREAREA_ADDR
static int WiFi_LowLevel_VerifyFirmware(void);
#endif
static void WiFi_LowLevel_WaitForResponse(const char *msg_title);

CRC_HandleTypeDef hcrc;
static uint8_t sdio_func_num = 0; // 功能区总数 (0号功能区除外)
static uint16_t sdio_block_size[2]; // 各功能区的块大小, 保存在此变量中避免每次都去发送CMD52命令读SDIO寄存器
static uint16_t sdio_rca; // RCA相对地址: 虽然SDIO标准规定SDIO接口上可以接多张SD卡, 但是STM32的SDIO接口只能接一张卡 (芯片手册上有说明)
static SDIO_CmdInitTypeDef sdio_cmd;
static SDIO_DataInitTypeDef sdio_data;

//必须声明kprintf，不然浮点数被当做整形处理
int kprintf(char* format, ...);

/* 计算SDIO时钟分频系数 */
static uint8_t WiFi_LowLevel_CalcClockDivider(uint32_t freq, uint32_t *preal)
{
  int divider;
  uint32_t sdioclk;
  uint64_t pll;
  RCC_OscInitTypeDef osc;
  
  HAL_RCC_GetOscConfig(&osc);
  if (osc.PLL.PLLSource == RCC_PLLSOURCE_HSE)
    pll = HSE_VALUE;
  else
    pll = HSI_VALUE;
  
  sdioclk = (pll * osc.PLL.PLLN) / (osc.PLL.PLLM * osc.PLL.PLLQ);
  if (freq == 0)
    freq = 1;
  
  divider = sdioclk / freq - 2;
  if (sdioclk % freq != 0)
    divider++; // 始终向上舍入, 保证实际频率不超过freq
  if (divider < 0)
    divider = 0;
  else if (divider > 255)
    divider = 255;
  
  *preal = sdioclk / (divider + 2);
  printf("[Clock] freq=%.1fkHz, requested=%.1fkHz, divider=%d\n", *preal / 1000.0f, freq / 1000.0f, divider);
  return divider & 0xff;
}

/* 检查并清除错误标志位 */
static int WiFi_LowLevel_CheckError(const char *msg_title)
{
  int err = 0;
  
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CCRCFAIL) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CCRCFAIL);
    err++;
    printf("%s: CMD%d CRC failed!\n", msg_title, sdio_cmd.CmdIndex);
  }
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CTIMEOUT) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CTIMEOUT);
    err++;
    printf("%s: CMD%d timeout!\n", msg_title, sdio_cmd.CmdIndex);
  }
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DCRCFAIL) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DCRCFAIL);
    err++;
    printf("%s: data CRC failed!\n", msg_title);
  }
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DTIMEOUT) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DTIMEOUT);
    err++;
    printf("%s: data timeout!\n", msg_title);
  }
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_STBITERR) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_STBITERR);
    err++;
    printf("%s: start bit error!\n", msg_title);
  }
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXUNDERR) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_TXUNDERR);
    err++;
    printf("%s: data underrun!\n", msg_title);
  }
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXOVERR) != RESET)
  {
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_RXOVERR);
    err++;
    printf("%s: data overrun!\n", msg_title);
  }
#if WIFI_USEDMA
  if (LL_DMA_IsActiveFlag_TE3(DMA2))
  {
    LL_DMA_ClearFlag_TE3(DMA2);
    err++;
    printf("%s: DMA transfer error!\n", msg_title);
  }
  if (LL_DMA_IsActiveFlag_DME3(DMA2))
  {
    LL_DMA_ClearFlag_DME3(DMA2);
    err++;
    printf("%s: DMA direct mode error!\n", msg_title);
  }
#endif
  return err;
}

/* 延时n毫秒 */
void WiFi_LowLevel_Delay(int nms)
{
  HAL_Delay(nms);
}

/* 打印数据内容 */
void WiFi_LowLevel_Dump(const void *data, int len)
{
  const uint8_t *p = data;
  
  while (len--)
    printf("%02X", *p++);
  printf("\n");
}

/* 判断应该采用哪种方式传输数据 */
// 返回值: 0为多字节模式, 否则表示块传输模式的数据块数
// *psize的值会做适当调整, 有可能大于原值
static uint16_t WiFi_LowLevel_GetBlockNum(uint8_t func, uint32_t *psize, uint32_t flags)
{
  uint16_t block_num = 0;
  
  if ((flags & WIFI_RWDATA_ALLOWMULTIBYTE) == 0 || *psize > 512) // 大于512字节时必须用数据块方式传输
  {
    // 块传输模式 (DTMODE=0)
    WiFi_LowLevel_SetSDIOBlockSize(sdio_block_size[func]);
    
    block_num = *psize / sdio_block_size[func];
    if (*psize % sdio_block_size[func] != 0)
      block_num++;
    *psize = block_num * sdio_block_size[func]; // 块数*块大小
  }
  else
  {
    // 多字节传输模式 (DTMODE=1)
    *psize = (*psize + 3) & ~3; // WiFi模块要求写入的字节数必须为4的整数倍
  }
  
  return block_num;
}

/* 获取WiFi模块支持的SDIO功能区个数 (0号功能区除外) */
uint8_t WiFi_LowLevel_GetFunctionNum(void)
{
  return sdio_func_num;
}

/* 判断是否触发了网卡中断 */
int WiFi_LowLevel_GetITStatus(uint8_t clear)
{
  if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_SDIOIT) != RESET)
  {
    if (clear)
      __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_SDIOIT);
    return 1;
  }
  else
    return 0;
}

uint32_t WiFi_LowLevel_GetTicks(void)
{
  return HAL_GetTick();
}

/* 初始化WiFi模块有关的所有GPIO引脚 */
static void WiFi_LowLevel_GPIOInit(void)
{
  GPIO_InitTypeDef gpio = {0};
  
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
#if 0
  // 使Wi-Fi模块复位信号(PDN)有效
  gpio.Mode = GPIO_MODE_OUTPUT_PP; // PD14设为推挽输出, 并立即输出低电平
  gpio.Pin = GPIO_PIN_14;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &gpio);
  
  // 撤销Wi-Fi模块的复位信号
  WiFi_LowLevel_Delay(100); // 延时一段时间, 使WiFi模块能够正确复位
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
#else
  // 使Wi-Fi模块复位信号(PDN)有效
  gpio.Mode = GPIO_MODE_OUTPUT_PP; // PD14设为推挽输出, 并立即输出低电平
  gpio.Pin = GPIO_PIN_3;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);
  
  // 撤销Wi-Fi模块的复位信号
  WiFi_LowLevel_Delay(100); // 延时一段时间, 使WiFi模块能够正确复位
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
#endif
  // SDIO相关引脚
  // PC8~11: SDIO_D0~3, PC12: SDIO_CK, 设为复用推挽输出
  gpio.Alternate = GPIO_AF12_SDIO;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOC, &gpio);
  
  // PD2: SDIO_CMD, 设为复用推挽输出
  gpio.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOD, &gpio);
}

void WiFi_LowLevel_Init(void)
{
  // 在此处打开WiFi模块所需要的除GPIO和SDIO外所有其他外设的时钟
  __HAL_RCC_CRC_CLK_ENABLE();
  
  hcrc.Instance = CRC;
  HAL_CRC_Init(&hcrc);
  
  // 检查Flash中保存的固件内容是否已被破坏
#ifdef WIFI_FIRMWAREAREA_ADDR
  if (!WiFi_LowLevel_VerifyFirmware())
  {
    printf("Error: The firmware stored in flash memory is corrupted!\n");
    printf("Either run flash_saver program, or remove the definition of WIFI_FIRMWAREAREA_ADDR in WiFi.h\n");
    abort();
  }
#endif
  
  WiFi_LowLevel_GPIOInit();
  WiFi_LowLevel_SDIOInit();
}

/* 接收数据, 自动判断采用哪种传输模式 */
// size为要接收的字节数, bufsize为data缓冲区的大小
// 若bufsize=0, 则只读取数据, 但不保存到data中, 此时data可以为NULL
int WiFi_LowLevel_ReadData(uint8_t func, uint32_t addr, void *data, uint32_t size, uint32_t bufsize, uint32_t flags)
{
  int i, err = 0;
  uint16_t block_num; // 数据块个数
  uint32_t cmd53_flags = 0;
#if WIFI_USEDMA
  uint32_t temp; // 丢弃数据用的变量
  LL_DMA_InitTypeDef dma;
#else
  uint32_t *p = data;
#endif
  
  if ((uintptr_t)data & 3)
  {
    // DMA每次传输多个字节时, 内存和外设地址必须要对齐, 否则将不能正确传输且不会提示错误
    printf("%s: data must be 4-byte aligned!\n", __FUNCTION__);
    return -2; // 不重试
  }
  if (size == 0)
  {
    printf("%s: size cannot be 0!\n", __FUNCTION__);
    return -2;
  }
  
  block_num = WiFi_LowLevel_GetBlockNum(func, &size, flags);
  if (bufsize != 0 && bufsize < size)
  {
    printf("%s: a buffer of at least %d bytes is required! bufsize=%d\n", __FUNCTION__, size, bufsize);
    return -2;
  }
  
#if WIFI_USEDMA
  dma.Channel = LL_DMA_CHANNEL_4;
  dma.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
  dma.FIFOMode = LL_DMA_FIFOMODE_ENABLE; // 必须使用FIFO模式
  dma.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
  dma.MemBurst = LL_DMA_MBURST_INC4;
  dma.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
  dma.Mode = LL_DMA_MODE_PFCTRL; // 必须由SDIO控制DMA传输的数据量
  dma.NbData = 0;
  dma.PeriphBurst = LL_DMA_PBURST_INC4;
  dma.PeriphOrM2MSrcAddress = (uint32_t)&SDIO->FIFO;
  dma.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_WORD;
  dma.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
  dma.Priority = LL_DMA_PRIORITY_VERYHIGH;
  if (bufsize > 0)
  {
    dma.MemoryOrM2MDstAddress = (uint32_t)data;
    dma.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  }
  else
  {
    // 数据丢弃模式
    dma.MemoryOrM2MDstAddress = (uint32_t)&temp;
    dma.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_NOINCREMENT;
  }
  LL_DMA_Init(DMA2, LL_DMA_STREAM_3, &dma);
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_3);
#endif
  
  if (flags & WIFI_RWDATA_ADDRINCREMENT)
    cmd53_flags |= CMD53_INCREMENTING;
  if (block_num)
  {
    sdio_data.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    WiFi_LowLevel_SendCMD53(func, addr, block_num, cmd53_flags | CMD53_BLOCKMODE);
  }
  else
  {
    sdio_data.TransferMode = SDIO_TRANSFER_MODE_STREAM;
    WiFi_LowLevel_SendCMD53(func, addr, size, cmd53_flags);
  }
  
  sdio_data.DataLength = size;
  sdio_data.DPSM = SDIO_DPSM_ENABLE;
  sdio_data.TransferDir = SDIO_TRANSFER_DIR_TO_SDIO;
  SDIO_ConfigData(SDIO, &sdio_data);
  
#if !WIFI_USEDMA
  while (size > 0)
  {
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXDAVL) != RESET)
    {
      // 如果有数据到来就读取数据
      size -= 4;
      if (bufsize > 0)
        *p++ = SDIO_ReadFIFO(SDIO);
      else
        SDIO_ReadFIFO(SDIO); // 读寄存器, 但不保存数据
    }
    else
    {
      // 如果出现错误, 则退出循环
      err += WiFi_LowLevel_CheckError(__FUNCTION__);
      if (err)
        break;
    }
  }
#endif
  
  // 等待数据接收完毕
  i = 0;
  while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT) != RESET || __SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND) == RESET)
  {
    err += WiFi_LowLevel_CheckError(__FUNCTION__);
    if (err)
      break;
    
    i++;
    if (i == CMD53_TIMEOUT)
    {
      printf("%s: timeout!\n", __FUNCTION__);
      err++;
      break;
    }
  }
  sdio_data.DPSM = SDIO_DPSM_DISABLE;
  SDIO_ConfigData(SDIO, &sdio_data);
#if WIFI_USEDMA
  LL_DMA_ClearFlag_TC3(DMA2); // 清除DMA传输完成标志位
  LL_DMA_ClearFlag_FE3(DMA2); // 忽略FIFO error错误
  LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_3); // 关闭DMA
#endif
  
  // 清除相关标志位
  __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND | SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  err += WiFi_LowLevel_CheckError(__FUNCTION__);
  if (err != 0)
    return -1;
  return 0;
}

/* 读SDIO寄存器 */
uint8_t WiFi_LowLevel_ReadReg(uint8_t func, uint32_t addr)
{
  WiFi_LowLevel_SendCMD52(func, addr, 0, 0);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  return SDIO_GetResponse(SDIO, SDIO_RESP1) & 0xff;
}

/* 初始化SDIO外设并完成WiFi模块的枚举 */
// SDIO Simplified Specification Version 3.00: 3. SDIO Card Initialization
static void WiFi_LowLevel_SDIOInit(void)
{
  SDIO_InitTypeDef sdio = {0};
  uint32_t freq, resp;
  
  // 在F4单片机中, 必须要开启PLL才能使用SDIO外设
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET)
  {
    __HAL_RCC_PLL_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET);
    printf("PLL is enabled!\n");
  }
  
  // SDIO外设拥有两个时钟: SDIOCLK=48MHz (分频后用于产生SDIO_CK=PC12引脚时钟), APB2 bus clock=PCLK2=84MHz (用于访问寄存器)
  __HAL_RCC_SDIO_CLK_ENABLE();
#if WIFI_USEDMA
  __HAL_RCC_DMA2_CLK_ENABLE();
#endif
  
  SDIO_PowerState_ON(SDIO); // 打开SDIO外设
  sdio.ClockDiv = WiFi_LowLevel_CalcClockDivider(400000, &freq); // 初始化时最高允许的频率: 400kHz
  SDIO_Init(SDIO, sdio);
  __SDIO_ENABLE(SDIO);
  
  __SDIO_OPERATION_ENABLE(SDIO); // 设为SDIO模式
#if WIFI_USEDMA
  WiFi_LowLevel_Delay(1); // 在F4上打开SDIO DMA前必须延时, 否则会失败
  __SDIO_DMA_ENABLE(SDIO); // 设为DMA传输模式
#endif
  
  // 不需要发送CMD0, 因为SD I/O card的初始化命令是CMD52
  // An I/O only card or the I/O portion of a combo card is NOT reset by CMD0. (See 4.4 Reset for SDIO)
  WiFi_LowLevel_Delay(10); // 延时可防止CMD5重发
  
  /* 发送CMD5: IO_SEND_OP_COND */
  sdio_cmd.Argument = 0;
  sdio_cmd.CmdIndex = 5;
  sdio_cmd.CPSM = SDIO_CPSM_ENABLE;
  sdio_cmd.Response = SDIO_RESPONSE_SHORT; // 接收短回应
  sdio_cmd.WaitForInterrupt = SDIO_WAIT_NO;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  printf("RESPCMD%d, RESP1_%08x\n", SDIO_GetCommandResponse(SDIO), SDIO_GetResponse(SDIO, SDIO_RESP1));
  
  /* 设置参数VDD Voltage Window: 3.2~3.4V, 并再次发送CMD5 */
  sdio_cmd.Argument = 0x300000;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  resp = SDIO_GetResponse(SDIO, SDIO_RESP1);
  printf("RESPCMD%d, RESP1_%08x\n", SDIO_GetCommandResponse(SDIO), resp);
  if (resp & _BV(31))
  {
    // Card is ready to operate after initialization
    sdio_func_num = (resp >> 28) & 7;
    printf("Number of I/O Functions: %d\n", sdio_func_num);
    printf("Memory Present: %d\n", (resp & _BV(27)) != 0);
  }
  
  /* 获取WiFi模块地址 (CMD3: SEND_RELATIVE_ADDR, Ask the card to publish a new relative address (RCA)) */
  sdio_cmd.Argument = 0;
  sdio_cmd.CmdIndex = 3;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  sdio_rca = SDIO_GetResponse(SDIO, SDIO_RESP1) >> 16;
  printf("Relative Card Address: 0x%04x\n", sdio_rca);
  
  /* 选中WiFi模块 (CMD7: SELECT/DESELECT_CARD) */
  sdio_cmd.Argument = sdio_rca << 16;
  sdio_cmd.CmdIndex = 7;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  printf("Card selected! RESP1_%08x\n", SDIO_GetResponse(SDIO, SDIO_RESP1));
  
  /* 提高时钟频率, 并设置数据超时时间为0.1s */
  sdio.ClockDiv = WiFi_LowLevel_CalcClockDivider(WIFI_CLOCK_FREQ, &freq);
  sdio_data.DataTimeOut = freq / 10;
  
  /* SDIO外设的总线宽度设为4位 */
  sdio.BusWide = SDIO_BUS_WIDE_4B;
  SDIO_Init(SDIO, sdio);
  WiFi_LowLevel_WriteReg(0, SDIO_CCCR_BUSIFCTRL, WiFi_LowLevel_ReadReg(0, SDIO_CCCR_BUSIFCTRL) | SDIO_CCCR_BUSIFCTRL_BUSWID_4Bit);
}

static void WiFi_LowLevel_SendCMD52(uint8_t func, uint32_t addr, uint8_t data, uint32_t flags)
{
  sdio_cmd.Argument = (func << 28) | (addr << 9) | data | flags;
  sdio_cmd.CmdIndex = 52;
  sdio_cmd.CPSM = SDIO_CPSM_ENABLE;
  sdio_cmd.Response = SDIO_RESPONSE_SHORT;
  sdio_cmd.WaitForInterrupt = SDIO_WAIT_NO;
  SDIO_SendCommand(SDIO, &sdio_cmd);
}

static void WiFi_LowLevel_SendCMD53(uint8_t func, uint32_t addr, uint16_t count, uint32_t flags)
{
  // 当count=512时, 和0x1ff相与后为0, 符合SDIO标准
  sdio_cmd.Argument = (func << 28) | (addr << 9) | (count & 0x1ff) | flags;
  sdio_cmd.CmdIndex = 53;
  sdio_cmd.CPSM = SDIO_CPSM_ENABLE;
  sdio_cmd.Response = SDIO_RESPONSE_SHORT;
  sdio_cmd.WaitForInterrupt = SDIO_WAIT_NO;
  SDIO_SendCommand(SDIO, &sdio_cmd);
}

/* 设置WiFi模块功能区的数据块大小 */
int WiFi_LowLevel_SetBlockSize(uint8_t func, uint32_t size)
{
  if (WiFi_LowLevel_SetSDIOBlockSize(size) == -1)
    return -1;
  
  sdio_block_size[func] = size;
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x10, size & 0xff);
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x11, size >> 8);
  return 0;
}

/* 设置SDIO外设的数据块大小 */
static int WiFi_LowLevel_SetSDIOBlockSize(uint32_t size)
{
  switch (size)
  {
    case 1:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_1B;
      break;
    case 2:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_2B;
      break;
    case 4:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_4B;
      break;
    case 8:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_8B;
      break;
    case 16:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_16B;
      break;
    case 32:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_32B;
      break;
    case 64:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_64B;
      break;
    case 128:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_128B;
      break;
    case 256:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_256B;
      break;
    case 512:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
      break;
    case 1024:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_1024B;
      break;
    case 2048:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_2048B;
      break;
    case 4096:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_4096B;
      break;
    case 8192:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_8192B;
      break;
    case 16384:
      sdio_data.DataBlockSize = SDIO_DATABLOCK_SIZE_16384B;
      break;
    default:
      return -1;
  }
  return 0;
}

/* 检查Flash中保存的固件内容是否完整 */
#ifdef WIFI_FIRMWAREAREA_ADDR
static int WiFi_LowLevel_VerifyFirmware(void)
{
  uint32_t crc, len;
  
  if (WIFI_FIRMWARE_SIZE != 255536)
    return 0; // 检验成功, 但检验结果为固件不完整, 所以不返回-1, 返回0
  
  len = WIFI_FIRMWARE_SIZE / 4 + 2; // 固件区(包括CRC)总大小的1/4
  crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)WIFI_FIRMWAREAREA_ADDR, len);
  return crc == 0; // 返回1为完整, 0为不完整
}
#endif

/* 等待SDIO命令回应 */
static void WiFi_LowLevel_WaitForResponse(const char *msg_title)
{
  uint8_t i = 0;
  
  do
  {
    if (i == WIFI_LOWLEVEL_MAXRETRY)
      abort();
    
    if (i != 0)
      SDIO_SendCommand(SDIO, &sdio_cmd); // 重发命令
    i++;
    
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT) != RESET); // 等待命令发送完毕
    WiFi_LowLevel_CheckError(msg_title);
  } while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND) == RESET); // 如果没有收到回应, 则重试
  __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
}

/* 发送数据, 自动判断采用哪种传输模式 */
// size为要发送的字节数, bufsize为data缓冲区的大小, bufsize=0时禁用缓冲区检查
int WiFi_LowLevel_WriteData(uint8_t func, uint32_t addr, const void *data, uint32_t size, uint32_t bufsize, uint32_t flags)
{
  int i, err = 0;
  uint16_t block_num; // 数据块个数
  uint32_t cmd53_flags = CMD53_WRITE;
#if WIFI_USEDMA
  LL_DMA_InitTypeDef dma;
#else
  const uint32_t *p = data;
#endif
  
  if ((uintptr_t)data & 3)
  {
    printf("%s: data must be 4-byte aligned!\n", __FUNCTION__);
    return -2; // 不重试
  }
  if (size == 0)
  {
    printf("%s: size cannot be 0!\n", __FUNCTION__);
    return -2;
  }

  block_num = WiFi_LowLevel_GetBlockNum(func, &size, flags);
  if (bufsize != 0 && bufsize < size) // 只读缓冲区越界不会影响数据传输, 所以这只是一个警告
    printf("%s: a buffer of at least %d bytes is required! bufsize=%d\n", __FUNCTION__, size, bufsize);
  
  if (flags & WIFI_RWDATA_ADDRINCREMENT)
    cmd53_flags |= CMD53_INCREMENTING;
  if (block_num)
  {
    sdio_data.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    WiFi_LowLevel_SendCMD53(func, addr, block_num, cmd53_flags | CMD53_BLOCKMODE);
  }
  else
  {
    sdio_data.TransferMode = SDIO_TRANSFER_MODE_STREAM;
    WiFi_LowLevel_SendCMD53(func, addr, size, cmd53_flags);
  }
  WiFi_LowLevel_WaitForResponse(__FUNCTION__); // 必须要等到CMD53收到回应后才能开始发送数据
  
  // 开始发送数据
#if WIFI_USEDMA
  dma.Channel = LL_DMA_CHANNEL_4;
  dma.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  dma.FIFOMode = LL_DMA_FIFOMODE_ENABLE; // 必须使用FIFO模式
  dma.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
  dma.MemBurst = LL_DMA_MBURST_INC4;
  dma.MemoryOrM2MDstAddress = (uint32_t)data;
  dma.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
  dma.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  dma.Mode = LL_DMA_MODE_PFCTRL; // 必须由SDIO控制DMA传输的数据量
  dma.NbData = 0; // 由SDIO控制流量时, 该选项无效 (HTIF3也不会置位)
  dma.PeriphBurst = LL_DMA_PBURST_INC4;
  dma.PeriphOrM2MSrcAddress = (uint32_t)&SDIO->FIFO;
  dma.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_WORD;
  dma.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
  dma.Priority = LL_DMA_PRIORITY_VERYHIGH;
  LL_DMA_Init(DMA2, LL_DMA_STREAM_3, &dma);
  LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_3);
#endif
  
  sdio_data.DataLength = size;
  sdio_data.DPSM = SDIO_DPSM_ENABLE;
  sdio_data.TransferDir = SDIO_TRANSFER_DIR_TO_CARD;
  SDIO_ConfigData(SDIO, &sdio_data);
  
#if !WIFI_USEDMA
  while (size > 0)
  {
    size -= 4;
    SDIO_WriteFIFO(SDIO, (uint32_t *)p); // 向FIFO送入4字节数据
    p++;
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXFIFOF) != RESET); // 如果FIFO已满则等待
    
    // 如果出现错误, 则退出循环
    err += WiFi_LowLevel_CheckError(__FUNCTION__);
    if (err)
      break;
  }
#endif
  
  // 等待发送完毕
  i = 0;
  while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND) == RESET)
  {
    err += WiFi_LowLevel_CheckError(__FUNCTION__);
    if (err)
      break;
    
    i++;
    if (i == CMD53_TIMEOUT)
    {
      printf("%s: timeout!\n", __FUNCTION__); // 用于跳出TXACT始终不清零, 也没有错误标志位置位的情况
      err++;
      break;
    }
  }
  sdio_data.DPSM = SDIO_DPSM_DISABLE;
  SDIO_ConfigData(SDIO, &sdio_data);
#if WIFI_USEDMA
  LL_DMA_ClearFlag_TC3(DMA2); // 清除DMA传输完成标志位
  LL_DMA_ClearFlag_FE3(DMA2); // 忽略FIFO error错误
  LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_3); // 关闭DMA
#endif
  
  // 清除相关标志位
  __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  err += WiFi_LowLevel_CheckError(__FUNCTION__);
  if (err != 0)
    return -1;
  return 0;
}

/* 写寄存器, 返回写入后寄存器的实际内容 */
uint8_t WiFi_LowLevel_WriteReg(uint8_t func, uint32_t addr, uint8_t value)
{
  WiFi_LowLevel_SendCMD52(func, addr, value, CMD52_WRITE | CMD52_READAFTERWRITE);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  return SDIO_GetResponse(SDIO, SDIO_RESP1) & 0xff;
}
