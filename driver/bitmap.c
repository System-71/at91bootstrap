#include "bitmap.h"
#include "common.h"
#include "string.h"
#include "debug.h"


#ifdef CONFIG_LCD_BOOT_SPLASH
__attribute__((weak)) unsigned char* bitmap_location(void)
{
        return (unsigned char*) BMP_OFFSET;
}
#endif

struct video_buf bitmap_frame_buffer_init() {

	int ret = 0;
	struct image_info *img; 

	struct video_buf vid;
	memset(&vid, 0, sizeof(struct video_buf));

	memset(img,		0, sizeof(struct image_info));

	#if defined(CONFIG_DATAFLASH) || defined(CONFIG_NANDFLASH) || defined(CONFIG_FLASH)
		img->length = 0x100000; // copy 1MB
		img->offset = (unsigned int) atoh((char*) bitmap_location() + 2); // stored at 7MB offset in QSPI
	#else 
		dbg_info("Unsupported medium for bitmap storage\n");
		return vid;
	#endif 
	
	img->dest = (unsigned char*) BMP_COPY_ADDR; // copy to 2nd last 1MB sector of DRAM

	/* video diplay buffer */

	load_function load_image;

	load_image = get_image_load_func();

	ret = load_image(img, 0);

	if (ret != 0) {
		dbg_info("failed to copy bitmap from storage\n");
		return vid;
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

	unsigned long addr = FRAME_BUFFER_ADDR;  // frame buffer at last 1MB of memory (256MB - 1MB)
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

	vid.size = (VID_SIZE_BYTES);
	vid.base = addr;

	return vid;
}
