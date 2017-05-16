
/* #include <stdio.h> */
#include <math.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
/* ./libopencm3/include/libopencm3/stm32/common/spi_common_all.h */


/* ./libopencm3/include/libopencm3/cm3/nvic.h */
/* ./libopencm3/include/libopencm3/cm3/systick.h */

#define M_PI 		(3.14159165)

void gpio_setup(void);
void pll_setup(void);
void plli2s_setup(uint16_t n, uint8_t r);
void i2s_init_master_receive(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe);
void i2s_init_slave_transmit(uint32_t i2s);
void i2s_write(uint32_t i2s, int16_t data);
void systick_setup(uint32_t tick_frequency);
uint32_t i2s_read(uint32_t i2s);
uint8_t chkside(uint32_t i2s);
uint32_t tick = 0;
volatile double datal = 0, datar = 0;
int main(void)
{
	/* void initialise_monitor_handles(void); */
	/* initialise_monitor_handles(); */
	uint32_t i2s2, i2s2ext;
	if((i2s2 = I2S2)) {}
	if((i2s2ext = I2S2ext)) {}

	pll_setup();
	gpio_setup();
	gpio_set(GPIOA, GPIO12);
	/* systick_setup(100000); */
	systick_setup(10);
	/* take care of i2s configuration */
	int div, odd, mckoe;
	/* 48?kHz i think */
	/* plli2s_setup(258,3); */
	/* div = 3; */
	/* odd = 1; */
	/* mckoe = 1; */

	/* TODO temporary this sets the interface to only 2*8kHz! */
	plli2s_setup(256,5);
	div = 12;
	odd = 1;
	mckoe = 1;
	/* enable SPI2/I2S2 peripheral clock */
	rcc_periph_clock_enable(RCC_SPI2);
	/* use interrupts */
	spi_enable_tx_buffer_empty_interrupt(I2S2ext);
	nvic_enable_irq(NVIC_SPI2_IRQ);
	/* slave has to be enabled before the master! */
	i2s_init_slave_transmit(I2S2ext);
	i2s_init_master_receive(I2S2, div, odd, mckoe);

	/* test output without interrupt */
	/* this should make a sawtooth signal */
	/* uint32_t status; */
	/* for (uint16_t j = 0; j < 0xFFFE; ++j) */
	/* for (int16_t i = -0x7FFF+1; i < 0x7FFF; ++i){ */
	/* 	while (!((status = SPI_SR(I2S2ext))&SPI_SR_TXE)){} */
	/* 	if (status&SPI_SR_CHSIDE) */
	/* 		i2s_write(I2S2ext, i); */
	/* 	else i2s_write(I2S2ext,0); */
	/* } */

	/* test output without interrupt */
	/* constantly write 0x3e8 to right channel and 0 to left */
	/* uint32_t status; */
	/* while (1) { */
	/* 	while (!((status = SPI_SR(I2S2ext))&SPI_SR_TXE)){} */
	/* 	if (status&SPI_SR_CHSIDE) */
	/* 	i2s_write(I2S2ext, 0); */
	/* 	else i2s_write(I2S2ext, 1000); */
	/* } */

	/* int i = 0; */
	/* uint32_t status; */
	while (1) {
		/* while (!((status = SPI_SR(I2S2ext))&SPI_SR_TXE)){} */
		/* 	/1* left channel *1/ */
		/* 		/1* i2s_write(I2S2ext, wr++); *1/ */
		/* if (i < 40) */
		/* i2s_write(I2S2ext, 1); */
		/* else i2s_write(I2S2ext, -1); */
		/* while (!((status = SPI_SR(I2S2ext))&SPI_SR_TXE)){} */
		/* 	/1* right channel *1/ */
		/* 		i2s_write(I2S2ext, 0); */
		/* i = (i+1)%80; */
		/* if(status&SPI_SR_UDR) */
		/* 	gpio_toggle(GPIOA,GPIO12); */

			/* __asm__("wfi"); */
			/* __asm__("nop"); */
	}
	return 0;
}

void spi2_isr(void)
{
	if(SPI_SR(I2S2ext) & SPI_SR_CHSIDE)
		i2s_write(I2S2ext, 0x0FAC);
	else i2s_write(I2S2ext, 0xFAB0);
}

