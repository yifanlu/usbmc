usbmc (ux0+uma0 mod)
=====
This is VitaShell's patches for USB storage support as a standalone plugin. If 
loaded on startup (before SceShell), it will automatically mount the USB 
storage as `ux0` instead of the memory card or internal memory. The memory card 
or internal memory will then be mounted to `uma0` by default.

Full credits to The_FloW for the patches and Yifan Lu for `usbmc`.

**This is NOT for gamecard-to-microSD adapters! For that, you want to check 
out [this](https://github.com/xyzz/gamecard-microsd)**.

Note: When using USB storage as memory, make sure all taiHEN plugins are 
installed to `ur0:` with `ur0:tai/config.txt` as the config file. This is 
because taiHEN is loaded _before_ usbmc so you cannot use taiHEN plugins 
installed to USB.

## Installation

1. Download the [latest pre-built release](https://github.com/M1lk4h0l1c/usbmc/releases/latest) or compile the 
source yourself.
1. Make sure your USB storage drive is formatted to a single FAT, FAT32, or 
exFAT partition using the MBR partition scheme.
1. Install the `usbmc_installer.vpk`.
1. Run `usbmc` to start the installer.
1. Press _CROSS_ to install the plugin.
1. Reboot

Now you can use the USB storage as a replacement for your memory card (or internal 
memory) at `ux0`. Continue reading if you want to transfer the content of your 
memory card (or internal memory).

1. Start your Vita with _SQUARE_ while holding down _SQUARE_ to swap mount points.
1. Open the `usbmc` installer again.
1. Press _CROSS_ to start the memory card installation (`Install USB as memory card.`).
1. Choose to either copy VitaShell/molecularShell to the USB storage or copy 
**everything** from your memory card to the USB storage, replacing any files 
already on there.
1. Once the copying is complete, press X to shut down the Vita.

## Usage

* On boot, the USB device will be mounted at `ux0` and the memory card (or 
internal memory) at `uma0`.
* If _SQUARE_ is being held down on boot, the mount points will 
be swapped for this session. Reboot to go back to normal.
* If no USB is found within a few seconds, the memory card (or internal 
memory) is mounted at `ux0`.

## Uninstallation

1. Remove the USB storage or start with swapped mount points (see _Usage_ section).
1. Install the `usbmc_installer.vpk` if necessary.
1. Launch the installer and use the _TRIANGLE_ option to uninstall the plugin.

Note you cannot uninstall usbmc while it is in use (duh).

### Disclaimer

I can only test this modified version with a PSVSD v2.1 on a PCH-100x (PS Vita OLED 3G).
Therefore, I don't know if this functions properly when used on a PSTV. Sorry.
