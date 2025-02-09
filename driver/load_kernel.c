// Copyright (C) 2006 Microchip Technology Inc. and its subsidiaries
//
// SPDX-License-Identifier: MIT

#include "common.h"
#include "hardware.h"
#include "board.h"
#include "arch/at91_pmc/pmc.h"
#include "string.h"
#include "slowclk.h"
#include "dataflash.h"
#include "ddramc.h"
#include "nandflash.h"
#include "optee.h"
#include "flash.h"
#include "sdcard.h"
#include "sdramc.h"
#include "fdt.h"
#include "board_hw_info.h"
#include "mon.h"
#include "tz_utils.h"
#include "secure.h"

#include "arch/at91-qspi/qspi.h"
#include "spi_flash/spi_nor.h"
#include "at91-qspi/qspi-common.h"
#include "hlcdc.h"

#include "debug.h"

static char cmdline_buf[256];
static char *bootargs;

#ifdef CONFIG_OF_LIBFDT

static int setup_dt_blob(void *blob)
{
	int ret;
#if !defined(CONFIG_LOAD_OPTEE)
	unsigned int mem_bank = AT91C_BASE_DDRCS;
	unsigned int mem_bank2 = 0;
	unsigned int mem_size = 0;
#if defined(CONFIG_SDRAM)
	mem_size = get_sdram_size();
#elif defined(CONFIG_DDRC) || defined(CONFIG_UMCTL2)
	mem_size = get_ddram_size();
#else
#error "No DRAM type specified!"
#endif
#endif

	if (check_dt_blob_valid(blob)) {
		dbg_info("DT: the blob is not a valid fdt\n");
		return -1;
	}

	dbg_info("DT: Using device tree in place at %x\n",
						(unsigned int)blob);

	/* no point in fixing if we do not have configured bootargs */
	if (bootargs && *bootargs) {
		char *p;

		/* set "/chosen" node */
		for (p = bootargs; *p == ' '; p++) /* skip spaces */
			;

		if (*p == '\0')
			return -1;

		ret = fixup_chosen_node(blob, p);
		if (ret)
			return ret;
	}

/*
 * When using OP-TEE the memory node should match the configuration of the DDR
 * that has been secured. Since this can't easily be inferred from
 * at91bootstrap, do not modify the memory node and let the user provide a
 * correct device tree. Moreover, the memory node in newer device tree is often
 * already correctly configured.
 */
#if !defined(CONFIG_LOAD_OPTEE)
	ret = fixup_memory_node(blob, &mem_bank, &mem_bank2, &mem_size);
	if (ret)
		return ret;
#endif

	return 0;
}
#else
#define TAG_FLAG_NONE		0x00000000
#define TAG_FLAG_CORE		0x54410001
#define TAG_FLAG_MEM		0x54410002
#define TAG_FLAG_SERIAL		0x54410006
#define TAG_FLAG_REVISION	0x54410007
#define TAG_FLAG_CMDLINE	0x54410009

#define	TAG_SIZE_HEADER		8
#define TAG_SIZE_CORE		5
#define TAG_SIZE_MEM32		4
#define TAG_SIZE_SERIAL		4
#define TAG_SIZE_REVISION	3

struct tag_header {
	unsigned int	size;
	unsigned int	tag;
};

struct tag_core {
	struct tag_header	header;
	unsigned int		flags;
	unsigned int		pagesize;
	unsigned int		rootdev;
};

struct tag_mem32 {
	struct tag_header	header;
	unsigned int		size;
	unsigned int		start;
};

struct tag_serial {
	struct tag_header	header;
	unsigned int		low;
	unsigned int		high;
};

struct tag_revision {
	struct tag_header	header;
	unsigned int		version;
};

struct tag_cmdline {
	struct tag_header	header;
	char			cmdline[1];
};

struct tag_none {
	struct tag_header	header;
};

static void setup_commandline_tag(struct tag_cmdline *params,
						char *commandline)
{
	char *p;

	if (!commandline)
		return;

	for (p = commandline; *p == ' '; p++)
		;

	if (*p == '\0')
		return;

	params->header.tag = TAG_FLAG_CMDLINE;
	params->header.size = (TAG_SIZE_HEADER + strlen(p) + 1 + 4) >> 2;

	strcpy(params->cmdline, p);
}

