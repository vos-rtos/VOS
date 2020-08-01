#include "cmsis/stm32f407xx.h"
#include "../vos/vtype.h"
#include "../vos/vos.h"

void VOSSemInit();

extern unsigned int __data_rw_array_start;
extern unsigned int __data_rw_array_end;
extern unsigned int __bss_zero_array_start;
extern unsigned int __bss_zero_array_end;
extern unsigned int __vectors_start;
void init_data_and_bss ()
{
    unsigned int *to_begin = 0;
    unsigned int *to_end = 0;
    unsigned int *from = 0;
    unsigned int *p = 0;
    // Copy the data sections from flash to SRAM.
	for (p = &__data_rw_array_start; p < &__data_rw_array_end;) {
		from = (unsigned int *) (*p++);
		to_begin = (unsigned int *) (*p++);
		to_end = (unsigned int *) (*p++);
		while (to_begin < to_end) *to_begin++ = *from++;
	}

	// Zero fill all bss segments
	for (p = &__bss_zero_array_start; p < &__bss_zero_array_end;) {
		to_begin = (unsigned int*) (*p++);
		to_end = (unsigned int*) (*p++);
		while (to_begin < to_end) *to_begin++ = 0;
	}
}
#define VECT_TAB_OFFSET 0
void SysInit(void)
{
  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

#if defined (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM)
  SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

void hardware_early(void)
{
  // Call the CSMSIS system initialisation routine.
	SysInit();

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  // Set VTOR to the actual address, provided by the linker script.
  // Override the manual, possibly wrong, SystemInit() setting.
  SCB->VTOR = (uint32_t)(&__vectors_start);
#endif

}

void
__attribute__((weak))
_exit(int code __attribute__((unused)))
{
  // TODO: write on trace
  while (1)
    ;
}

extern void local_irq_disable(void);

extern void local_irq_enable(void);
// ----------------------------------------------------------------------------

void
__attribute__((weak,noreturn))
abort(void)
{
  //trace_puts("abort(), exiting...");

  _exit(1);
}
void hw_init_clock(void);
u32 VOSTaskInit();

void __attribute__ ((section(".after_vectors")))
___start (void)
{

	int argc;
	static char name[] = "";
	static char* argv[2] = { "vos.bin", 0 };
	int code = 0;

	local_irq_disable();

	hardware_early();
	init_data_and_bss ();


	hw_init_clock();

	VOSSemInit();
	VOSTaskInit();

	local_irq_enable();

	argc = 1;
	*argv = &argv[0];
	//__run_init_array ();
	code = main (argc, argv);
	//__run_fini_array ();
	_exit (code);
	while(1) ;
}



StVosTask *VOSTaskCreate(void (*task_fun)(void *param), void *param, void *pstack, u32 stack_size, u32 prio, char *task_nm);


int aaa = 101;
int bbb = 102;
int ccc = 0;
int test();

u32 VOSTaskDelay(u32 us);

StVOSSemaphore *sem_hdl = 0;
void task0(void *param)
{
	sem_hdl = VOSSemCreate(1, 1, "sem_hdl");
	while (1) {
		//VOSTaskDelay(3000);
		//os_switch_next();
		if (sem_hdl) {
			VOSSemWait(sem_hdl, 10*1000*1000);
			VOSTaskDelay(10*1000*1000);
		}
	}
}
void task1(void *param)
{
	while(1) {
		//VOSTaskDelay(3000);
		if (sem_hdl) {
			VOSSemWait(sem_hdl, 10*1000*1000);
			VOSTaskDelay(10*1000*1000);
		}
	}
}
void task2(void *param)
{
	while(1) {
		//VOSTaskDelay(5000);
		if (sem_hdl) {
			VOSSemRelease(sem_hdl);
		}
	}
}

long long task0_stack[1024], task1_stack[1024], task2_stack[1024];
void VOSStarup();
int main(int argc, char* argv[])
{
//	VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), 100);
//	VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), 200);
//	VOSTaskCreate(task2, 0, task2_stack, sizeof(task2_stack), 300);
	VOSTaskCreate(task0, 0, task0_stack, sizeof(task0_stack), 100, "task0");
	VOSTaskCreate(task1, 0, task1_stack, sizeof(task1_stack), 200, "task1");
	VOSTaskCreate(task2, 0, task2_stack, sizeof(task2_stack), 300, "task2");
	ccc = aaa+bbb;
	test();
	return 0;
}
