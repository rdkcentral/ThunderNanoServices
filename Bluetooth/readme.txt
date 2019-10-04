Serial Console and Bluetooth on RaspberryPi:
----------------------------------------------------------------------------------------------
If you would like to enable bluetooth and a serial terminal on the pi, make sure the following
entries are enables in the following RPI files:

/boot/config.txt
enable_uart=1
core_freq=250

Note: /boot/config.txt must not include the following line:
dtoverlay=pi3-miniuart-bt

/boot/cmdline.txt
... console=ttyS0,115200

/etc/inittab:
ttyS0::respawn:/sbin/getty -L ttyS0 115200 vt100 # GENERIC_SERIAL


Implementation notes
--------------------
- during pairing discovery needs to be disabled and, if needed, re-enabled afterwards