static void setup_boot_params(void)
{
	unsigned int *params = (unsigned int *)(AT91C_BASE_DDRCS + 0x100);

	struct tag_core *coreparam = (struct tag_core *)params;
	coreparam->header.tag = TAG_FLAG_CORE;
	coreparam->header.size = TAG_SIZE_CORE;

	coreparam->flags = 0;
	coreparam->pagesize = 0;
	coreparam->rootdev = 0;

	params = (unsigned int *)params + TAG_SIZE_CORE;

	struct tag_mem32 *memparam = (struct tag_mem32 *)params;
	memparam->header.tag = TAG_FLAG_MEM;
	memparam->header.size = TAG_SIZE_MEM32;

	memparam->start = AT91C_BASE_DDRCS;

#if defined(CONFIG_SDRAM)
	memparam->size = get_sdram_size();
#elif defined(CONFIG_DDRC)
	memparam->size = get_ddram_size();
#else
#error "No DRAM type specified!"
#endif

	params = (unsigned int *)params + TAG_SIZE_MEM32;

	struct tag_cmdline *cmdparam = (struct tag_cmdline *)params;
	setup_commandline_tag(cmdparam, bootargs);

	params = (unsigned int *)params + cmdparam->header.size;

#ifdef CONFIG_LOAD_ONE_WIRE
	struct tag_revision *revparam = (struct tag_revision *)params;
	revparam->header.tag = TAG_FLAG_REVISION;
	revparam->header.size = TAG_SIZE_REVISION;
	revparam->version = get_sys_rev();

	params = (unsigned int *)params + TAG_SIZE_REVISION;

	struct tag_serial *serialparam = (struct tag_serial *)params;
	serialparam->header.tag = TAG_FLAG_SERIAL;
	serialparam->header.size = TAG_SIZE_SERIAL;
	serialparam->low = get_sys_sn();
	serialparam->high = 0;

	params = (unsigned int *)params + TAG_SIZE_SERIAL;
#endif

	/* end tag */
	struct tag_none * noneparam = (struct tag_none *)params;
	noneparam->header.tag = TAG_FLAG_NONE;
	noneparam->header.size = 0;
}
#endif /* #ifdef CONFIG_OF_LIBFDT */

#if defined(CONFIG_LINUX_IMAGE)

#if defined(CONFIG_QSPI_XIP)

int kernel_size(unsigned char *add)
{
	return 0;
}

static int boot_image_setup(unsigned char *addr, unsigned int *entry)
{
	*entry = (unsigned int)addr;
	return 0;
}
#else

/* Linux uImage Header */
#define LINUX_UIMAGE_MAGIC	0x27051956
struct linux_uimage_header {
	unsigned int	magic;
	unsigned int	header_crc;
	unsigned int	time;
	unsigned int	size;
	unsigned int	load;
	unsigned int	entry_point;
	unsigned int	data_crc;
	unsigned char	os_type;
	unsigned char	arch;
	unsigned char	image_type;
	unsigned char	comp_type;
	unsigned char	name[32];
};

/* Linux zImage Header */
#define	LINUX_ZIMAGE_MAGIC	0x016f2818
struct linux_zimage_header {
	unsigned int	code[9];
	unsigned int	magic;
	unsigned int	start;
	unsigned int	end;
};

int kernel_size(unsigned char *addr)
{
	struct linux_uimage_header *uimage_header
			= (struct linux_uimage_header *)addr;

	struct linux_zimage_header *zimage_header
			= (struct linux_zimage_header *)addr;
	unsigned int size = -1;
	unsigned int magic = swap_uint32(uimage_header->magic);

	if (magic == LINUX_UIMAGE_MAGIC)
		size = swap_uint32(uimage_header->size)
			+ sizeof(struct linux_uimage_header);

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC)
		size = zimage_header->end - zimage_header->start;

	if ((int)size < 0)
		return -1;

	return (int)size;
}

