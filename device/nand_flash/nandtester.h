#ifndef __NANDTESTER_H
#define __NANDTESTER_H
#include "vos.h"
#include "common.h"

u8 test_writepage(u32 pagenum,u16 colnum,u16 writebytes);
u8 test_readpage(u32 pagenum,u16 colnum,u16 readbytes);
u8 test_copypageandwrite(u32 spnum,u32 dpnum,u16 colnum,u16 writebytes);
u8 test_readspare(u32 pagenum,u16 colnum,u16 readbytes);
void test_readallblockinfo(u32 sblock);
u8 test_ftlwritesectors(u32 secx,u16 secsize,u16 seccnt);
u8 test_ftlreadsectors(u32 secx,u16 secsize,u16 seccnt);

#endif
