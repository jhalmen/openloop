
/* #include <stdio.h> */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/spi.h>
/* ./libopencm3/include/libopencm3/stm32/common/spi_common_all.h */


/* ./libopencm3/include/libopencm3/cm3/nvic.h */
/* ./libopencm3/include/libopencm3/cm3/systick.h */


void gpio_setup(void);
void gpio_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	/* led */
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	/* i2s2 pins */
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF,
			GPIO_PUPD_NONE, GPIO6);
	gpio_set_af(GPIOC, GPIO_AF5, GPIO6);
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF,
			GPIO_PUPD_NONE, GPIO15 | GPIO13 | GPIO12);
	gpio_set_af(GPIOB, GPIO_AF5, GPIO15 | GPIO13 | GPIO12);
	/* mco */
	/* enable clock to GPIOC */
	RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
	/* set mco ouput divider to 4 */
	RCC_CFGR |= (RCC_CFGR_MCOPRE_DIV_4) << RCC_CFGR_MCO2PRE_SHIFT;
	/* set to alternate function mode */
	GPIOC_MODER |= GPIO_MODE(9, GPIO_MODE_AF);
	/* set pc9 to mco */
	gpio_set_af(GPIOC, GPIO_AF0, GPIO9);
}

void pll_setup(void);
void pll_setup(void)
{
	/* set main pll 84MHz */
	uint8_t pllm = 8;
	uint16_t plln = 336;
	uint8_t pllp = 8;
	uint8_t pllq = 14;
	uint8_t pllr = 0;
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
	rcc_system_clock_source();
}

void plli2s_setup(uint16_t n, uint8_t r);
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

static void systick_setup(uint32_t tick_frequency)
{
	/* systick_set_frequency (desired_systick_freq, current_freq_ahb) */
	/* returns false if frequency can't be set */
	if (!systick_set_frequency(tick_frequency,rcc_ahb_frequency))
		while (1);
	systick_counter_enable();
	systick_interrupt_enable();
}

uint32_t tick = 0;

void sys_tick_handler(void)
{
	++tick;
#ifdef DEBUG
	printf("tick: %ld\n", tick);
#endif
}
void	i2s_init_master(uint32_t spi, uint8_t div, uint8_t odd, uint8_t mckoe);
void	i2s_init_master(uint32_t spi, uint8_t div, uint8_t odd, uint8_t mckoe)
{
	/* set prescaler register */
	uint16_t pre = (((mckoe & 0x1) << 9) |
		((odd & 0x1) << 8) | div);
	SPI_I2SPR(spi) = pre;
	/* set configuration register */
	uint32_t cfg = SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE |
			SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT; /*|
			SPI_I2SCFGR_CHLEN; */ /*only if chlen=32bit*/
	/* finally enable the peripheral */
	SPI_I2SCFGR(spi) |= cfg;
	SPI_I2SCFGR(spi) |= SPI_I2SCFGR_I2SE;
}

void i2s_write(uint32_t spi, uint16_t data);
void i2s_write(uint32_t spi, uint16_t data)
{
	SPI_DR(spi) = data;
}


uint32_t i2s_read(uint32_t spi);
uint32_t i2s_read(uint32_t spi)
{
	if (SPI_I2SCFGR(spi) & SPI_I2SCFGR_DATLEN_32BIT) {
		uint16_t dat = SPI_DR(spi);
		return dat << 16 | SPI_DR(spi);
	}
	return SPI_DR(spi);
}

uint8_t chkside(uint32_t spi);
uint8_t chkside(uint32_t spi)
{
	return SPI_SR(spi) & (1 << 2);
}

volatile int16_t datal = 0, datar = 0;
int main(void)
{
	/* void initialise_monitor_handles(void); */
	/* initialise_monitor_handles(); */
	pll_setup();
	gpio_setup();
	systick_setup(100000);
	/* take care of i2s configuration */
	/* rcc_clock_setup_hsi(&rcc_hsi_16mhz); */
	/* /1* n=258; *1/ */
	/* /1* r=3; *1/ */
	/* plli2s_setup(258,3); */
	plli2s_setup(256,5);
	/* /1* i2sdiv=3; *1/ */
	/* /1* i2sodd=1; *1/ */
	/* int div = 3; */
	int div = 12;
	int odd = 1;
	int mckoe = 1;
	/* enable SPI2/I2S2 peripheral clock */
	rcc_periph_clock_enable(RCC_SPI2);
	i2s_init_master(SPI2, div, odd, mckoe);

	uint32_t spi2 = SPI2;
	spi2--; spi2++;
	int16_t data[1024][2];
	for (int i = 0; i < 1024; ++i){
		data[i][0] = 0;
		data[i][1] = 0;
	}
	while (1) {
		int i = 0;
		while (SPI_SR(SPI2) & SPI_SR_RXNE) {
			if (i > 1023)
				break;
			if (chkside(SPI2)){
				data[i][1] = 1;
				gpio_set(GPIOA,GPIO12);
			} else {
				data[i][1] = 0;
				gpio_clear(GPIOA, GPIO12);
			}
			data[i][0] = i2s_read(SPI2);
			++i;
		}
		/* i2s_write(SPI2, ++data); */
		int32_t tmpl = 0, tmpr = 0;
		uint16_t il = 0, ir = 0;
		for (int j = 0; j < i; ++j)
			if (data[i][1]) {
				tmpr += data[i][0];
				++ir;
			} else {
				tmpl += data[i][0];
				++il;
			}
		datar = tmpr / ir;
		datal = tmpl / il;


		/* printf("points:%3d%3d\tdata is: %6d\t%6d\n", il, ir, datal, datar); */
			/* __asm__("wfi"); */
			/* __asm__("nop"); */
	}
	return 0;
}
