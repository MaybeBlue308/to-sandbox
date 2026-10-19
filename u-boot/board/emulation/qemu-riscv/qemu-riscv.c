// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/ofnode.h>
#include <env.h>
#include <fdtdec.h>
#include <image.h>
#include <log.h>
#include <spl.h>
#include <init.h>
#include <usb.h>
#include <virtio_types.h>
#include <virtio.h>

DECLARE_GLOBAL_DATA_PTR;

#if IS_ENABLED(CONFIG_MTD_NOR_FLASH)
int is_flash_available(void)
{
	if (!ofnode_equal(ofnode_by_compatible(ofnode_null(), "cfi-flash"),
			  ofnode_null()))
		return 1;

	return 0;
}
#endif

int board_init(void)
{
#if IS_ENABLED(CONFIG_VIRTIO)
	/*
	 * Make sure virtio bus is enumerated so that peripherals
	 * on the virtio bus can be discovered by their drivers
	 */
	virtio_init();
#endif

	return 0;
}

int board_late_init(void)
{
#if IS_ENABLED(CONFIG_SOVRA_ZCU102)
	/* initr_env() has already imported the saved environment, if valid. */
	static char *const menu_vars[] = {
		"bootmenu_0", "bootmenu_1", "bootmenu_2", "bootmenu_3",
		"bootmenu_4", "menucmd",
	};
	static char *const policy_vars[] = {
		"boot_targets", "bootcmd_qspi", "boot_distro", "distro_bootcmd",
	};
	static char *const current_script_vars[] = {
		"boot_selected", "boot_net", "boot_eth", "update_firmware",
		"boot_console",
	};
	static char *const feature_vars[] = {
		"net_boot_file", "net_dtb", "net_rootfs", "update_addr",
		"update_offset", "update_env_start", "update_spi_bus",
		"update_spi_cs", "update_spi_hz",
	};
	const char *value;
	int i;

	value = env_get("bootcmd");
	if (!value || !*value)
		env_set("bootcmd", "run boot_selected");
	value = env_get("bootmode");
	if (!value || !*value)
		env_set("bootmode", "distro_boot");

	for (i = 0; i < ARRAY_SIZE(policy_vars); i++) {
		value = env_get(policy_vars[i]);
		if (!value || !*value)
			env_set_default_vars(1, &policy_vars[i], 0);
	}
	env_set_default_vars(ARRAY_SIZE(current_script_vars),
			     current_script_vars, 0);
	for (i = 0; i < ARRAY_SIZE(feature_vars); i++) {
		value = env_get(feature_vars[i]);
		if (!value || !*value)
			env_set_default_vars(1, &feature_vars[i], 0);
	}

	value = env_get("distro_bootcmd");
	if (value && !strcmp(value,
		"for target in ${boot_targets}; do run bootcmd_${target}; done"))
		env_set("distro_bootcmd", "sovra_boot_targets");
	value = env_get("boot_distro");
	if (value && !strcmp(value,
		"echo Legacy distro boot: ${boot_targets}; setenv autoload yes; "
		"run distro_bootcmd; setenv autoload no"))
		env_set_default_vars(1, &policy_vars[2], 0);

	env_set("bootdelay", "3");
	env_set_default_vars(ARRAY_SIZE(menu_vars), menu_vars, 0);
	env_set("boot_standard", NULL);
#endif

	/* start usb so that usb keyboard can be used as input device */
	if (CONFIG_IS_ENABLED(USB_KEYBOARD))
		usb_init();

	return 0;
}

#ifdef CONFIG_SPL
u32 spl_boot_device(void)
{
	/* RISC-V QEMU only supports RAM as SPL boot device */
	return BOOT_DEVICE_RAM;
}
#endif

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* boot using first FIT config */
	return 0;
}
#endif

void *board_fdt_blob_setup(int *err)
{
	*err = 0;
	/* Stored the DTB address there during our init */
	return (void *)(ulong)gd->arch.firmware_fdt_addr;
}
