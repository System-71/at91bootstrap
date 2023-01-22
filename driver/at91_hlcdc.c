#include "hlcdc.h"
#include "common.h"
#include "hardware.h"
#include "gpio.h"
#include "debug.h"
#include "div.h"
#include "board.h"
#include "pmc.h"
#include "timer.h"

#define BASE_LCDC 0xf8038000

static inline unsigned int hlcdc_readl(unsigned int reg)
{
        return readl(BASE_LCDC + reg);
}

static inline void hlcdc_writel(unsigned int reg, unsigned int value)
{
        writel(value, BASE_LCDC + reg);
}

static inline int hlcdc_readl_with_timeout(unsigned int reg, unsigned int mask)
{
	int count = 0;
	int ret = 0;
	volatile unsigned int val;

	do {
		val = hlcdc_readl(reg);
		wait_interval_timer(10);
                count+=1;
                if (count >= 10) {
                        dbg_info("Timeout waiting for %d to set, val=%x\n", reg, val);
			ret = 1;
			dbg_info("val = %#x\n", hlcdc_readl(reg));
			break;
                }
        } while (!(val & mask));
	
	dbg_info("val = %#x\n", hlcdc_readl(reg));

	return ret;
}

static inline int hlcdc_readl_with_timeout_clear(unsigned int reg, unsigned int mask)
{
	int count = 0;
	int ret = 0;

	do {
                wait_interval_timer(10);
                count++;
                if (count >= 10) {
                        dbg_info("Timeout waiting for %d to clear\n", reg);
			ret = 1;
                        break;
                }
        } while (!(~(hlcdc_readl(reg)) & (mask)));
	
	return ret;
}

