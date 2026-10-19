// SPDX-License-Identifier: GPL-2.0+
/* Ordered legacy distro targets with a per-attempt failure report. */
#include <command.h>
#include <env.h>
#include <malloc.h>
#include <mmc.h>
#include <net.h>
#include <vsprintf.h>
#include <linux/ctype.h>
#include <linux/list.h>

struct boot_failure {
	struct list_head list;
	char *target;
	char *reason;
	int status;
};

/* Only preflight the built-in scripts; custom bootcmd_* remain authoritative. */
static const char *target_preflight(const char *target, const char *cmd)
{
	const char *def;
	struct mmc *mmc;

	if (!strcmp(target, "mmc0")) {
		def = env_get_default("bootcmd_mmc0");
		if (!def || strcmp(cmd, def))
			return NULL;
		mmc = find_mmc_device(0);
		if (!mmc)
			return "MMC0 controller/device unavailable";
		if (mmc_init(mmc))
			return "MMC0 card initialization failed";
	} else if (!strcmp(target, "dhcp")) {
		char *buf = malloc(strlen(cmd) + 1);
		int len;

		if (!buf)
			return "out of memory comparing DHCP policy";
		len = env_get_default_into("bootcmd_dhcp", buf, strlen(cmd) + 1);
		if (len > 0 && !strcmp(cmd, buf) && !eth_get_dev()) {
			free(buf);
			return "Ethernet interface unavailable";
		}
		free(buf);
	}
	return NULL;
}

static const char *returned_reason(const char *target)
{
	const char *reason = env_get("boot_errmsg");

	if (reason && *reason)
		return reason;
	if (!strcmp(target, "mmc0"))
		return "MMC scan returned without booting; "
		       "check partition, filesystem, extlinux/script/EFI log";
	if (!strcmp(target, "dhcp"))
		return "network boot returned; check DHCP, TFTP and script/EFI log";
	return "boot command returned without handing control to an OS";
}

static int do_sovra_boot_targets(struct cmd_tbl *cmdtp, int flag, int argc,
				 char *const argv[])
{
	static bool scanning;
	LIST_HEAD(failures);
	struct boot_failure *failure, *next;
	const char *configured = env_get("boot_targets"), *cmd, *reason;
	char *targets, *cursor, *target, *p, *name;
	bool valid;

	if (scanning) {
		env_set("boot_errmsg", "recursive distro boot is not allowed");
		return CMD_RET_FAILURE;
	}
	if (!configured || !*configured) {
		puts("[SOVRA BOOT] ALL FAILED: boot_targets is empty\n");
		return CMD_RET_FAILURE;
	}
	targets = strdup(configured);
	if (!targets)
		return CMD_RET_FAILURE;
	scanning = true;
	cursor = targets;
	while ((target = strsep(&cursor, " \t\r\n"))) {
		if (!*target)
			continue;
		failure = calloc(1, sizeof(*failure));
		name = malloc(strlen(target) + sizeof("bootcmd_"));
		if (!failure || !name) {
			free(failure);
			free(name);
			puts("[SOVRA BOOT] scan stopped: out of memory\n");
			goto out;
		}
		failure->target = target;
		list_add_tail(&failure->list, &failures);
		sprintf(name, "bootcmd_%s", target);
		valid = true;
		for (p = target; *p; p++)
			if (!isalnum((unsigned char)*p) && *p != '_')
				valid = false;
		printf("[SOVRA BOOT] Trying %s\n", target);
		env_set("boot_errmsg", NULL);
		cmd = valid ? env_get(name) : NULL;
		reason = !valid ? "invalid target name" :
			 !cmd || !*cmd ? "bootcmd_<target> is missing or empty" :
			 target_preflight(target, cmd);
		failure->status = CMD_RET_FAILURE;
		if (!reason) {
			failure->status = run_commandf("run %s", name);
			/* A successful OS handoff never returns, even if rc is 0. */
			reason = returned_reason(target);
		}
		failure->reason = strdup(reason);
		free(name);
		printf("[SOVRA BOOT] FAIL %s: %s (rc=%d)\n", target,
		       failure->reason ?: "cannot allocate failure reason",
		       failure->status);
	}
	puts("[SOVRA BOOT] ALL FAILED: no configured target booted\n");
	list_for_each_entry(failure, &failures, list)
		printf("  %s: %s (rc=%d)\n", failure->target,
		       failure->reason ?: "cannot allocate failure reason",
		       failure->status);
out:
	list_for_each_entry_safe(failure, next, &failures, list) {
		list_del(&failure->list);
		free(failure->reason);
		free(failure);
	}
	free(targets);
	scanning = false;
	return CMD_RET_FAILURE;
}

U_BOOT_CMD(sovra_boot_targets, 1, 0, do_sovra_boot_targets,
	   "try boot_targets in order and report each failure", "");
