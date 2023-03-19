
#define FRAME_BUFFER_ADDR 0x2FE00000
#define BMP_COPY_ADDR     0x2FF00000

#define VID_SIZE_BYTES 400*240*2 // 400*240*16BPP/ (8bits/byte) [bytes]

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
};

struct bitmap {
	struct bitmap_header hdr;
	struct bitmap_dib_header dib;
	unsigned char* pixels;
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