void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	/* led */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	/* i2s2 pins */
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_af(GPIOC, GPIO_AF5, GPIO6);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF,
			GPIO_PUPD_NONE, GPIO6);
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_af(GPIOB, GPIO_AF5, GPIO15 | GPIO13 | GPIO12);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF,
			GPIO_PUPD_NONE, GPIO15 | GPIO13 | GPIO12);
	/* i2s2ext pin */
	gpio_set_af(GPIOC, GPIO_AF6, GPIO2);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);

	/* mco */
	/* enable clock to GPIOC */
	RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	/* set mco ouput divider to 4 */
	RCC_CFGR |= (RCC_CFGR_MCOPRE_DIV_4) << RCC_CFGR_MCO2PRE_SHIFT;
	/* set to alternate function mode */
	GPIOC_MODER |= GPIO_MODE(9, GPIO_MODE_AF);
	/* set pc9 to mco */
	/* this is unnecessary as alternate function 0 is the default */
	gpio_set_af(GPIOC, GPIO_AF0, GPIO9);
}

void pll_setup(void)
{
	/* set main pll 84MHz */
	uint8_t pllm = 8;
	uint16_t plln = 336;
	uint8_t pllp = 8;
	uint8_t pllq = 14;
	uint8_t pllr = 0;
	/* set flash waitstates! */
	flash_set_ws(FLASH_ACR_PRFTEN | FLASH_ACR_DCEN |
		FLASH_ACR_ICEN | FLASH_ACR_LATENCY_2WS);
	rcc_set_main_pll_hsi(pllm, plln, pllp, pllq, pllr);
	/* set prescalers for the different domains */
	rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
	rcc_set_ppre1(RCC_CFGR_PPRE_DIV_2);
	rcc_set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
	/* in case this is needed by the library: */
	rcc_ahb_frequency  = 84000000;
	rcc_apb1_frequency = 42000000;
	rcc_apb2_frequency = 84000000;
	/* finally enable pll */
	rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	/* use pll as system clock */
	rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
	rcc_wait_for_sysclk_status(RCC_PLL);
}

void plli2s_setup(uint16_t n, uint8_t r)
{
	/* configure plli2s */
	/* void rcc_plli2s_config(uint16_t n, uint8_t r) */
	RCC_PLLI2SCFGR = (
		((n & RCC_PLLI2SCFGR_PLLI2SN_MASK) << RCC_PLLI2SCFGR_PLLI2SN_SHIFT) |
		((r & RCC_PLLI2SCFGR_PLLI2SR_MASK) << RCC_PLLI2SCFGR_PLLI2SR_SHIFT));
	/* enable pll */
	rcc_osc_on(RCC_PLLI2S);
	rcc_wait_for_osc_ready(RCC_PLLI2S);
}

void systick_setup(uint32_t tick_frequency)
{
	/* systick_set_frequency (desired_systick_freq, current_freq_ahb) */
	/* returns false if frequency can't be set */
	if (!systick_set_frequency(tick_frequency,rcc_ahb_frequency))
		while (1);
	systick_counter_enable();
	systick_interrupt_enable();
}

void sys_tick_handler(void)
{
	++tick;
}

enum i2smode{
	I2S_SLAVE_TRANSMIT,
	I2S_SLAVE_RECEIVE,
	I2S_MASTER_TRANSMIT,
	I2S_MASTER_RECEIVE
};

void i2s_init_master_receive(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe)
{
	/* set prescaler register */
	uint16_t pre = (((mckoe & 0x1) << 9) |
		((odd & 0x1) << 8) | div);
	SPI_I2SPR(i2s) = pre;
	/* set configuration register */
	uint32_t cfg = SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT |
			SPI_I2SCFGR_CHLEN;  /*only if chlen=32bit*/
	/* finally enable the peripheral */
	SPI_I2SCFGR(i2s) |= cfg;
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SE;
}

void i2s_init_slave_transmit(uint32_t i2s)
{
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SCFG_SLAVE_TRANSMIT |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT |
			SPI_I2SCFGR_CHLEN;  /*only if chlen=32bit*/
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SE;
}

void i2s_write(uint32_t i2s, int16_t data)
{
	SPI_DR(i2s) = data;
}

uint32_t i2s_read(uint32_t i2s)
{
	if (SPI_I2SCFGR(i2s) & SPI_I2SCFGR_DATLEN_32BIT) {
		uint16_t dat = SPI_DR(i2s);
		return dat << 16 | SPI_DR(i2s);
	}
	return SPI_DR(i2s);
}

uint8_t chkside(uint32_t i2s)
{
	return SPI_SR(i2s) & BIT2;
}

