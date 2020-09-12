/* 配置sections内容 */

__stack = ORIGIN(RAM) + LENGTH(RAM);

_estack = __stack; 	/* STM specific definition */

__Main_Stack_Size = 1024 ;

PROVIDE ( _Main_Stack_Size = __Main_Stack_Size ) ;

__Main_Stack_Limit = __stack  - __Main_Stack_Size ;


PROVIDE ( _Main_Stack_Limit = __Main_Stack_Limit ) ;

_Minimum_Stack_Size = 256 ;

/* RAM: 系统堆1，就是正常的vmalloc使用的堆  */
PROVIDE ( _Heap_Begin = _end_noinit ) ;
PROVIDE ( _Heap_Limit = __stack - __Main_Stack_Size ) ;

/* CCMRAM: 系统堆2，就是正常的vmalloc使用的堆  */
PROVIDE ( _Heap_ccmram_Begin = _end_ccmram_noinit ) ;
PROVIDE ( _Heap_ccmram_Limit = ORIGIN(CCMRAM)+LENGTH(CCMRAM)) ;

/* vos_start入口 */
ENTRY(vos_start)

SECTIONS
{
    .isr_vector : ALIGN(4)
    {
        FILL(0xFF)
        __vectors_start = ABSOLUTE(.) ;
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))
		KEEP(*(.cfmconfig))
        *(.after_vectors .after_vectors.*)
    } >FLASH
    
    sec_shell : ALIGN(4)
    {
        FILL(0xFF)
        vshell_name_start = . ;
        KEEP(*(.vshell_name))
        KEEP(*(.vshell_name.*))
        vshell_name_end = . ; 
        
       	vshell_fun_start = . ;
        KEEP(*(.vshell_fun))
        KEEP(*(.vshell_fun.*))
        vshell_fun_end = . ;
        
        vshell_note_start = . ;
        KEEP(*(.vshell_note))
        KEEP(*(.vshell_note.*))
        vshell_note_end = . ;
       
    } >FLASH

    .inits : ALIGN(4)
    {
        __data_rw_array_start = .;
        
        LONG(LOADADDR(sec_data_ram));
        LONG(ADDR(sec_data_ram));
        LONG(ADDR(sec_data_ram)+SIZEOF(sec_data_ram));
        
        LONG(LOADADDR(sec_data_ccm_ram));
        LONG(ADDR(sec_data_ccm_ram));
        LONG(ADDR(sec_data_ccm_ram)+SIZEOF(sec_data_ccm_ram));
        
        __data_rw_array_end = .;
        
        __bss_zero_array_start = .;
        
        LONG(ADDR(sec_bss));
        LONG(ADDR(sec_bss)+SIZEOF(sec_bss));
        
        LONG(ADDR(sec_bss_ccm_ram));
        LONG(ADDR(sec_bss_ccm_ram)+SIZEOF(sec_bss_ccm_ram));
        
        __bss_zero_array_end = .;

		KEEP(*(.init))
		KEEP(*(.fini))

		. = ALIGN(4);

		PROVIDE_HIDDEN (__preinit_array_start = .);
        
		KEEP(*(.preinit_array_sysinit .preinit_array_sysinit.*))
        
		KEEP(*(.preinit_array_platform .preinit_array_platform.*))
        
		KEEP(*(.preinit_array .preinit_array.*))

		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);

		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);

		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

    } >FLASH

    .flashtext : ALIGN(4)
    {
        *(.flashtext .flashtext.*)	/* Startup code */
    } >FLASH
 
    sec_text : ALIGN(4)
    {
        *(.text .text.*)
 
        *(.rodata .rodata.* .constdata .constdata.*) 		

        *(vtable) /* C++ virtual tables */

		KEEP(*(.eh_frame*))

        *(.glue_7)
        *(.glue_7t)

    } >FLASH

	.ARM.extab : ALIGN(4)
   	{
       *(.ARM.extab* .gnu.linkonce.armextab.*)
   	} > FLASH
   	
    . = ALIGN(4);
   	__exidx_start = .;   	
   	.ARM.exidx : ALIGN(4)
   	{
       *(.ARM.exidx* .gnu.linkonce.armexidx.*)
   	} > FLASH
   	__exidx_end = .;
   	
    . = ALIGN(4);
    _etext = .;
    __etext = .;
    
    sec_data_ccm_ram : ALIGN(4)
    {
       FILL(0xFF)
       *(.data.CCMRAM .data.CCMRAM.*)
       . = ALIGN(4) ;
    } > CCMRAM AT>FLASH

    _sidata = LOADADDR(sec_data_ram);

    sec_data_ram : ALIGN(4)
    {
    	FILL(0xFF)
        _sdata = . ;
        __data_start__ = . ;
		*(.data_begin .data_begin.*)

		*(.data .data.*)
		
		*(.data_end .data_end.*)
	    . = ALIGN(4);

        _edata = . ;
        __data_end__ = . ;

    } >RAM AT>FLASH
    

	sec_bss_ccm_ram (NOLOAD) : ALIGN(4)
	{
		*(.bss.CCMRAM .bss.CCMRAM.*)
	} > CCMRAM

    sec_bss (NOLOAD) : ALIGN(4)
    {
        __bss_start__ = .;
        _sbss = .;
        *(.bss_begin .bss_begin.*)

        *(.bss .bss.*)
        *(COMMON)
        
        *(.bss_end .bss_end.*)
	    . = ALIGN(4);
        __bss_end__ = .; 
        _ebss = . ;
    } >RAM

    .noinit_CCMRAM (NOLOAD) : ALIGN(4)
    {
        *(.noinit.CCMRAM .noinit.CCMRAM.*)
        _end_ccmram_noinit = .;         
    } > CCMRAM
    
    .noinit (NOLOAD) : ALIGN(4)
    {
        _noinit = .;
        
        *(.noinit .noinit.*) 
        
         . = ALIGN(4) ;
        _end_noinit = .;   
    } > RAM
    
    PROVIDE ( end = _end_noinit ); /* was _ebss */
    PROVIDE ( _end = _end_noinit );
    PROVIDE ( __end = _end_noinit );
    PROVIDE ( __end__ = _end_noinit );
    
    ._check_stack : ALIGN(4)
    {
        . = . + _Minimum_Stack_Size ;
    } >RAM
   
   
    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }

    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }

    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }

    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }

    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }

    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }    
}
