/* �����뵥Ƭ���Ĵ���������ģ��ӿ���صĺ���, �����ڲ�ͬƽ̨����ֲ */
// ��Ƭ��: STM32F407VE, ģ��ӿ�: SDIO

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
static uint8_t sdio_func_num = 0; // ���������� (0�Ź���������)
static uint16_t sdio_block_size[2]; // ���������Ŀ��С, �����ڴ˱����б���ÿ�ζ�ȥ����CMD52�����SDIO�Ĵ���
static uint16_t sdio_rca; // RCA��Ե�ַ: ��ȻSDIO��׼�涨SDIO�ӿ��Ͽ��ԽӶ���SD��, ����STM32��SDIO�ӿ�ֻ�ܽ�һ�ſ� (оƬ�ֲ�����˵��)
static SDIO_CmdInitTypeDef sdio_cmd;
static SDIO_DataInitTypeDef sdio_data;

//��������kprintf����Ȼ���������������δ���
int kprintf(char* format, ...);

/* ����SDIOʱ�ӷ�Ƶϵ�� */
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
    divider++; // ʼ����������, ��֤ʵ��Ƶ�ʲ�����freq
  if (divider < 0)
    divider = 0;
  else if (divider > 255)
    divider = 255;
  
  *preal = sdioclk / (divider + 2);
  printf("[Clock] freq=%.1fkHz, requested=%.1fkHz, divider=%d\n", *preal / 1000.0f, freq / 1000.0f, divider);
  return divider & 0xff;
}

/* ��鲢��������־λ */
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

/* ��ʱn���� */
void WiFi_LowLevel_Delay(int nms)
{
  HAL_Delay(nms);
}

/* ��ӡ�������� */
void WiFi_LowLevel_Dump(const void *data, int len)
{
  const uint8_t *p = data;
  
  while (len--)
    printf("%02X", *p++);
  printf("\n");
}

/* �ж�Ӧ�ò������ַ�ʽ�������� */
// ����ֵ: 0Ϊ���ֽ�ģʽ, �����ʾ�鴫��ģʽ�����ݿ���
// *psize��ֵ�����ʵ�����, �п��ܴ���ԭֵ
static uint16_t WiFi_LowLevel_GetBlockNum(uint8_t func, uint32_t *psize, uint32_t flags)
{
  uint16_t block_num = 0;
  
  if ((flags & WIFI_RWDATA_ALLOWMULTIBYTE) == 0 || *psize > 512) // ����512�ֽ�ʱ���������ݿ鷽ʽ����
  {
    // �鴫��ģʽ (DTMODE=0)
    WiFi_LowLevel_SetSDIOBlockSize(sdio_block_size[func]);
    
    block_num = *psize / sdio_block_size[func];
    if (*psize % sdio_block_size[func] != 0)
      block_num++;
    *psize = block_num * sdio_block_size[func]; // ����*���С
  }
  else
  {
    // ���ֽڴ���ģʽ (DTMODE=1)
    *psize = (*psize + 3) & ~3; // WiFiģ��Ҫ��д����ֽ�������Ϊ4��������
  }
  
  return block_num;
}

/* ��ȡWiFiģ��֧�ֵ�SDIO���������� (0�Ź���������) */
uint8_t WiFi_LowLevel_GetFunctionNum(void)
{
  return sdio_func_num;
}

/* �ж��Ƿ񴥷��������ж� */
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

/* ��ʼ��WiFiģ���йص�����GPIO���� */
static void WiFi_LowLevel_GPIOInit(void)
{
  GPIO_InitTypeDef gpio = {0};
  
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
#if 0
  // ʹWi-Fiģ�鸴λ�ź�(PDN)��Ч
  gpio.Mode = GPIO_MODE_OUTPUT_PP; // PD14��Ϊ�������, ����������͵�ƽ
  gpio.Pin = GPIO_PIN_14;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &gpio);
  
  // ����Wi-Fiģ��ĸ�λ�ź�
  WiFi_LowLevel_Delay(100); // ��ʱһ��ʱ��, ʹWiFiģ���ܹ���ȷ��λ
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
#else
  // ʹWi-Fiģ�鸴λ�ź�(PDN)��Ч
  gpio.Mode = GPIO_MODE_OUTPUT_PP; // PD14��Ϊ�������, ����������͵�ƽ
  gpio.Pin = GPIO_PIN_3;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);
  
  // ����Wi-Fiģ��ĸ�λ�ź�
  WiFi_LowLevel_Delay(100); // ��ʱһ��ʱ��, ʹWiFiģ���ܹ���ȷ��λ
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
#endif
  // SDIO�������
  // PC8~11: SDIO_D0~3, PC12: SDIO_CK, ��Ϊ�����������
  gpio.Alternate = GPIO_AF12_SDIO;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
  HAL_GPIO_Init(GPIOC, &gpio);
  
  // PD2: SDIO_CMD, ��Ϊ�����������
  gpio.Pin = GPIO_PIN_2;
  HAL_GPIO_Init(GPIOD, &gpio);
}

