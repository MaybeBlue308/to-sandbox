/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CFG_SYS_SDRAM_BASE		0x80000000

#define RISCV_MMODE_TIMERBASE		0x2000000
#define RISCV_MMODE_TIMEROFF		0xbff8
#define RISCV_MMODE_TIMER_FREQ		1000000
#define RISCV_SMODE_TIMER_FREQ		1000000

/* Environment options */

#ifdef CONFIG_SOVRA_ZCU102
#define CFG_STD_DEVICES_SETTINGS	"stdin=serial\0" \
					"stdout=serial\0" \
					"stderr=serial\0"

/* Legacy distro discovery; actual access still requires board drivers/DT. */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(DHCP, dhcp, na) \
	func(SOVRA_QSPI, qspi, na)

#define BOOTENV_DEV_SOVRA_QSPI(devtypeu, devtypel, instance) \
	"bootcmd_qspi=run boot_qspi\0"
#define BOOTENV_DEV_NAME_SOVRA_QSPI(devtypeu, devtypel, instance) "qspi "
#define BOOTENV_RUN_TARGETS "sovra_boot_targets"

/* RAM layout shared with the Sovra harness JTAG memory map. */
#define SOVRA_MEM_ENV \
	"kernel_addr_r=0x81000000\0" \
	"fdt_addr_r=0x82000000\0" \
	"ramdisk_addr_r=0x84000000\0"

/*
 * Boot-source menu. bootcmd dispatches bootmode after CONFIG_BOOTDELAY;
 * any key during the countdown runs menucmd (AUTOBOOT_MENU_ON_ANY_KEY).
 *
 * Shared helpers use chk_dtb/chk_kernel/chk_rootfs as the addresses to
 * validate and set boot_errmsg on failure. rootfs_size is taken from the
 * per-source *_rootfs_size variable, else from /chosen linux,initrd-{start,end}
 * in the Linux DTB. All messages go to the console (UART).
 */
