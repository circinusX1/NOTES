# QT creartor Step into so libraries source while debugging

*  QT -> View Views Debugger Log, Command
    * set solib-search-path \n:/path_to_where_the_so_lib_is_located

# gdb with coredump
* gdb -iex 'set sysroot /' application coredump
* thread apply all bt




# nano arduino programmer
USBAsp


# make OS RO
    echo u > /proc/sysrq-trigger

## dropbear to ssh key

#### on dropbear machine
```
cd user/.ssh
dropbearkey -t rsa -f ./id_dropbear # < this is the private key

dropbearkey -y -f ./id_dropbear | grep "^ssh-rsa " > authorized_keys_fragment # take this fragment
									      # to a server whenre to access


```



### reverse shell
##### have a sercer abc.com
##### have a device running linux behind a router localhost
### on device under user device_user:
```
device_user$     ssh -R localhost:7070:localhost:22 abc_user@abc.com
```

### on abc.com
```
abc-user$     ssh -p 7070 device_user@localhost
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

### qemu on ubuntu

```
sudo apt install --reinstall qemu-kvm qemu-utils libvirt-daemon-system libvirt-clients bridge-utils virt-manager

sudo systemctl enable --now libvirtd
sudo systemctl restart libvirtd

```
https://software.download.prss.microsoft.com/dbazure/Win11_24H2_English_x64.iso?t=718612c7-28e0-4d8b-8117-a18ce264eb5a&P1=1758204396&P2=601&P3=2&P4=oy7lCkt%2blNr%2fzSASBKzA6QnrnWJ8DXxAs13JQbHOdxT2eySoLeEg9yXgkucgnQ5wJF2dKkdYXQTEj8aPCFC%2fG503x0sGwHneSBSHPZqSPZYsiETvHmMafmqXmYdumjF49neZPSaUulm43me2rj5cIyjuo6Ib84rD93%2fdLl8cosrWiBdccqc0YFH2ifXMYg%2fjeBdd7H4zwTz%2f%2f8zlFsL%2buRP%2f43bo3lEZsYl0BTr0RSRfHEQA6xtKC%2bHN9TJedxP7nVKupFDQ%2bRaJuHviNgOOXTYByI%2fUElLyB51gHhHlcqtUm0TNBj%2fJG3iM34NuyhcyIZjNSy7iwmIZu46kAcVxFg%3d%3d

#### disable exporation
* serices client certificate disable
* adming cmd
    * slmgr -rearm 
```
#### display
* on guest windows
* https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.285-1/virtio-win-0.1.285.iso
    * mount iso
    * Control panel -> Display -> update driver -> select mounted sio drive usually E:
* Restart Guest WINdows from power button menu while holding Shift Key
* Should boot with TROUBLESHOOTING OPTION
   * Troubleshoot->Advanced Options->Startup Settings
   * Select 7 'Disable driver signature enforcement' 