void WiFi_LowLevel_Init(void)
{
  // �ڴ˴���WiFiģ������Ҫ�ĳ�GPIO��SDIO���������������ʱ��
  __HAL_RCC_CRC_CLK_ENABLE();
  
  hcrc.Instance = CRC;
  HAL_CRC_Init(&hcrc);
  
  // ���Flash�б���Ĺ̼������Ƿ��ѱ��ƻ�
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

/* ��������, �Զ��жϲ������ִ���ģʽ */
// sizeΪҪ���յ��ֽ���, bufsizeΪdata�������Ĵ�С
// ��bufsize=0, ��ֻ��ȡ����, �������浽data��, ��ʱdata����ΪNULL
int WiFi_LowLevel_ReadData(uint8_t func, uint32_t addr, void *data, uint32_t size, uint32_t bufsize, uint32_t flags)
{
  int i, err = 0;
  uint16_t block_num; // ���ݿ����
  uint32_t cmd53_flags = 0;
#if WIFI_USEDMA
  uint32_t temp; // ���������õı���
  LL_DMA_InitTypeDef dma;
#else
  uint32_t *p = data;
#endif
  
  if ((uintptr_t)data & 3)
  {
    // DMAÿ�δ������ֽ�ʱ, �ڴ�������ַ����Ҫ����, ���򽫲�����ȷ�����Ҳ�����ʾ����
    printf("%s: data must be 4-byte aligned!\n", __FUNCTION__);
    return -2; // ������
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
  dma.FIFOMode = LL_DMA_FIFOMODE_ENABLE; // ����ʹ��FIFOģʽ
  dma.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
  dma.MemBurst = LL_DMA_MBURST_INC4;
  dma.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
  dma.Mode = LL_DMA_MODE_PFCTRL; // ������SDIO����DMA�����������
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
    // ���ݶ���ģʽ
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
      // ��������ݵ����Ͷ�ȡ����
      size -= 4;
      if (bufsize > 0)
        *p++ = SDIO_ReadFIFO(SDIO);
      else
        SDIO_ReadFIFO(SDIO); // ���Ĵ���, ������������
    }
    else
    {
      // ������ִ���, ���˳�ѭ��
      err += WiFi_LowLevel_CheckError(__FUNCTION__);
      if (err)
        break;
    }
  }
#endif
  
  // �ȴ����ݽ������
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
  LL_DMA_ClearFlag_TC3(DMA2); // ���DMA������ɱ�־λ
  LL_DMA_ClearFlag_FE3(DMA2); // ����FIFO error����
  LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_3); // �ر�DMA
#endif
  
  // �����ر�־λ
  __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND | SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  err += WiFi_LowLevel_CheckError(__FUNCTION__);
  if (err != 0)
    return -1;
  return 0;
}

/* ��SDIO�Ĵ��� */
uint8_t WiFi_LowLevel_ReadReg(uint8_t func, uint32_t addr)
{
  WiFi_LowLevel_SendCMD52(func, addr, 0, 0);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  return SDIO_GetResponse(SDIO, SDIO_RESP1) & 0xff;
}

/* ��ʼ��SDIO���貢���WiFiģ���ö�� */
// SDIO Simplified Specification Version 3.00: 3. SDIO Card Initialization
static void WiFi_LowLevel_SDIOInit(void)
{
  SDIO_InitTypeDef sdio = {0};
  uint32_t freq, resp;
  
  // ��F4��Ƭ����, ����Ҫ����PLL����ʹ��SDIO����
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET)
  {
    __HAL_RCC_PLL_ENABLE();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET);
    printf("PLL is enabled!\n");
  }
  
  // SDIO����ӵ������ʱ��: SDIOCLK=48MHz (��Ƶ�����ڲ���SDIO_CK=PC12����ʱ��), APB2 bus clock=PCLK2=84MHz (���ڷ��ʼĴ���)
  __HAL_RCC_SDIO_CLK_ENABLE();
#if WIFI_USEDMA
  __HAL_RCC_DMA2_CLK_ENABLE();