static int boot_image_setup(unsigned char *addr, unsigned int *entry)
{
	struct linux_zimage_header *zimage_header
			= (struct linux_zimage_header *)addr;

	struct linux_uimage_header *uimage_header
			= (struct linux_uimage_header *)addr;
	unsigned int src, dest;
	unsigned int size;
	unsigned int magic;

	dbg_loud("KERNEL: try as zImage: magic=%x\n", zimage_header->magic);
	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = ((unsigned int)addr + zimage_header->start);
		dbg_info("\nKERNEL: Booting zImage ...\n");
		return 0;
	}

	magic = swap_uint32(uimage_header->magic);
	dbg_loud("KERNEL: try as uImage: magic=%x\n", magic);
	if (magic == LINUX_UIMAGE_MAGIC) {
		dbg_info("\nKERNEL: Booting uImage ...\n");

		if (uimage_header->comp_type != 0) {
			dbg_info("KERNEL: No uImage compression is supported!\n");
			return -1;
		}

		size = swap_uint32(uimage_header->size);
		dest = swap_uint32(uimage_header->load);
		src = (unsigned int)addr + sizeof(struct linux_uimage_header);
		*entry = swap_uint32(uimage_header->entry_point);

		dbg_info("KERNEL: Relocating image dest=%x, src=%x\n", dest, src);

		memcpy((void *)dest, (void *)src, size);

		dbg_info("KERNEL: %x bytes relocated\n", size);

		return 0;
	}

	dbg_info("KERNEL: Got unsupported magic!\n"
			"\tas uImage magic: %x\n"
			"\tas zImage magic: %x\n",
			magic, zimage_header->magic);
	return -1;
}
#endif /* !CONFIG_QSPI_XIP */
#endif /* CONFIG_LINUX_IMAGE */

static int load_kernel_image(struct image_info *image)
{
	int ret;
	load_function load_func = get_image_load_func();

	ret = load_func(image, 1);
	if (ret)
		return ret;

	return 0;
}

#ifdef CONFIG_OVERRIDE_CMDLINE_FROM_EXT_FILE
__attribute__((weak)) char *board_override_cmd_line_ext(char *cmdline_args)
{
        return cmdline_args;
}
#endif

__attribute__((weak)) char *board_override_cmd_line(void)
{
	return CMDLINE;
}

#ifdef CONFIG_LINUX_IMAGE_DUAL_BOOT
__attribute__((weak)) unsigned char* dual_bank_scratch_target(void)
{
	return (unsigned char*) SCRATCH_ADDR;
}
#endif 