#define SOVRA_BOOT_ENV \
	"bootmenu_0=Boot từ QSPI=run boot_qspi\0" \
	"bootmenu_1=Boot từ eMMC=run boot_emmc\0" \
	"bootmenu_2=Network Boot=run boot_net\0" \
	"bootmenu_3=Update Firmware=run update_firmware\0" \
	"bootmenu_4=U-Boot console=run boot_console\0" \
	"bootmode=distro_boot\0" \
	"boot_selected=if test -z \"${bootmode}\" || " \
		"test \"${bootmode}\" = distro_boot; then run boot_distro; " \
		"elif test \"${bootmode}\" = qspi; then run boot_qspi; " \
		"elif test \"${bootmode}\" = emmc; then run boot_emmc; " \
		"elif test \"${bootmode}\" = ethernet || " \
		"test \"${bootmode}\" = network; then run boot_net; " \
		"else echo Invalid bootmode: ${bootmode}; false; fi\0" \
	"boot_distro=echo Legacy distro boot: ${boot_targets}; " \
		"setenv autoload yes; if run distro_bootcmd; then " \
		"setenv autoload no; true; else setenv autoload no; false; fi\0" \
	"menucmd=bootmenu -1\0" \
	"autoload=no\0" \
	"qspi_dtb_addr=0xc0500000\0" \
	"qspi_kernel_addr=0xc0600000\0" \
	"qspi_rootfs_addr=0xc1a00000\0" \
	"emmc_dev=0\0" \
	"emmc_part=1\0" \
	"emmc_dtb=dtb.dtb\0" \
	"emmc_kernel=Image\0" \
	"emmc_rootfs=rootfs.cpio.gz\0" \
	"net_boot_file=os.itb\0" \
	"net_dtb=dtb.dtb\0" \
	"net_rootfs=rootfs.cpio.gz\0" \
	"update_addr=0x88000000\0" \
	"update_offset=0x00000000\0" \
	"update_env_start=0x00700000\0" \
	"update_erase_size=0x00001000\0" \
	"update_spi_bus=1\0" \
	"update_spi_cs=0\0" \
	"update_spi_hz=10000000\0" \
	"boot_console=echo \"[SOVRA BOOT] Exit to U-Boot console. " \
		"Type bootmenu to reopen the boot menu.\"\0" \
	"boot_fail=echo \"[SOVRA BOOT] ERROR: ${boot_src} boot failed: " \
		"${boot_errmsg}\"; false\0" \
	"check_dtb=if fdt addr -q ${chk_dtb}; then true; else " \
		"setenv boot_errmsg \"no valid device tree at ${chk_dtb}\"; " \
		"false; fi\0" \
	"check_kernel=setexpr chk_tmp ${chk_kernel} + 0x38; " \
		"if itest.l *${chk_tmp} == 0x05435352; then true; else " \
		"setenv boot_errmsg \"no RISC-V Linux Image at ${chk_kernel}\"; " \
		"false; fi\0" \
	"check_rootfs=if itest.w *${chk_rootfs} == 0x8b1f; then true; else " \
		"setenv boot_errmsg \"no gzip initramfs at ${chk_rootfs}\"; " \
		"false; fi\0" \
	"get_rootfs_size=if test -n \"${rootfs_size}\"; then true; " \
		"elif fdt addr -q ${chk_dtb} && " \
		"fdt get value chk_rs /chosen linux,initrd-start 1 && " \
		"fdt get value chk_re /chosen linux,initrd-end 1; then " \
		"setexpr rootfs_size ${chk_re} - ${chk_rs}; else " \
		"setenv boot_errmsg \"rootfs size unknown: set " \
		"${boot_src_lc}_rootfs_size or put linux,initrd-start/end " \
		"in /chosen of the Linux DTB\"; false; fi\0" \
	"boot_linux=echo \"[SOVRA BOOT] booti kernel=${kernel_addr_r} " \
		"initrd=${ramdisk_addr_r}:${rootfs_size} fdt=${fdt_addr_r}\"; " \
		"booti ${kernel_addr_r} ${ramdisk_addr_r}:${rootfs_size} " \
		"${fdt_addr_r}; setenv boot_errmsg \"booti returned\"; false\0" \
	"boot_qspi=setenv boot_src QSPI; setenv boot_src_lc qspi; " \
		"setenv rootfs_size ${qspi_rootfs_size}; " \
		"setenv chk_dtb ${qspi_dtb_addr}; " \
		"setenv chk_kernel ${qspi_kernel_addr}; " \
		"setenv chk_rootfs ${qspi_rootfs_addr}; " \
		"echo \"[SOVRA BOOT] Boot source: QSPI (dtb ${qspi_dtb_addr}, " \
		"kernel ${qspi_kernel_addr}, rootfs ${qspi_rootfs_addr})\"; " \
		"if run check_dtb && run check_kernel && run check_rootfs && " \
		"run get_rootfs_size; then " \
		"setenv boot_errmsg \"QSPI image copy to DDR failed\"; " \
		"if fdt header get chk_tmp totalsize && " \
		"cp.b ${qspi_dtb_addr} ${fdt_addr_r} ${chk_tmp} && " \
		"setexpr chk_tmp ${qspi_kernel_addr} + 0x10 && " \
		"setexpr.l chk_tmp *${chk_tmp} && " \
		"cp.b ${qspi_kernel_addr} ${kernel_addr_r} ${chk_tmp} && " \
		"cp.b ${qspi_rootfs_addr} ${ramdisk_addr_r} ${rootfs_size}; then " \
		"run boot_linux; fi; fi; run boot_fail\0" \
	"boot_emmc=setenv boot_src eMMC; setenv boot_src_lc emmc; " \
		"echo \"[SOVRA BOOT] Boot source: eMMC (mmc ${emmc_dev}:${emmc_part})\"; " \
		"if mmc dev ${emmc_dev}; then " \
		"if load mmc ${emmc_dev}:${emmc_part} ${fdt_addr_r} ${emmc_dtb} && " \
		"load mmc ${emmc_dev}:${emmc_part} ${kernel_addr_r} ${emmc_kernel} && " \
		"load mmc ${emmc_dev}:${emmc_part} ${ramdisk_addr_r} ${emmc_rootfs}; then " \
		"setenv rootfs_size ${filesize}; run boot_ram_images; else " \
		"setenv boot_errmsg \"cannot load ${emmc_dtb}, ${emmc_kernel}, " \
		"${emmc_rootfs} from mmc ${emmc_dev}:${emmc_part}\"; fi; else " \
		"setenv boot_errmsg \"eMMC device ${emmc_dev} not found " \
		"(no MMC controller in the device tree or card not detected)\"; fi; " \
		"run boot_fail\0" \
	"boot_net=setenv boot_src Network; setenv boot_src_lc net; " \
		"setenv boot_errmsg; " \
		"setenv autoload no; " \
		"echo \"[SOVRA BOOT] Network Boot (DHCP + TFTP): ${net_boot_file}\"; " \
		"if dhcp; then " \
		"if test -n \"${net_serverip}\"; then setenv serverip ${net_serverip}; fi; " \
		"echo \"[SOVRA BOOT] TFTP server ${serverip}, ip ${ipaddr}\"; " \
		"if tftpboot ${kernel_addr_r} ${net_boot_file}; then " \
		"if itest.l *${kernel_addr_r} == 0xedfe0dd0; then " \
		"echo \"[SOVRA BOOT] FIT detected; bootm ${kernel_addr_r}${net_fit_config}\"; " \
		"bootm ${kernel_addr_r}${net_fit_config}; " \
		"setenv boot_errmsg \"bootm returned after FIT validation/boot\"; " \
		"else setexpr chk_tmp ${kernel_addr_r} + 0x38; " \
		"if itest.l *${chk_tmp} == 0x05435352; then " \
		"echo \"[SOVRA BOOT] raw RISC-V Image detected; loading DTB and initramfs\"; " \
		"if tftpboot ${fdt_addr_r} ${net_dtb} && " \
		"tftpboot ${ramdisk_addr_r} ${net_rootfs}; then " \
		"setenv rootfs_size ${filesize}; run boot_ram_images; else " \
		"setenv boot_errmsg \"TFTP failed for ${net_dtb} or ${net_rootfs}\"; fi; " \
		"else setenv boot_errmsg \"${net_boot_file} is neither FIT nor RISC-V Image\"; fi; fi; " \
		"else setenv boot_errmsg \"TFTP download of ${net_boot_file} failed\"; fi; " \
		"else setenv boot_errmsg \"no Ethernet interface found or DHCP failed\"; fi; " \
		"setenv autoload no; run boot_fail\0" \
	"boot_eth=run boot_net\0" \
	"update_firmware=setenv boot_src \"Firmware update\"; " \
		"setenv boot_errmsg; setenv autoload no; " \
		"if test -z \"${update_file}\"; then " \
		"setenv boot_errmsg \"set update_file and optional update_offset first\"; " \
		"else echo \"[SOVRA UPDATE] External NOR only: ${update_file} -> ${update_offset}\"; " \
		"if dhcp; then " \
		"if test -n \"${update_serverip}\"; then setenv serverip ${update_serverip}; fi; " \
		"if tftpboot ${update_addr} ${update_file}; then " \
		"setexpr update_align ${update_offset} % 0x1000; " \
		"setexpr update_erase_size ${filesize} + 0xfff; " \
		"setexpr update_erase_size ${update_erase_size} / 0x1000; " \
		"setexpr update_erase_size ${update_erase_size} * 0x1000; " \
		"setexpr update_end ${update_offset} + ${update_erase_size}; " \
		"if itest ${update_align} == 0 && itest ${filesize} > 0 && " \
		"itest ${update_offset} < ${update_env_start} && " \
		"itest ${update_end} > ${update_offset} && " \
		"itest ${update_end} <= ${update_env_start}; then " \
		"if sf probe ${update_spi_bus}:${update_spi_cs} ${update_spi_hz} 0 && " \
		"sf erase ${update_offset} +${filesize} && " \
		"sf write ${update_addr} ${update_offset} ${filesize}; then " \
		"echo \"[SOVRA UPDATE] PASS: wrote ${filesize} bytes; ENV A/B preserved\"; " \
		"true; else setenv boot_errmsg \"SPI probe/erase/write failed\"; fi; " \
		"else setenv boot_errmsg \"invalid range: offset must be 4-KiB aligned and end before 0x700000\"; fi; " \
		"else setenv boot_errmsg \"TFTP download of ${update_file} failed\"; fi; " \
		"else setenv boot_errmsg \"no Ethernet interface found or DHCP failed\"; fi; fi; " \
		"setenv autoload no; if test -n \"${boot_errmsg}\"; then " \
		"run boot_fail; else true; fi\0" \
	"boot_jtag=setenv boot_src JTAG; setenv boot_src_lc jtag; " \
		"setenv rootfs_size ${jtag_rootfs_size}; " \
		"echo \"[SOVRA BOOT] Boot source: JTAG (images preloaded in RAM: " \
		"dtb ${fdt_addr_r}, kernel ${kernel_addr_r}, rootfs ${ramdisk_addr_r})\"; " \
		"run boot_ram_images; run boot_fail\0" \
	"boot_ram_images=setenv chk_dtb ${fdt_addr_r}; " \
		"setenv chk_kernel ${kernel_addr_r}; " \
		"setenv chk_rootfs ${ramdisk_addr_r}; " \
		"if run check_dtb && run check_kernel && run check_rootfs && " \
		"run get_rootfs_size; then run boot_linux; fi; false\0"
