usbmc
=====
This is VitaShell's patches for USB storage support as a standalone plugin. If 
loaded on startup (before SceShell), it will automatically mount the USB 
storage as `ux0` instead of the memory card or internal memory.

Full credits to The_FloW for the patches.

Note: When using USB storage as memory, make sure all taiHEN plugins are 
installed to `ur0:` with `ur0:tai/config.txt` as the config file. This is 
because taiHEN is loaded _before_ usbmc so you cannot use taiHEN plugins 
installed to USB.

## Installation

1. Make sure your USB storage drive is formatted to a single FAT, FAT32, or 
exFAT partition using the MBR partition scheme.
2. Install the vpk
3. Run usbmc to start the installer
4. Press X to install the plugin.
5. Reboot

Now you can use the USB storage as extra storage. Continue reading if you want 
to use it in place of your memory card.

1. Open the usbmc installer again.
2. Press X to start the memory card installation.
3. Choose to either copy VitaShell/molecularShell to the USB storage or copy 
**everything** from your memory card to the USB storage, replacing any files 
already on there.
4. Once the copying is complete, press X to shut down the Vita.
5. Remove your old memory card to start using your USB storage as a memory card.

## Memory Card Priority

1. Vita memory card will be used if inserted.
2. USB storage will be used if inserted and formatted correctly.
3. Internal memory will be used if it exists (PSTV and PCH-2000).
