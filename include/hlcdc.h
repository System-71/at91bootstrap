


#ifndef HLCDC_H_
#define HLCDC_H_

/* Register definitions */

/* LCD enable Register */

#define LCDC_LCDEN          0x20u
#define LCDC_LCDEN_PWMEN   (0x1 << 3) 
#define LCDC_LCDEN_DISPEN  (0x1 << 2) 
#define LCDC_LCDEN_SYNCEN  (0x1 << 1) 
#define LCDC_LCDEN_PCLKEN  (0x1 << 0) 

/* LCD disable Register */

#define LCDC_LCDDIS          0x24u
#define LCDC_LCDDIS_PWMDIS   (1 << 3) 
#define LCDC_LCDDIS_DISPDIS  (1 << 2) 
#define LCDC_LCDDIS_SYNCDIS  (1 << 1) 
#define LCDC_LCDDIS_PCLKDIS  (1 << 0) 

/* LCD Status Register */

#define LCDC_LCDSR             0x28u
#define LCDC_LCDSR_DISP_ACTIVE (1 << 2) /* display signal active */
#define LCDC_LCDSR_SYNC_ACTIVE (1 << 1) /* Synchronization status, timing engine running */
#define LCDC_LCDSR_PCLK_ACTIVE (1 << 0) /* Clock status, pixel clock running */
#define LCDC_LCDSR_PWM_ACTIVE  (1 << 3) /* PWM status, PWM signal is active */

/* LCD Controller Configuration Register 0 */

#define LCDC_LCDCFG0              0x00u
#define LCDC_LCDCFG0_CLKDIV_SHIFT 16
#define LCDC_LCDCFG0_CLKDIV_MASK  (0xFFu << LCDC_LCDCFG0_CLKDIV_SHIFT)
#define LCDC_LCDCFG0_CGDISHEO     (1 << 11)    /* Clock gating disable control for High-End overlay */
#define LCDC_LCDCFG0_CGDISOVR2    (1 << 10)    /* Clock gating disable control for overlay layer 2  */
#define LCDC_LCDCFG0_CGDISOVR1    (1 << 9)     /* Clock gating disable control for overlay layer 1  */
#define LCDC_LCDCFG0_CGDISBASE    (1 << 8)     /* Clock gating disable control for base layer       */
#define LCDC_LCDCFG0_CLKPWMSEL    (1 << 3)     /* PWM clock source - 1: sys clock, 0: slow clock    */
#define LCDC_LCDCFG0_CLKPOL       (1 << 0)     /* 1: Data control signals launched on falling edge of PCLK */

/* LCD Controller Configuration Register 5 */

#define LCDC_LCDCFG5 0x14
#define LCDC_LCDCFG5_GUARD_TIME_MASK (0xFFu << 16)
#define LCDC_LCDCFG5_VSPHO           (1 << 13)    /* VSYNC pulse held active one pixel clock cycle after HSYNC pulse */
#define LCDC_LCDCFG5_VSPSU           (1 << 12)    /* VSYNC pulse asserted one pixel clock cycle before HSYNC pulse   */
#define LCDC_LCDCFG5_MODE_SHIFT      8            /* Bits per pixel setting                                          */
#define LCDC_LCDCFG5_MODE_MASK       (0x3u << LCDC_LCDCFG5_MODE_SHIFT)
#define LCDC_LCDCFG5_DISPDLY         (1 << 7)
#define LCDC_LCDCFG5_DITHER_EN       (1 << 6)     /* Dither enable                                                   */
#define LCDC_LCDCFG5_DISPPOL         (1 << 4)     /* DISP signal polarity, 0: active HIGH, 1: active LOW             */
#define LCDC_LCDCFG5_VSPDLYE         (1 << 3)     /* VSYNC 2nd edge is in sync with first edge of HSYNC              */
#define LCDC_LCDCFG5_VSPDLYS         (1 << 2)     /* VSYNC 1st edge is in sync with first edge of HSYNC              */
#define LCDC_LCDCFG5_VSPOL           (1 << 1)     /* VSYNC pulse polarity, 0: active HIGH, 1: active LOW             */
#define LCDC_LCDCFG5_HSPOL           (1 << 0)     /* HSYNC pulse polarity, 0: active HIGH, 1: active LOW             */

/* LCD Controller Configuration Register 1 */

