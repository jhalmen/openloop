
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


static void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	/* enable clock to GPIOC */
	RCC_AHB1ENR |= RCC_AHB1ENR_IOPCEN;
	/* set to alternate function mode */
	GPIOC_MODER |= (1 << 19);
	/* set ouput divider to 4 */
	RCC_CFGR |= (RCC_CFGR_MCOPRE_DIV_4) << RCC_CFGR_MCO2PRE_SHIFT;
	/* set pc9 to mco */
	GPIOC_AFRH |= (0 << 4);
}

/* static void systick_setup(void) */
/* { */
/* 	/1* systick_set_frequency (desired_systick_freq, current_freq_ahb) *1/ */
/* 	/1* returns false if frequency can't be set *1/ */
/* 	if (!systick_set_frequency(1,84000000)) */
/* 		while (1); */
/* 	systick_counter_enable(); */
/* 	systick_interrupt_enable(); */
/* } */

/* void sys_tick_handler(void) */
/* { */
/* 	gpio_toggle(GPIOA, GPIO12); */
/* } */

int main(void)
{
	int i;
	gpio_setup();
	while (1) {
			gpio_toggle(GPIOA, GPIO12);
			/* F_CPU = 16MHz */
			/* blink with frequency of 2 Hz */
			/* -> 8 * 10^6 wait instr */
			/* nop -> 1 instr */
			/* for -> 1 addition, 1 branch -> 2 instr */
			/* -> 8 / 3 * 10^6 repeats */
			/* for (i = 0; i < 500000; ++i) */
			for (i = 0; i < 2666666; ++i)
				__asm__("nop");
	}

	return 0;
}
