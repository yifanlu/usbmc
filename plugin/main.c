/*
	VitaShell
	Copyright (C) 2015-2017, TheFloW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

#define MOUNT_POINT_ID 0x800

const char check_patch[] = {0x01, 0x20, 0x01, 0x20};

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

typedef struct {
	const char *dev;
	const char *dev2;
	const char *blkdev;
	const char *blkdev2;
	int id;
} SceIoDevice;

typedef struct {
	int id;
	const char *dev_unix;
	int unk;
	int dev_major;
	int dev_minor;
	const char *dev_filesystem;
	int unk2;
	SceIoDevice *dev;
	int unk3;
	SceIoDevice *dev2;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
} SceIoMountPoint;

static SceIoDevice uma_ux0_dev = { "ux0:", "exfatux0", "sdstor0:uma-pp-act-a", "sdstor0:uma-lp-act-entire", MOUNT_POINT_ID };

static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

static SceIoDevice *ori_dev = NULL, *ori_dev2 = NULL;

static SceUID hookid = -1;

static tai_hook_ref_t ksceSysrootIsSafeModeRef;

static int ksceSysrootIsSafeModePatched() {
	return 1;
}

static int exists(const char *path) {
	int fd = ksceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	ksceIoClose(fd);
	return 1;
}

static void io_remount(int id) {
	ksceIoUmount(id, 0, 0, 0);
	ksceIoUmount(id, 1, 0, 0);
	ksceIoMount(id, NULL, 0, 0, 0, 0);
}

int shellKernelIsUx0Redirected() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount) {
		return -1;
	}

	if (mount->dev == &uma_ux0_dev && mount->dev2 == &uma_ux0_dev) {
		return 1;
	}

	return 0;
}

int shellKernelRedirectUx0() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount) {
		return -1;
	}

	if (mount->dev != &uma_ux0_dev && mount->dev2 != &uma_ux0_dev) {
		ori_dev = mount->dev;
		ori_dev2 = mount->dev2;
	}

	mount->dev = &uma_ux0_dev;
	mount->dev2 = &uma_ux0_dev;

	return 0;
}

int shellKernelUnredirectUx0() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount) {
		return -1;
	}

	if (ori_dev && ori_dev2) {
		mount->dev = ori_dev;
		mount->dev2 = ori_dev2;

		ori_dev = NULL;
		ori_dev2 = NULL;
	}

	return 0;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	int (* _ksceKernelMountBootfs)(const char *bootImagePath);
	int (* _ksceKernelUmountBootfs)(void);
	SceUID tmp1, tmp2;
	int ret;

	// Get tai module info
	tai_module_info_t info;
	info.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &info) < 0)
		return SCE_KERNEL_START_NO_RESIDENT;

	// Get important function
	switch (info.module_nid) {
		case 0x9642948C: // 3.60 retail
			module_get_offset(KERNEL_PID, info.modid, 0, 0x138C1, (uintptr_t *)&sceIoFindMountPoint);
			break;

		case 0xA96ACE9D: // 3.65 retail
		case 0x3347A95F: // 3.67 retail
			module_get_offset(KERNEL_PID, info.modid, 0, 0x182F5, (uintptr_t *)&sceIoFindMountPoint);
			break;

		default:
			return SCE_KERNEL_START_NO_RESIDENT;
	}

	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x01360661, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
		ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0x185FF1BC, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
			return SCE_KERNEL_START_NO_RESIDENT;

	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x9C838A6B, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
		ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xBD61AD4D, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
			return SCE_KERNEL_START_NO_RESIDENT;

	// Load SceUsbMass

	// First try loading from bootimage
	SceUID modid;
	if (_ksceKernelMountBootfs("os0:kd/bootimage.skprx") >= 0) {
		modid = ksceKernelLoadModule("os0:kd/umass.skprx", 0x800, NULL);
		_ksceKernelUmountBootfs();
	} else {
		// try loading from VitaShell
		modid = ksceKernelLoadModule("ux0:VitaShell/module/umass.skprx", 0, NULL);
	}

	// Hook module_start
	// FIXME: add support to taihen so we don't need to hard code this address
	tmp1 = taiInjectDataForKernel(KERNEL_PID, modid, 0, 0x1546, check_patch, sizeof(check_patch));
	tmp2 = taiInjectDataForKernel(KERNEL_PID, modid, 0, 0x154c, check_patch, sizeof(check_patch));

	if (modid >= 0) ret = ksceKernelStartModule(modid, 0, NULL, 0, NULL, NULL); 
	else ret = modid;

	if (tmp1 >= 0) taiInjectReleaseForKernel(tmp1);
	if (tmp2 >= 0) taiInjectReleaseForKernel(tmp2);

	// Check result
	if (ret < 0)
		return SCE_KERNEL_START_NO_RESIDENT;

	// Fake safe mode in SceUsbServ
	hookid = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSysrootIsSafeModeRef, "SceUsbServ", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeModePatched);

	if (exists("sdstor0:xmc-lp-ign-userext") || shellKernelIsUx0Redirected()) {
		return SCE_KERNEL_START_SUCCESS;
	}

	// wait ~5 second max for USB to be detected
	// this may look bad but the Vita does this to detect ux0 so ¯\_(ツ)_/¯
	for (int i = 0; i < 26; i++) {
		// try to detect USB plugin 25 times for 0.2s each
		if (exists("sdstor0:uma-lp-act-entire")) {
			shellKernelRedirectUx0();
			io_remount(MOUNT_POINT_ID);
			break;
		}
		ksceKernelDelayThread(200000);
	}

	// load taiHEN plugins on this new memory stick
	if (exists("ux0:tai/config.txt")) {
		taiReloadConfigForKernel(1, 1);
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	if (hookid >= 0)
		taiHookReleaseForKernel(hookid, ksceSysrootIsSafeModeRef);

	return SCE_KERNEL_STOP_SUCCESS;
}