#define LCDC_LCDCFG1 0x04
#define LCDC_LCDCFG1_VSYNCPW_SHIFT 16
#define LCDC_LCDCFG1_VSYNCPW_MASK  (0x3FFu << LCDC_LCDCFG1_VSYNCPW_SHIFT)
#define LCDC_LCDCFG1_HSYNCPW_MASK  (0x3FFu << 0)


/* LCD Controller Configuration Register 2 */

#define LCDC_LCDCFG2 0x08
#define LCDC_LCDCFG2_VBPW_SHIFT 16
#define LCDC_LCDCFG2_VBPW_MASK  (0x3FFu << LCDC_LCDCFG2_VBPW_SHIFT)
#define LCDC_LCDCFG2_VFBPW_MASK (0x3FFu << 0)

/* LCD Controller Configuration Register 3 */

#define LCDC_LCDCFG3 0x0C
#define LCDC_LCDCFG3_HBPW_SHIFT 16 
#define LCDC_LCDCFG3_HBPW_MASK (0x3FFu << LCDC_LCDCFG3_HBPW_SHIFT)
#define LCDC_LCDCFG3_HFPW_MASK (0x3FFu << 0)

/* LCD Controller Configuration Register 4 */

#define LCDC_LCDCFG4 0x10
#define LCDC_LCDCFG4_RPF_SHIFT 16
#define LCDC_LCDCFG4_RPF_MASK (0x3FFu << LCDC_LCDCFG4_RPF_SHIFT)
#define LCDC_LCDCFG4_PPL_MASK (0x3FFu << 0)


/* LCD Controller DMA Base Control Register */

#define LCDC_LCDC_BASECTRL 0x84
#define LCDC_LCDC_BASECTRL_DONEIEN_SHIFT 5
#define LCDC_LCDC_BASECTRL_DONEIEN_EN    (1u << LCDC_LCDC_BASECTRL_DONEIEN_SHIFT)
#define LCDC_LCDC_BASECTRL_ADDIEN_SHIFT 4
#define LCDC_LCDC_BASECTRL_ADDIEN_EN    (1u << LCDC_LCDC_BASECTRL_ADDIEN_SHIFT)
#define LCDC_LCDC_BASECTRL_DSCRIEN_SHIFT 3
#define LCDC_LCDC_BASECTRL_DSCRIEN_DIS   (1u << LCDC_LCDC_BASECTRL_DSCRIEN_SHIFT)
#define LCDC_LCDC_BASECTRL_DMAIEN_SHIFT 2
#define LCDC_LCDC_BASECTRL_DMAIEN_EN    (1u << LCDC_LCDC_BASECTRL_DMAIEN_SHIFT)
#define LCDC_LCDC_BASECTRL_DFETCH_SHIFT 0
#define LCDC_LCDC_BASECTRL_DFETCH_EN    (1u << LCDC_LCDC_BASECTRL_DFETCH_SHIFT)

/* LCD Controller DMA Base Address Register */

#define LCDC_LCDC_BASEADDR 0x80

/* LCD Controller DMA Next Address Register */

#define LCDC_LCDC_NEXTADDR 0x88

/* LCD Controller DMA HEAD Address Register */

#define LCDC_LCDC_HEADADDR 0x7C

/* LCD Controller DMA Base Channel Enable Register */

#define LCDC_LCDC_BASECHEN 0x60
#define LCDC_LCDC_BASECHEN_CHEN_SHIFT    0
#define LCDC_LCDC_BASECHEN_CHEN_EN       (1u << LCDC_LCDC_BASECHEN_CHEN_SHIFT)
#define LCDC_LCDC_BASECHEN_UPDATEN_SHIFT 1
#define LCDC_LCDC_BASECHEN_UPDATEN_EN    (1u << LCDC_LCDC_BASECHEN_UPDATEN_SHIFT)
#define LCDC_LCDC_BASECHEN_AQEN_SHIFT    2
#define LCDC_LCDC_BASECHEN_AQEN_EN       (1u << LCDC_LCDC_BASECHEN_AQEN_SHIFT)

/* LCD Controller DMA Base Interrupt Status Register */

#define LCDC_LCDC_BASEISR 0x78

struct hlcdc_dma_desc {
	unsigned int address;
	unsigned int control;
	unsigned int next;
};

struct video_buf {
	unsigned int size;
	unsigned long base;
};

extern int hlcdc_init(struct video_buf* vid_buf);

#endif