#endif
  
  SDIO_PowerState_ON(SDIO); // ��SDIO����
  sdio.ClockDiv = WiFi_LowLevel_CalcClockDivider(400000, &freq); // ��ʼ��ʱ��������Ƶ��: 400kHz
  SDIO_Init(SDIO, sdio);
  __SDIO_ENABLE(SDIO);
  
  __SDIO_OPERATION_ENABLE(SDIO); // ��ΪSDIOģʽ
#if WIFI_USEDMA
  WiFi_LowLevel_Delay(1); // ��F4�ϴ�SDIO DMAǰ������ʱ, �����ʧ��
  __SDIO_DMA_ENABLE(SDIO); // ��ΪDMA����ģʽ
#endif
  
  // ����Ҫ����CMD0, ��ΪSD I/O card�ĳ�ʼ��������CMD52
  // An I/O only card or the I/O portion of a combo card is NOT reset by CMD0. (See 4.4 Reset for SDIO)
  WiFi_LowLevel_Delay(10); // ��ʱ�ɷ�ֹCMD5�ط�
  
  /* ����CMD5: IO_SEND_OP_COND */
  sdio_cmd.Argument = 0;
  sdio_cmd.CmdIndex = 5;
  sdio_cmd.CPSM = SDIO_CPSM_ENABLE;
  sdio_cmd.Response = SDIO_RESPONSE_SHORT; // ���ն̻�Ӧ
  sdio_cmd.WaitForInterrupt = SDIO_WAIT_NO;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  printf("RESPCMD%d, RESP1_%08x\n", SDIO_GetCommandResponse(SDIO), SDIO_GetResponse(SDIO, SDIO_RESP1));
  
  /* ���ò���VDD Voltage Window: 3.2~3.4V, ���ٴη���CMD5 */
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
  
  /* ��ȡWiFiģ���ַ (CMD3: SEND_RELATIVE_ADDR, Ask the card to publish a new relative address (RCA)) */
  sdio_cmd.Argument = 0;
  sdio_cmd.CmdIndex = 3;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  sdio_rca = SDIO_GetResponse(SDIO, SDIO_RESP1) >> 16;
  printf("Relative Card Address: 0x%04x\n", sdio_rca);
  
  /* ѡ��WiFiģ�� (CMD7: SELECT/DESELECT_CARD) */
  sdio_cmd.Argument = sdio_rca << 16;
  sdio_cmd.CmdIndex = 7;
  SDIO_SendCommand(SDIO, &sdio_cmd);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  printf("Card selected! RESP1_%08x\n", SDIO_GetResponse(SDIO, SDIO_RESP1));
  
  /* ���ʱ��Ƶ��, ���������ݳ�ʱʱ��Ϊ0.1s */
  sdio.ClockDiv = WiFi_LowLevel_CalcClockDivider(WIFI_CLOCK_FREQ, &freq);
  sdio_data.DataTimeOut = freq / 10;
  
  /* SDIO��������߿����Ϊ4λ */
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
  // ��count=512ʱ, ��0x1ff�����Ϊ0, ����SDIO��׼
  sdio_cmd.Argument = (func << 28) | (addr << 9) | (count & 0x1ff) | flags;
  sdio_cmd.CmdIndex = 53;
  sdio_cmd.CPSM = SDIO_CPSM_ENABLE;
  sdio_cmd.Response = SDIO_RESPONSE_SHORT;
  sdio_cmd.WaitForInterrupt = SDIO_WAIT_NO;
  SDIO_SendCommand(SDIO, &sdio_cmd);
}

/* ����WiFiģ�鹦���������ݿ��С */
int WiFi_LowLevel_SetBlockSize(uint8_t func, uint32_t size)
{
  if (WiFi_LowLevel_SetSDIOBlockSize(size) == -1)
    return -1;
  
  sdio_block_size[func] = size;
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x10, size & 0xff);
  WiFi_LowLevel_WriteReg(0, (func << 8) | 0x11, size >> 8);
  return 0;
}

/* ����SDIO��������ݿ��С */
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

/* ���Flash�б���Ĺ̼������Ƿ����� */
#ifdef WIFI_FIRMWAREAREA_ADDR
static int WiFi_LowLevel_VerifyFirmware(void)
{
  uint32_t crc, len;
  
  if (WIFI_FIRMWARE_SIZE != 255536)
    return 0; // ����ɹ�, ��������Ϊ�̼�������, ���Բ�����-1, ����0
  
  len = WIFI_FIRMWARE_SIZE / 4 + 2; // �̼���(����CRC)�ܴ�С��1/4
  crc = HAL_CRC_Calculate(&hcrc, (uint32_t *)WIFI_FIRMWAREAREA_ADDR, len);
  return crc == 0; // ����1Ϊ����, 0Ϊ������
}
#endif

