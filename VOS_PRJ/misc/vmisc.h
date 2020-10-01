#ifndef __VMISC_H__
#define __VMISC_H__

#define printf kprintf

#define SHOW_REG(x) printf(#x"=0x%08x\r\n", x)

#define ARRAY_CNTS(arr) (sizeof(arr)/sizeof(arr[0]))

/*
  * λͼ�����궨��
 */
#define bitmap_for_each(pos, bitmap_byte) \
	for (pos = 0; pos < bitmap_byte*8; pos++)

#define bitmap_get(n, bitmap)		(!!(((u8*)(bitmap))[(n)>>3] & 1<<((n)&0x07)))
#define bitmap_clr(n, bitmap)		(((u8*)(bitmap))[(n)>>3] &= ~(1<<((n)&0x07)))
#define bitmap_set(n, bitmap)		(((u8*)(bitmap))[(n)>>3] |= 1<<((n)&0x07))


#endif
