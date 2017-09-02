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
#include <psp2kern/ctrl.h> 

#include <stdio.h>
#include <string.h>

#include <taihen.h>

// Constants
#define MOUNT_POINT_UX0_ID  0x800
#define MOUNT_POINT_UX0_NAME "ux0:"
#define MOUNT_POINT_UX0_NAME_2 "exfatux0"
#define MOUNT_POINT_UMA0_ID 0xF00
#define MOUNT_POINT_UMA0_NAME "uma0:"
#define MOUNT_POINT_UMA0_NAME_2 "exfatuma0"

#define BLOCK_DEVICE_XMC "sdstor0:xmc-lp-ign-userext"
#define BLOCK_DEVICE_UMA "sdstor0:uma-pp-act-a"
#define BLOCK_DEVICE_UMA_2 "sdstor0:uma-lp-act-entire"

#define SWAP_BUTTON SCE_CTRL_SQUARE

const char check_patch[] = { 0x01, 0x20, 0x01, 0x20 };

// External functions
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

// Structs
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

// Static object definitions
static SceIoDevice usbToUx0Device  = { MOUNT_POINT_UX0_NAME,  MOUNT_POINT_UX0_NAME_2,  BLOCK_DEVICE_UMA, BLOCK_DEVICE_UMA_2, MOUNT_POINT_UX0_ID  };
static SceIoDevice usbToUma0Device = { MOUNT_POINT_UMA0_NAME, MOUNT_POINT_UMA0_NAME_2, BLOCK_DEVICE_UMA, BLOCK_DEVICE_UMA_2, MOUNT_POINT_UMA0_ID };
static SceIoDevice xmcToUma0Device = { MOUNT_POINT_UMA0_NAME, MOUNT_POINT_UMA0_NAME_2, BLOCK_DEVICE_XMC, BLOCK_DEVICE_XMC,   MOUNT_POINT_UMA0_ID };

// External functions (not loaded yet)
static SceIoMountPoint *(*sceIoFindMountPoint)(int id) = NULL;

// Static variables
static SceUID safeModeHookId = -1;
static tai_hook_ref_t ksceSysrootIsSafeModeRef;

// Functions
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

int shellKernelRedirect(SceIoDevice *device) {
  SceIoMountPoint *mount = sceIoFindMountPoint(device->id);
  if (!mount)
    return -1;

  mount->dev = device;
  mount->dev2 = device;

  io_remount(device->id);

  return 0;
}

int importFindMountPointFunction() {
  // Get tai module info
  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &info) < 0)
    return -1;

  // Get important function
  module_get_offset(KERNEL_PID, info.modid, 0, 0x138C1, (uintptr_t *)&sceIoFindMountPoint);

  return 0;
}

SceUID loadSceUsbMassModule() {
  SceUID moduleId;
  if (ksceKernelMountBootfs("os0:kd/bootimage.skprx") >= 0) { // Try loading from bootimage
    moduleId = ksceKernelLoadModule("os0:kd/umass.skprx", 0x800, NULL);
    ksceKernelUmountBootfs();
  } else // Try loading from VitaShell
    moduleId = ksceKernelLoadModule("ux0:VitaShell/module/umass.skprx", 0, NULL);

  return moduleId;
}

int loadAndStartSceUsbMassModule() {
  SceUID sceUsbMassModuleId = loadSceUsbMassModule();
  
  // Hook module_start
  // FIXME: add support to taiHEN so we don't need to hard code this address
  SceUID taiPatchReference1546 = taiInjectDataForKernel(KERNEL_PID, sceUsbMassModuleId, 0, 0x1546, check_patch, sizeof(check_patch));
  SceUID taiPatchReference154c = taiInjectDataForKernel(KERNEL_PID, sceUsbMassModuleId, 0, 0x154c, check_patch, sizeof(check_patch));

  int status;
  if (sceUsbMassModuleId >= 0)
    status = ksceKernelStartModule(sceUsbMassModuleId, 0, NULL, 0, NULL, NULL);
  else
    status = sceUsbMassModuleId;

  if (taiPatchReference1546 >= 0)
    taiInjectReleaseForKernel(taiPatchReference1546);
  if (taiPatchReference154c >= 0)
    taiInjectReleaseForKernel(taiPatchReference154c);

  return status;
}

int isSwapKeyPressed() {
  ksceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);
  
  SceCtrlData ctrl;
  ksceCtrlPeekBufferPositive(0, &ctrl, 1);

  return ctrl.buttons & SWAP_BUTTON;
}

int isUsbDeviceAvailable() {
  // Wait ~5 second max. for USB device to be detected
  // This may look bad but the Vita does this to detect XMC so ¯\_(ツ)_/¯
  for (int i = 0; i < 26; i++) {
    if (exists(BLOCK_DEVICE_UMA_2))
      return 1;
    ksceKernelDelayThread(200000);
  }

  return 0;
}

// Main module function
void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  int importStatus = importFindMountPointFunction();
  if (importStatus < 0)
    return SCE_KERNEL_START_NO_RESIDENT;

  int loadStatus = loadAndStartSceUsbMassModule();
  if (loadStatus < 0)
    return SCE_KERNEL_START_NO_RESIDENT;

  // Fake safe mode in SceUsbServ
  safeModeHookId = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSysrootIsSafeModeRef, "SceUsbServ", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeModePatched);

  if (isUsbDeviceAvailable()) {
    if (isSwapKeyPressed()) {
      // Mount USB device at uma0
      shellKernelRedirect(&usbToUma0Device);
    } else {
      // Mount USB device at ux0
      shellKernelRedirect(&usbToUx0Device);
  
      // Mount XMC device at uma0
      shellKernelRedirect(&xmcToUma0Device);
    }
  }

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (safeModeHookId >= 0)
    taiHookReleaseForKernel(safeModeHookId, ksceSysrootIsSafeModeRef);
  return SCE_KERNEL_STOP_SUCCESS;
}