/* �ȴ�SDIO�����Ӧ */
static void WiFi_LowLevel_WaitForResponse(const char *msg_title)
{
  uint8_t i = 0;
  
  do
  {
    if (i == WIFI_LOWLEVEL_MAXRETRY)
      abort();
    
    if (i != 0)
      SDIO_SendCommand(SDIO, &sdio_cmd); // �ط�����
    i++;
    
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT) != RESET); // �ȴ���������
    WiFi_LowLevel_CheckError(msg_title);
  } while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND) == RESET); // ���û���յ���Ӧ, ������
  __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
}

/* ��������, �Զ��жϲ������ִ���ģʽ */
// sizeΪҪ���͵��ֽ���, bufsizeΪdata�������Ĵ�С, bufsize=0ʱ���û��������
int WiFi_LowLevel_WriteData(uint8_t func, uint32_t addr, const void *data, uint32_t size, uint32_t bufsize, uint32_t flags)
{
  int i, err = 0;
  uint16_t block_num; // ���ݿ����
  uint32_t cmd53_flags = CMD53_WRITE;
#if WIFI_USEDMA
  LL_DMA_InitTypeDef dma;
#else
  const uint32_t *p = data;
#endif
  
  if ((uintptr_t)data & 3)
  {
    printf("%s: data must be 4-byte aligned!\n", __FUNCTION__);
    return -2; // ������
  }
  if (size == 0)
  {
    printf("%s: size cannot be 0!\n", __FUNCTION__);
    return -2;
  }

  block_num = WiFi_LowLevel_GetBlockNum(func, &size, flags);
  if (bufsize != 0 && bufsize < size) // ֻ��������Խ�粻��Ӱ�����ݴ���, ������ֻ��һ������
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
  WiFi_LowLevel_WaitForResponse(__FUNCTION__); // ����Ҫ�ȵ�CMD53�յ���Ӧ����ܿ�ʼ��������
  
  // ��ʼ��������
#if WIFI_USEDMA
  dma.Channel = LL_DMA_CHANNEL_4;
  dma.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
  dma.FIFOMode = LL_DMA_FIFOMODE_ENABLE; // ����ʹ��FIFOģʽ
  dma.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
  dma.MemBurst = LL_DMA_MBURST_INC4;
  dma.MemoryOrM2MDstAddress = (uint32_t)data;
  dma.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
  dma.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
  dma.Mode = LL_DMA_MODE_PFCTRL; // ������SDIO����DMA�����������
  dma.NbData = 0; // ��SDIO��������ʱ, ��ѡ����Ч (HTIF3Ҳ������λ)
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
    SDIO_WriteFIFO(SDIO, (uint32_t *)p); // ��FIFO����4�ֽ�����
    p++;
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXFIFOF) != RESET); // ���FIFO������ȴ�
    
    // ������ִ���, ���˳�ѭ��
    err += WiFi_LowLevel_CheckError(__FUNCTION__);
    if (err)
      break;
  }
#endif
  
  // �ȴ��������
  i = 0;
  while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND) == RESET)
  {
    err += WiFi_LowLevel_CheckError(__FUNCTION__);
    if (err)
      break;
    
    i++;
    if (i == CMD53_TIMEOUT)
    {
      printf("%s: timeout!\n", __FUNCTION__); // ��������TXACTʼ�ղ�����, Ҳû�д����־λ��λ�����
      err++;
      break;
    }
  }
  sdio_data.DPSM = SDIO_DPSM_DISABLE;
  SDIO_ConfigData(SDIO, &sdio_data);
#if WIFI_USEDMA
  LL_DMA_ClearFlag_TC3(DMA2); // ���DMA������ɱ�־λ
  LL_DMA_ClearFlag_FE3(DMA2); // ����FIFO error����
  LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_3); // �ر�DMA
#endif
  
  // �����ر�־λ
  __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
  err += WiFi_LowLevel_CheckError(__FUNCTION__);
  if (err != 0)
    return -1;
  return 0;
}

/* д�Ĵ���, ����д���Ĵ�����ʵ������ */
uint8_t WiFi_LowLevel_WriteReg(uint8_t func, uint32_t addr, uint8_t value)
{
  WiFi_LowLevel_SendCMD52(func, addr, value, CMD52_WRITE | CMD52_READAFTERWRITE);
  WiFi_LowLevel_WaitForResponse(__FUNCTION__);
  return SDIO_GetResponse(SDIO, SDIO_RESP1) & 0xff;
}
