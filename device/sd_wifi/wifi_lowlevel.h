#ifndef __WIFI_LOWLEVEL_H
#define __WIFI_LOWLEVEL_H

#define WIFI_RWDATA_ADDRINCREMENT _BV(0)
#define WIFI_RWDATA_ALLOWMULTIBYTE _BV(1)

/* WiFiÄ£¿éµ×²ãº¯Êý */
void WiFi_LowLevel_Delay(int nms);
void WiFi_LowLevel_Dump(const void *data, int len);
uint8_t WiFi_LowLevel_GetFunctionNum(void);
int WiFi_LowLevel_GetITStatus(uint8_t clear);
uint32_t WiFi_LowLevel_GetTicks(void);
void WiFi_LowLevel_Init(void);
int WiFi_LowLevel_ReadData(uint8_t func, uint32_t addr, void *data, uint32_t size, uint32_t bufsize, uint32_t flags);
uint8_t WiFi_LowLevel_ReadReg(uint8_t func, uint32_t addr);
int WiFi_LowLevel_SetBlockSize(uint8_t func, uint32_t size);
int WiFi_LowLevel_WriteData(uint8_t func, uint32_t addr, const void *data, uint32_t size, uint32_t bufsize, uint32_t flags);
uint8_t WiFi_LowLevel_WriteReg(uint8_t func, uint32_t addr, uint8_t value);

#endif
