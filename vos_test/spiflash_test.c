
#include "spi.h"
#include "w25qxx.h"

void spiflash_test()
{
	s32 i = 0;
	s32 j = 0;
	u8 buf[4*1024];//4k每个扇区
	u16 chip_id;
	W25QXX_Init();
	chip_id = W25QXX_ReadID();
	kprintf("W25QXX_ReadID, chip_id=0x%04x!\r\n", chip_id);
	W25QXX_Erase_Chip();
	memset(buf, 0x00, sizeof(buf));
	W25QXX_Read(buf, 0, sizeof(buf));
	memset(buf, 0x35, sizeof(buf));
	W25QXX_Write(buf, 0, sizeof(buf));
	memset(buf, 0x00, sizeof(buf));
	W25QXX_Read(buf, 0, sizeof(buf));
	//写flash满数据
	W25QXX_Erase_Chip();
	for (i=0; i<2*1024*1024; )
	{
		memset(buf, (u8)(i/sizeof(buf)), sizeof(buf));
		W25QXX_Write(buf, i, sizeof(buf));
		i += sizeof(buf);
	}
	//读并验证数据是否准确
	for (i=0; i<2*1024*1024; )
	{
		memset(buf, 0, sizeof(buf));
		W25QXX_Read(buf, i, sizeof(buf));
		for (j=0; j<sizeof(buf); j++) {
			if (buf[j] != (u8)(i/sizeof(buf))) {
				kprintf("ERROR: Read Validate Failed!\r\n");
				break;
			}
		}
		i += sizeof(buf);
	}
}



