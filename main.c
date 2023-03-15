// Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include "common.h"
#include "debug.h"
#include "board.h"
#include "usart.h"
#include "slowclk.h"
#include "board_hw_info.h"
#include "tz_utils.h"
#include "pm.h"
#include "act8865.h"
#include "mcp16502.h"
#include "backup.h"
#include "secure.h"
#include "autoconf.h"
#include "optee.h"
#include "sfr_aicredir.h"
#include "bitmap.h"
#include "string.h"


#ifdef CONFIG_LCD
#include "hlcdc.h"
#endif

#ifdef CONFIG_CACHES
#include "l1cache.h"
#endif

#ifdef CONFIG_MMU
#include "mmu.h"
static unsigned int *tlb = (unsigned int *)MMU_TABLE_BASE_ADDR;
#endif

#ifdef CONFIG_HW_DISPLAY_BANNER
static void display_banner (void)
{
	usart_puts(BANNER);
}
#endif

#ifdef CONFIG_LCD
struct hlcdc_dma_desc dma_desc; // prevent stack from clobbering our dma struct
#endif

int main(void)
{
#ifdef CONFIG_LOAD_SW
	struct image_info image;
#endif
	int ret = 0;

	hw_init();

/* enable display early */
#if defined(CONFIG_LCD)

	struct image_info *img; 

	memset(img,		0, sizeof(struct image_info));

	#if defined(CONFIG_DATAFLASH) || defined(CONFIG_NANDFLASH) || defined(CONFIG_FLASH)
		img->length = 0x100000; // copy 1MB
		img->offset = 0x600000; // stored at 7MB offset in QSPI
	#else 
		dbg_info("Unsupported medium for bitmap storage\n");
		return;
	#endif 
	
	img->dest = (unsigned char *) 0x2FF00000; // copy to 2nd last 1MB sector of DRAM

	/* video diplay buffer */

	load_function load_image;

	load_image = get_image_load_func();

	ret = load_image(img, 0);

	if (ret != 0) {
		dbg_info("failed to copy bitmap from storage\n");
	}

	struct bitmap_dib_header stack_dib; 
	memset(&stack_dib, 0, sizeof(struct bitmap_dib_header));

	struct bitmap_header stack_hdr; 
	memset(&stack_hdr, 0, sizeof(struct bitmap_header));

	struct bitmap* bmp;

	/* point bitmap to location in storage */
	bmp = (struct bitmap*) img->dest; 

	unsigned long* hdr_addr;

	hdr_addr = (unsigned long*) &bmp->dib.size;
	dbg_info("bitmap dbi header %x @hdr_addr: %d bytes\n", (unsigned int) hdr_addr, sizeof(struct bitmap_header));

	int bpp;
	/* since dib header is 14 bytes into bitmap, may not meet DRAM alignment requirement of 4-bytes */
	if (((int)hdr_addr % 32) != 0) {
		/* not 4-byte aligned */
		dbg_info("bitmap dbi header is not 4-byte aligned in memory. Copy to stack.\n");

		unsigned char* dst = (unsigned char*) &stack_hdr;
		unsigned char* src = (unsigned char*) &bmp->hdr;
		for (int i=0; i < sizeof(struct bitmap_header); i++) {
			dst[i] = src[i];
		}
		dst = (unsigned char*) &stack_dib;
		int offset = sizeof(struct bitmap_header);
		for (int i=offset; i < (sizeof(struct bitmap_dib_header)+sizeof(struct bitmap_header)); i++) {
			dst[i-offset] = src[i];
		}
		dbg_info("bitmap found at %x: %dx%d pixels - type %d BPP, header size = %d, pixel array @%x\n", &stack_dib, (int) stack_dib.height, (int) stack_dib.width, (unsigned int)((0xFF00 & (stack_dib.bpp[1]) << 8) | stack_dib.bpp[0]), stack_dib.size, stack_hdr.offset[0]);
	}
	else {
		stack_hdr = bmp->hdr;
		stack_dib = bmp->dib;
	}
	
	bmp->pixels = (unsigned int*) (bmp + stack_hdr.offset[0]);
	bpp = (unsigned int)((0xFF00 & (stack_dib.bpp[1]) << 8) | stack_dib.bpp[0]);

	int num_bytes = bpp*stack_dib.height*stack_dib.width/8;

	#define VID_SIZE_BYTES 400*240*2 // 400*240*16BPP/ (8bits/byte) [bytes]

	// TODO dynamically assign this address to DRAM_BASE_OFFSET + DRAM_SIZE - 1MB
	unsigned long addr = 0x2FE00000;  // frame buffer at last 1MB of memory (256MB - 1MB)
	unsigned char* frame; 

	frame = (unsigned char*) addr;

	char red=0;
	char green=0;
	char blue=0;
	char red16=0;
	char green16=0;
	char blue16=0;

	dbg_info("pixel address = %x\n", (int) bmp->pixels);
	dbg_info("bpp = %d\n", (int)bpp);

	int i = 0;
	int offset = stack_dib.width; // center w.r.t to x-axis

	for (int j=0; j < VID_SIZE_BYTES; j++) {
		/* blank the screen */
		frame[j] = 0x00;
	}

	for (int j=0; j < num_bytes; j++) {

		switch(bpp) {
			case(BPP_32BIT):

				if ( (j % (32/8)) == 0 ) {
					// pixel every 4 bytes
					red   = bmp->pixels[j-1];
		   			green = bmp->pixels[j-2];
		   			blue  = bmp->pixels[j-3];

					red16   = (red * ( 1 << 5 )/( 1 << 8 ) ) & 0x1F;
					green16 = (green*( 1 << 6 )/( 1 << 8 ) ) & 0x3F;
					blue16  = (blue *( 1 << 5 )/( 1 << 8 ) ) & 0x1F;

					frame[i + offset]   = 0xFF & (red16 | ((0x7 & green16) << 5));
					frame[i + offset + 1] = 0xFF & (green16 | ((0x1F & blue16) << 4));
					i += 2;
				}
				break; 
			default:
				dbg_info("Bitmap uses unsupported bits per pixel\n");
				break;
		}
	}

	struct video_buf vid = {.size = (VID_SIZE_BYTES), .base = addr };

	ret = hlcdc_init(&vid);

	load_image = &load_kernel;

#endif /* #ifdef CONFIG_LCD */

#ifdef CONFIG_OCMS_STATIC
	ocms_init_keys();
	ocms_enable();
#endif

#if defined(CONFIG_SCLK)
#if !defined(CONFIG_SCLK_BYPASS)
	slowclk_enable_osc32();
#endif
#elif defined(CONFIG_SCLK_INTRC)
	slowclk_switch_rc32();
#endif

#ifdef CONFIG_BACKUP_MODE
	ret = backup_mode_resume();
	if (ret) {
		/* Backup+Self-Refresh mode detected... */
#ifdef CONFIG_REDIRECT_ALL_INTS_AIC
		redirect_interrupts_to_nsaic();
#endif
		slowclk_switch_osc32();

		/* ...jump to Linux here */
		return ret;
	}
	usart_puts("Backup mode enabled\n");
#endif

#ifdef CONFIG_HW_DISPLAY_BANNER
	display_banner();
#endif

#ifdef CONFIG_REDIRECT_ALL_INTS_AIC
	redirect_interrupts_to_nsaic();
#endif

#ifdef CONFIG_LOAD_HW_INFO
	load_board_hw_info();
#endif

#ifdef CONFIG_PM
	at91_board_pm();
#endif

#ifdef CONFIG_ACT8865
	act8865_workaround();

	act8945a_suspend_charger();
#endif

#ifdef CONFIG_MCP16502_SET_VOLTAGE
	mcp16502_voltage_select();
#endif

#ifdef CONFIG_SAMA7G5
	hw_postinit();
#endif

#ifdef CONFIG_LOAD_SW
	init_load_image(&image);

#if defined(CONFIG_SECURE)
	image.dest -= sizeof(at91_secure_header_t);
#endif

#ifdef CONFIG_MMU
	mmu_tlb_init(tlb);
	mmu_configure(tlb);
	mmu_enable();
#endif
#ifdef CONFIG_CACHES
	icache_enable();
	dcache_enable();
#endif

	ret = (*load_image)(&image, 1);

#ifdef CONFIG_CACHES
	icache_disable();
	dcache_disable();
#endif
#ifdef CONFIG_MMU
	mmu_disable();
#endif

#if defined(CONFIG_SECURE)
	if (!ret)
		ret = secure_check(image.dest);
	image.dest += sizeof(at91_secure_header_t);
#endif

#endif
	load_image_done(ret);

#ifdef CONFIG_SCLK
#ifdef CONFIG_SCLK_BYPASS
	slowclk_switch_osc32_bypass();
#else
	slowclk_switch_osc32();
#endif
#endif


#if defined(CONFIG_LOAD_OPTEE)
	/* Will never return since we will jump to OP-TEE in secure mode */
	optee_load();
#endif

#if defined(CONFIG_ENTER_NWD)
	switch_normal_world();

	/* point never reached with TZ support */
#endif

#ifdef CONFIG_JUMP_TO_SW
	return JUMP_ADDR;
#else
	return 0;
#endif
}
