BBB Iot Buster Debian
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
