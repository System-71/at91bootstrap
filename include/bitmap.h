
#define FRAME_BUFFER_ADDR 0x2FE00000
#define BMP_COPY_ADDR     0x2FF00000

#define VID_SIZE_BYTES 320*240*2 // 320*240*16BPP/ (8bits/byte) [bytes]
#define VID_BPP 16

// uncomment to flip X axis
#define VID_X_MIRROR

struct bitmap_header {
	unsigned char header[2];
	unsigned char size[4];
	unsigned char reserved[4];
	unsigned char offset[4];
};

/*
/ https://en.wikipedia.org/wiki/File:WinBmpHeaders.png
/ entries are unsigned int 
*/
struct bitmap_dib_header {
	unsigned int size;
	int width;
	int height;
	unsigned char planes[2];
	unsigned char bpp[2];
	int compression;
	int picsize;
	int xpx_per_meter;
	int ypx_per_meter;
	int colors;
	int important_colors;
	unsigned int red_mask;
	unsigned int green_mask;
	unsigned int blue_mask;
	unsigned int alpha_mask;
};

struct bitmap {
	struct bitmap_header hdr;
	struct bitmap_dib_header dib;
	unsigned int* pixels;
};

enum bitmap_bpp {
	BPP_16BIT = 16,
	BPP_18BIT = 18,
	BPP_24BIT = 24,
	BPP_32BIT = 32,
};

struct video_buf {
	unsigned int size;
	unsigned long base;
};

struct video_buf bitmap_frame_buffer_init();