int hlcdc_init(struct video_buf* vid){

	int ret = 0;
	unsigned int val;

	/* to be safe, run powerdown flow on init */
	val = hlcdc_readl(LCDC_LCDDIS);
	
	dbg_info("LCDC_LCDDIS = %d\n", val);
	
	hlcdc_writel(LCDC_LCDDIS, val | LCDC_LCDDIS_DISPDIS);

	ret =hlcdc_readl_with_timeout_clear(LCDC_LCDSR, LCDC_LCDSR_DISP_ACTIVE);

	val = hlcdc_readl(LCDC_LCDDIS);

	dbg_info("LCDC_LCDDIS = %d\n", val);
	if (ret){
		dbg_info("Error setting LCDC_LCDDIS\n");
	}

	/* Disable synchronization */
	val = hlcdc_readl(LCDC_LCDDIS);
	hlcdc_writel(LCDC_LCDDIS, val | LCDC_LCDDIS_SYNCDIS);

	ret = hlcdc_readl_with_timeout_clear(LCDC_LCDSR, LCDC_LCDSR_SYNC_ACTIVE);

	if (ret){
		dbg_info("Error setting LCDC_LCDSYNCDIS\n");
	}

	/* Disable Pixel clk */
	val = hlcdc_readl(LCDC_LCDDIS);
	hlcdc_writel(LCDC_LCDDIS, val | LCDC_LCDDIS_PCLKDIS);

	ret = hlcdc_readl_with_timeout_clear(LCDC_LCDSR, LCDC_LCDSR_PCLK_ACTIVE);

	if (ret){
		dbg_info("Error clearing LCDC_LCDSR_PCLK_ACTIVE\n");
	}

	/* Disable PWM */
	val = hlcdc_readl(LCDC_LCDDIS);
	hlcdc_writel(LCDC_LCDDIS, val | LCDC_LCDDIS_PWMDIS);

	ret = hlcdc_readl_with_timeout_clear(LCDC_LCDSR, LCDC_LCDSR_PWM_ACTIVE);

	if (ret){
		dbg_info("Error clearing LCDC_LCDSR_PWM_ACTIVE\n");
	}

	/* Setup Pixel Clock divider */
	unsigned int ck_div = 9-2; // 100MHz GCK / (CLK_DIV + 2) = 100/(7+2) = 11.11MHz 

        val = hlcdc_readl(LCDC_LCDCFG0);
        hlcdc_writel(LCDC_LCDCFG0, val | ((ck_div << LCDC_LCDCFG0_CLKDIV_SHIFT) & LCDC_LCDCFG0_CLKDIV_MASK));

        val = hlcdc_readl(LCDC_LCDCFG0);
        hlcdc_writel(LCDC_LCDCFG0, val | LCDC_LCDCFG0_CGDISHEO | LCDC_LCDCFG0_CGDISOVR1 | LCDC_LCDCFG0_CGDISBASE ); 
        val = hlcdc_readl(LCDC_LCDCFG0);
	dbg_info("cfg0 = %x\n", val);

	/* Setup Control reg 5 */

	// set mode to 0x1 16BPP
        val = hlcdc_readl(LCDC_LCDCFG5);
        hlcdc_writel(LCDC_LCDCFG5, val | LCDC_LCDCFG5_VSPOL | LCDC_LCDCFG5_HSPOL | ((0x1 << LCDC_LCDCFG5_MODE_SHIFT) & LCDC_LCDCFG5_MODE_MASK)); // active LOW VSYNC & HSYNC


        val = hlcdc_readl(LCDC_LCDCFG5);
	dbg_info("cfg5 = %x\n", val);

	/* Setup Control reg 1 */

	// set VSYNC pulse-width to 20 lines
	// set HSYNC pulse-width to 4 PCLK cycles
        val = hlcdc_readl(LCDC_LCDCFG1);
        hlcdc_writel(LCDC_LCDCFG1, val | (((20-1) << LCDC_LCDCFG1_VSYNCPW_SHIFT) & LCDC_LCDCFG1_VSYNCPW_MASK) 
			| ((4-1) & LCDC_LCDCFG1_HSYNCPW_MASK));

        val = hlcdc_readl(LCDC_LCDCFG1);
	dbg_info("cfg1 = %x\n", val);

	/* Setup Control reg 2 */

	// set Vertical back porch width to 2 lines
	// set Vertical front porch width to 4 lines
        val = hlcdc_readl(LCDC_LCDCFG2);
        hlcdc_writel(LCDC_LCDCFG2, val | (((11-1) << LCDC_LCDCFG2_VBPW_SHIFT) & LCDC_LCDCFG2_VBPW_MASK)
			| ((10-1) & LCDC_LCDCFG2_VFBPW_MASK));

        val = hlcdc_readl(LCDC_LCDCFG2);
	dbg_info("cfg1 = %x\n", val);

	/* Setup Control reg 3 */

	// set Horizontal back porch width to 20 PCLK cycles
	// set Horizontal front porch width to 10 PCLK cycles
        val = hlcdc_readl(LCDC_LCDCFG3);
        hlcdc_writel(LCDC_LCDCFG3, val | (((2-1) << LCDC_LCDCFG3_HBPW_SHIFT) & LCDC_LCDCFG3_HBPW_MASK)
			| ((2-1) & LCDC_LCDCFG3_HFPW_MASK));

        val = hlcdc_readl(LCDC_LCDCFG3);
	dbg_info("cfg3 = %x\n", val);

	/* Setup Control reg 4 */

	// 400V x 240H size
        val = hlcdc_readl(LCDC_LCDCFG4);
        hlcdc_writel(LCDC_LCDCFG4, val | (((400-1) << LCDC_LCDCFG4_RPF_SHIFT) & LCDC_LCDCFG4_RPF_MASK)
			| ((240-1) & LCDC_LCDCFG4_PPL_MASK));

        val = hlcdc_readl(LCDC_LCDCFG4);
	dbg_info("cfg4 = %x\n", val);

	// setup bus access
	//
	//val = hlcdc_readl(0x8C); //basecfg0
	//hlcdc_writel(0x8C, val | (1 << 8) | (1 << 4));

	hlcdc_writel(0x90, (3 << 4)); // baseceg1, 18BPP RGB666

	hlcdc_writel(0x9C, (1 << 8)); // baseceg4, use DMA channel

	// need DMA shit here !
	//
	//
	
	struct hlcdc_dma_desc dma_desc;

	dma_desc.address = vid->base;
	dma_desc.control = LCDC_LCDC_BASECTRL_DFETCH_EN | LCDC_LCDC_BASECTRL_DONEIEN_EN;

	dma_desc.next = &dma_desc; // point to itself

	hlcdc_writel(LCDC_LCDC_BASEADDR, dma_desc.address);
	hlcdc_writel(LCDC_LCDC_BASECTRL, dma_desc.control);
	hlcdc_writel(LCDC_LCDC_NEXTADDR, dma_desc.next);

	hlcdc_writel(LCDC_LCDC_BASECHEN, LCDC_LCDC_BASECHEN_CHEN_EN | LCDC_LCDC_BASECHEN_UPDATEN_EN);
	
	/* Enable LCD */

	val = hlcdc_readl(LCDC_LCDEN);
	hlcdc_writel(LCDC_LCDEN, val | LCDC_LCDEN_PCLKEN);
	ret = hlcdc_readl_with_timeout(LCDC_LCDSR, LCDC_LCDSR_PCLK_ACTIVE);

	if (ret){
		dbg_info("Error setting LCDC_LCDPCLKEN\n");
	}

        val = hlcdc_readl(LCDC_LCDSR);
	dbg_info("LCDC_LCDSR = %x\n", val);

	val = hlcdc_readl(LCDC_LCDEN);
	hlcdc_writel(LCDC_LCDEN, val | LCDC_LCDEN_SYNCEN);

	ret = hlcdc_readl_with_timeout(LCDC_LCDSR, LCDC_LCDSR_SYNC_ACTIVE);

	if (ret){
		dbg_info("Error setting LCD_SYNCEN\n");
	}

	val = hlcdc_readl(LCDC_LCDEN);
	hlcdc_writel(LCDC_LCDEN, val | LCDC_LCDEN_DISPEN);

	ret = hlcdc_readl_with_timeout(LCDC_LCDSR, LCDC_LCDSR_DISP_ACTIVE);

	if (ret){
		dbg_info("Error setting LCD_DISPEN\n");
	}

	val = hlcdc_readl(LCDC_LCDEN_PWMEN);
	hlcdc_writel(LCDC_LCDEN, val | LCDC_LCDEN_PWMEN);

	ret = hlcdc_readl_with_timeout(LCDC_LCDSR, LCDC_LCDSR_PWM_ACTIVE);

	if (ret){
		dbg_info("Error setting LCD_PWMEN\n");
	}

	dbg_info("\nBASE_SR = %x\n", hlcdc_readl(0x68));
	dbg_info("BASEISR = %x\n", hlcdc_readl(LCDC_LCDC_BASEISR));


	/* queue next DMA descriptor */
	//hlcdc_writel(LCDC_LCDC_HEADADDR, dma_desc.address);
        //val = hlcdc_readl(LCDC_LCDC_BASECHEN);
        //hlcdc_writel(LCDC_LCDC_BASECHEN, val | LCDC_LCDC_BASECHEN_CHEN_EN | LCDC_LCDC_BASECHEN_AQEN_EN);

	dbg_info("\n After queuing \nBASE_SR = %x\n", hlcdc_readl(0x68));
	dbg_info("BASEISR = %x\n", hlcdc_readl(LCDC_LCDC_BASEISR));

	// kernel seems to be clobbering our boot image immediately when this is removed
	// my guess is it is because we enabled console output to LCD in kernel meaning it inits display immediately ?
	// either way still critical since UI application will launch after some delay as part of init script
	//while (1) {
//
//	}
	return ret;
}