#else
#define SOVRA_MEM_ENV \
	"kernel_addr_r=0x84000000\0" \
	"fdt_addr_r=0x8c000000\0" \
	"ramdisk_addr_r=0x8c300000\0"
#define SOVRA_BOOT_ENV
#define CFG_STD_DEVICES_SETTINGS	"stdin=serial,usbkbd\0" \
					"stdout=serial,vidconsole\0" \
					"stderr=serial,vidconsole\0"

#define BOOT_TARGET_DEVICES(func) \
	func(NVME, nvme, 0) \
	func(VIRTIO, virtio, 0) \
	func(VIRTIO, virtio, 1) \
	func(SCSI, scsi, 0) \
	func(DHCP, dhcp, na)
#endif

#include <config_distro_bootcmd.h>

#define CFG_EXTRA_ENV_SETTINGS \
	CFG_STD_DEVICES_SETTINGS \
	"initrd_high=0xffffffffffffffff\0" \
	SOVRA_MEM_ENV \
	"kernel_comp_addr_r=0x88000000\0" \
	"kernel_comp_size=0x4000000\0" \
	"scriptaddr=0x8c100000\0" \
	"pxefile_addr_r=0x8c200000\0" \
	SOVRA_BOOT_ENV \
	BOOTENV

#endif /* __CONFIG_H */