int load_kernel(struct image_info *image)
{
	unsigned char *addr;
	unsigned int entry_point;
	unsigned int r2;
	unsigned int mach_type;
	int ret;
	unsigned int mem_size;
#ifdef CONFIG_LINUX_IMAGE_DUAL_BOOT
	unsigned char* scratch_offset;
	unsigned char sector[2];
	unsigned char target;
#endif 

#if defined(CONFIG_SDRAM)
	mem_size = get_sdram_size();
#elif defined(CONFIG_DDRC) || defined(CONFIG_UMCTL2)
	mem_size = get_ddram_size();
#else
#error "No DRAM type specified!"
#endif

	void (*kernel_entry)(int zero, int arch, unsigned int params);

	bootargs = board_override_cmd_line();

	if (sizeof(cmdline_buf) < 10 + strlen(bootargs)){
		dbg_very_loud("\nKERNEL: buffer for bootargs is too small\n\n");
		return -1;
	}

	switch(mem_size){
		case 0x800000:
			memcpy(cmdline_buf, "mem=8M ", 7);
			memcpy(&cmdline_buf[7], bootargs, strlen(bootargs));
			break;
		case 0x1000000:
			memcpy(cmdline_buf, "mem=16M ", 8);
			memcpy(&cmdline_buf[8], bootargs, strlen(bootargs));
			break;
		case 0x2000000:
			memcpy(cmdline_buf, "mem=32M ", 8);
			memcpy(&cmdline_buf[8], bootargs, strlen(bootargs));
			break;
		case 0x4000000:
			memcpy(cmdline_buf, "mem=64M ", 8);
			memcpy(&cmdline_buf[8], bootargs, strlen(bootargs));
			break;
		case 0x8000000:
			memcpy(cmdline_buf, "mem=128M ", 9);
			memcpy(&cmdline_buf[9], bootargs, strlen(bootargs));
			break;
		case 0x10000000:
			#ifdef CONFIG_LCD
				memcpy(cmdline_buf, "mem=254M ", 9); // preserve last 1MB frame buffer to protect linux init from overwriting
			#else
				memcpy(cmdline_buf, "mem=256M ", 9); 
			#endif
			memcpy(&cmdline_buf[9], bootargs, strlen(bootargs));
			break;
		case 0x20000000:
			memcpy(cmdline_buf, "mem=512M ", 9);
			memcpy(&cmdline_buf[9], bootargs, strlen(bootargs));
			break;
		default:
			dbg_very_loud("\nKERNEL: bootargs incorrect due to the memory size is not a multiple of MB\n\n");
			break;
	}
	bootargs = cmdline_buf;


#ifdef CONFIG_LINUX_IMAGE_DUAL_BOOT

        const struct spi_flash_hwcaps hwcaps = {
                .mask = (SFLASH_HWCAPS_READ_MASK |
                         SFLASH_HWCAPS_PP_MASK),
        };
        struct spi_flash flash;
        struct qspi_priv qspi;

        memset(&qspi, 0, sizeof(qspi));
        qspi.reg_base = CONFIG_SYS_BASE_QSPI;
        qspi.mem = (void *)CONFIG_SYS_BASE_QSPI_MEM;
        qspi.mmap_size = CONFIG_SYS_QSPI_MEM_SIZE;

        memset(&flash, 0, sizeof(flash));
        flash.ops = &qspi_ops;
        spi_flash_set_priv(&flash, &qspi);

        /* Init the SPI controller. */
        ret = spi_flash_init(&flash);
        if (ret) {
                dbg_info("SF: Fail to initialize spi\n");
                return -1;
        }

        /* Probe the SPI flash memory. */
        ret = spi_nor_probe(&flash, &hwcaps);
        if (ret) {
                dbg_info("SF: Fail to probe SPI flash\n");
                spi_flash_cleanup(&flash);
                return -1;
        }

	scratch_offset = "0x700000"; //TODO actually read offset from config using dual_bank_scratch_target()
	spi_flash_read(&flash, 0x700000, 2, &sector[0]);
        target = sector[0];	
	int b = sector[0];

	dbg_info("b=%d\n", b);

	char partition[20] = {'\0'};
	int key=0;
	int len=(strlen(bootargs)-5);

	for (int i=0; i < len; i++){
		if(strncmp((char*) bootargs+i,"root=",5)==0){
			strncpy(&partition[0], (char *) bootargs+i, 19);
			dbg_info("default partition is %s\n", partition);
			key = i+18;
			break;
		}
	}

	dbg_info("\nTarget @ %s\n", scratch_offset);

	switch(b){
		case 0x0:
			target='1';
			break;
		case 0xFF:
			target='2';
			break;
		default:
			target='1';
	}

	dbg_info("setting partition to %d\n", (int) target);
	bootargs[key] = (unsigned char) target;
#endif

	ret = load_kernel_image(image);
	if (ret)
		return ret;

#ifdef CONFIG_OVERRIDE_CMDLINE_FROM_EXT_FILE
	bootargs = board_override_cmd_line_ext(image->cmdline_args);
#endif
#if defined(CONFIG_SECURE)
	ret = secure_check(image->dest);
	if (ret)
		return ret;
	image->dest += sizeof(at91_secure_header_t);
#endif

#ifdef CONFIG_SCLK
	slowclk_switch_osc32();
#endif

	addr = image->dest;
#if defined(CONFIG_LINUX_IMAGE)
	ret = boot_image_setup(addr, &entry_point);
#endif
	if (ret)
		return -1;

	kernel_entry = (void (*)(int, int, unsigned int))entry_point;

#ifdef CONFIG_OF_LIBFDT
	ret = setup_dt_blob((char *)image->of_dest);
	if (ret)
		return ret;

	mach_type = 0xffffffff;
	r2 = (unsigned int)image->of_dest;
#else
	setup_boot_params();

	mach_type = MACH_TYPE;
	r2 = (unsigned int)(AT91C_BASE_DDRCS + 0x100);
#endif

	dbg_info("\nKERNEL: Starting linux kernel ..., machid: %x\n\n",
							mach_type);

#if defined(CONFIG_ENTER_NWD)
	monitor_init();

	init_loadkernel_args(0, mach_type, r2, (unsigned int)kernel_entry);

	dbg_info("KERNEL: Enter Normal World, Run Kernel at %x\n",
					(unsigned int)kernel_entry);

	enter_normal_world();
#elif defined(CONFIG_LOAD_OPTEE)
	optee_init_nw_params(kernel_entry, 0, mach_type, r2);
#else
	kernel_entry(0, mach_type, r2);
#endif

	return 0;
}
