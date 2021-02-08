#include "ff_gen_drv.h"
#include "ftl.h"
#include "nand_diskio.h"
#include "nand.h"

DSTATUS nand_initialize (BYTE);
DSTATUS nand_status (BYTE);
DRESULT nand_read (BYTE, BYTE*, DWORD, UINT);
DRESULT nand_write (BYTE, const BYTE*, DWORD, UINT);
DRESULT nand_ioctl (BYTE, BYTE, void*);

const Diskio_drvTypeDef  NAND_Driver =
{
  nand_initialize,
  nand_status,
  nand_read,
  nand_write,
  nand_ioctl,
};

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS nand_initialize(BYTE lun)
{
  if (FTL_Init()==0) {
	  return RES_OK;
  }
  return STA_NOINIT;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS nand_status(BYTE lun)
{
  return RES_OK;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT nand_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
	DRESULT res = RES_ERROR;
	res=FTL_ReadSectors(buff,sector,512,count);

  return res;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT nand_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;
  res = FTL_WriteSectors((u8*)buff,sector,512,count);
  return res;
}

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
DRESULT nand_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;

  switch(cmd)
  {
	    case CTRL_SYNC:
			res = RES_OK;
	        break;
	    case GET_SECTOR_SIZE:
	        *(WORD*)buff = 512;
	        res = RES_OK;
	        break;
	    case GET_BLOCK_SIZE:
	        *(WORD*)buff = nand_dev.page_mainsize/512;
	        break;
	    case GET_SECTOR_COUNT:
	        *(DWORD*)buff = nand_dev.valid_blocknum*nand_dev.block_pagenum*nand_dev.page_mainsize/512;
	        res = RES_OK;
	        break;
	    default:
	        res = RES_PARERR;
	        break;
  }
  return res;
}
