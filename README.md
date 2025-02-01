### reverse shell
##### have a sercer abc.com
##### have a device running linux behind a router localhost
### on device under user device_user:
```
device_user$     ssh -R localhost:7070:localhost:22 abc_user@abc.com
```

### on abc.com
```
abs-user$     ssh -p 7070 device_user@localhost
```



### BBB Iot Buster Debian
activate HDMI

bone$ cd /var/lib/cloud9/displays
bone$ git pull
bone$ ./LCD-on.sh  -- did not work
sudo /opt/scripts/tools/update_kernel.sh -- nexy sollution

### OS BOOT MANAGER (UEFI) - ubuntu stuck in grub prompt

##### sollution worked for me
```
1462  [ -d /sys/firmware/efi ] && echo "Installed in UEFI mode" || echo "Installed in Legacy mode"
      Installed in UEFI mode
 1463  apt install --reinstall grub-efi-amd64 linux-generic linux-headers-generic
 1464  sudo apt install --reinstall grub-efi-amd64 linux-generic linux-headers-generic
 1465  sudo update-initramfs -c -k all
 1466  sudo update-grub
 1467  sudo reboot
 ```

#### YOCTO PATCH
```
git add .
git commit -a -m "sd"
git format-patch -1


```
#### M4 ENABLE FOR IMX8MM

* if you have your imx8mm_yourboard.dts
   *  include it at the top of the   imx8mm-evk-rpmsg.dts and replace current include *.dts
 
```
// SPDX-License-Identifier: (GPL-2.0 OR MIT)
/*
 * Copyright 2019 NXP
 */

/dts-v1/;

#include "mx8mm_yourboard.dts"


/ {
	reserved-memory {
		#address-cells = <2>;
		#size-cells = <2>;
		ranges;

		m4_reserved: m4@0x80000000 {
			no-map;
			reg = <0 0x80000000 0 0x1000000>;
		};

```
   * and use imx8mm-evk-rpmsg.dts.dtb instead

#### or in your mx8mm_yourboard.dts file include the in the last line

```
&flexspi {
	status = "disabled";
};

#include "imx8mm-evk-rpmsg.dts"
```
    * and kep using your dtb

Boot up and

```
root@imx8mm-marius:/sys/class/remoteproc/remoteproc0# tree
.
|-- coredump
|-- device -> ../../../imx8mm-cm4
|-- firmware
|-- name
|-- power
|   |-- autosuspend_delay_ms
|   |-- control
|   |-- runtime_active_time
|   |-- runtime_status
|   `-- runtime_suspended_time
|-- recovery
|-- state
|-- subsystem -> ../../../../../class/remoteproc
`-- uevent

```    


### uboot utils
https://docs.embeddedts.com/U-boot_commands

```
ums x mmc y   < freescale
load mmc 0:1 ${loadaddr} /boot/uImage

load mmc 0:1 ${fdtaddr} /boot/imx6q-ts4900.dtb
fdt addr ${fdtaddr}
fdt print

load mmc 0:1 ${loadaddr} /boot/custombinary
go ${loadaddr}

dhcp
env set serverip 192.168.0.11
nfs ${loadaddr} 192.168.0.11:/path/to/somefile


load mmc 0:1 ${loadaddr} /boot/ubootscript
source ${loadaddr}



```



