Secure Storage with i.MX 95 Verdin EVK using Trusted Keys with OP-TEE
Linux OP-TEE Security Tech Blog

In our previous blog post, we explored securing keys and certificates with Toradex’s recently launched i.MX 95 Verdin Evaluation Kit. We also demonstrated how to build and customise a Yocto reference image for the i.MX 95 Verdin EVK, leveraging OP-TEE and PKCS#11 which you can find here.

Another essential aspect of achieving a high level of security is encrypting data at rest. Without proper precautions, an attacker who gains physical access to the device, either through unauthorised access or theft, could attempt offline attacks, where the attacker tries to access and read the internal storage via another device.

A common mitigation is to encrypt the disk using Linux’s in-kernel key management in conjunction with a Trusted Platform Module (TPM). This is achieved by the TPM sealing (encrypting) the disk encryption key before storing it on disk. The key is unsealed (decrypted) and loaded into memory, typically during boot for a disk encryption tool like dm-crypt to encrypt and decrypt the disk. This prevents the key from being exposed in plaintext on disk. Upon system shutdown, the key is cleared from memory, requiring the TPM to unseal it again at the next boot. However, this approach requires additional hardware. The i.MX 95 provides an integrated solution by incorporating a Trusted Execution Environment (TEE)  (Arm TrustZone), creating a secure, isolated area within the processor. This secure environment operates alongside the main operating system to handle sensitive tasks such as cryptographic key management, secure boot and data encryption.

In this blog, we will explore how to build and customise a Yocto reference image for the i.MX 95 Verdin EVK, utilising dm-crypt and TEE-based Trusted Keys with OP-TEE for secure disk encryption. We will demonstrate how to create an encrypted data partition on an SD card using dm-crypt, securely store data within it, reload the encrypted volume after a reboot and successfully decrypt the data to verify its integrity.
Leveraging OP-TEE, Trusted Keys and DM-Crypt

OP-TEE is an open-source implementation of a TEE designed to comply with the GlobalPlatform TEE specifications. Leveraging Arm TrustZone, OP-TEE isolates sensitive operations, such as cryptographic functions, secure data management and authentication from the main operating system, providing a trusted environment for secure execution. OP-TEE uses a Hardware Unique Key (HUK) to seal and unseal the trusted keys. The HUK is a device-specific, hardware-embedded key that forms the foundation of OP-TEE’s security. It is typically configured via vendor tools, with its security implementation managed by the vendor. In this case, NXP’s i.MX 9 series uses the EdgeLock® secure enclave to securely manage the HUK.

The Trusted Keys Trusted Application (TA) runs inside the Trusted Execution Environment (TEE), providing secure, isolated execution of sensitive operations such as generating, sealing and unsealing keys, without being compromised by the non-secure operating system.

DM-Crypt is a Linux kernel module that provides transparent encryption for block devices, ensuring that data is automatically encrypted when written to the device and decrypted when read. Integrated with the Device Mapper framework, it enables the creation of virtual block device layers. As one of these layers, dm-crypt encrypts data before it reaches the underlying storage and decrypts it when accessed. It supports AES encryption with CBC, ESSIV, and XTS modes, offering robust security for disk encryption.

OP-TEE, in combination with the Trusted Keys TA and DM-Crypt, enables secure disk encryption by leveraging the Trusted Execution Environment (TEE) where OP-TEE provides an isolated environment, allowing the Trusted Keys TA to securely seal and unseal keys. The Trusted Keys TA derives its sealing and unsealing key from the device’s Hardware Unique Key (HUK), effectively binding the decryption process to the specific hardware. This ensures that only the original device can access the protected data. The sealing key is then used to seal (encrypt) the disk encryption key, which we then store to disk. This prevents the key from being exposed in plaintext on disk. DM-Crypt handles automatic encryption and decryption during read and write operations using the disk encryption key stored in memory. Upon system shutdown, the key is securely erased from memory, requiring the unsealing of the sealed key stored on disk using the Trusted Keys TA at the next boot.
Build i.MX 95 Verdin EVK Image

As of writing this, Toradex do not provide a reference image for the i.MX 95 Verdin Evaluation Kit for Yocto. Therefore, the example below uses NXP’s reference image instead.

Firstly, set up the manifest and populate the Yocto project layer sources with the following commands:

mkdir imx95-19x19-verdin-yocto
cd imx95-19x19-verdin-yocto
repo init -u https://github.com/nxp-imx/imx-manifest -b imx-linux-scarthgap -m imx-6.6.52-2.2.0.xml
repo sync

Configure the build for i.MX 95 Verdin EVK:

MACHINE=imx95-19x19-verdin DISTRO=fsl-imx-xwayland source ./imx-setup-release.sh -b bld-xwayland

Run the kernel’s menuconfig utility to enable TEE-based trusted keys and include support for Device Mapper and DM-Crypt:

bitbake -c menuconfig linux-imx

Enable Trusted key support by setting the following:

-> Security options
    -*- Enable access key retention support
    <*>   TRUSTED KEYS
    [ ]     TPM-based trusted keys
    [*]     TEE-based trusted keys
    < >   SECURE_KEYS
    <*>   ENCRYPTED KEYS

Enable Device Mapper and DM Crypt support by setting the following:

-> Device Drivers
    -> Multiple devices driver support (RAID and LVM)
        --- Multiple devices driver support (RAID and LVM)
        <*>   Device mapper support
        <*>     Crypt target support

Save the kernel configation and exit. In local.conf, or in the image recipe, add the following line:

IMAGE_INSTALL:append = " keyutils e2fsprogs-mke2fs util-linux"

This will include util-linux, which provides the blockdev utility for managing block devices; e2fsprogs-mke2fs, which provides mkfs.ext4 for creating ext4 file systems; and keyutils, which provides keyctl for managing encryption keys, to be used for performing disk encryption.

To add a data partition to the SD card, we will simply update the imx-imx-boot-bootpart.wks which can be found ${BUILDDIR}/../sources/meta-freescale/wic/imx-imx-boot-bootpart.wks.in from your build directory with the following:

part /data –ondisk mmcblk –fstype=ext4 –label data –size=500M

As can be seen below:

# short-description: Create SD card image with a boot partition
# long-description:
# Create an image that can be written onto a SD card using dd for use
# with i.MX SoC family
# It uses u-boot + other binaries gathered together on imx-boot file
#
# The disk layout used is:
# - ---------- -------------- --------------
# | | imx-boot |     boot     |    rootfs    |
# - ---------- -------------- --------------
# ^ ^          ^              ^              ^
# | |          |              |              |
# 0 |        8MiB          264MiB         264MiB + rootfs + IMAGE_EXTRA_SPACE (default 10MiB)
# ${IMX_BOOT_SEEK} 32 or 33kiB, see reference manual
#
part u-boot --source rawcopy --sourceparams="file=imx-boot.tagged" --ondisk mmcblk --no-table --align ${IMX_BOOT_SEEK}
part /boot --source bootimg-partition --ondisk mmcblk --fstype=vfat --label boot --active --align 8192 --size 256
part / --source rootfs --ondisk mmcblk --fstype=ext4 --label root --align 8192
part /data --ondisk mmcblk --fstype=ext4 --label data --size=500M
bootloader --ptable msdos

Note: Ideally, this .wks file would be created in your own custom layer and set using WKS_FILE variable in your local.conf or image recipe, but for simplicity we will modify the file directly.

Build the target image:

bitbake imx-image-core

Flash the image onto a SD card:

sudo bmaptool copy ${BUILDDIR}/tmp/deploy/images/imx95-19x19-verdin/core-image-minimal-imx95-19x19-verdin.rootfs.wic.zst /dev/sd<x>

Ensure the board is powered off by switching SW6 to the ‘OFF’ position. Insert the SD card, then configure the boot switch (SW2) to ‘1011’ for SD boot. Finally, power on the board by switching S6 to the ‘ON’ position.
Verify OP-TEE is installed and working

Once the board has booted up, log in as root (no password is required). To verify that the OP-TEE device driver has successfully loaded, run the following command:

root@imx95-19x19-verdin:~# dmesg | grep optee
 [    1.632146] optee: probing for conduit method.
 [    1.636410] optee: revision 4.4 (60beb308810f9561)
 [    1.636999] optee: dynamic shared memory is enabled
 [    1.646679] optee: initialized driver
root@imx95-19x19-verdin:~#

This output verifies that the OP-TEE driver has been successfully loaded. In this instance, the driver is operating with version 4.4.

To verify that the OP-TEE Trusted Keys TA is installed we can simply check that /lib/optee_armtz/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c.ta is present and to test that everything is functioning, we can run the OP-TEE using xtest as shown below:

root@imx95-19x19-verdin:~# xtest
 Run test suite with level=0
 TEE test application started over default TEE instance
 #
 #
 regression+pkcs11+regression_nxp
 #
 #
 regression_1001 Core self tests 
...
regression_nxp_1001 OK
 +-----------------------------------------------------
 45141 subtests of which 0 failed
 156 test cases of which 0 failed
 0 test cases were skipped
 TEE test application done!
 root@imx95-19x19-verdin:~#

Generate Trusted key

First, we need to generate a 32-byte (256-bit) random key in the kernel keyring. This key will later serve as the disk encryption key used by dm-crypt for encryption and decryption. As a TEE-based trusted key, the kernel will create it by interacting with the Trusted Key TA.

root@imx95-19x19-verdin:~# keyctl add trusted kmk "new 32" @s
1062158069
root@imx95-19x19-verdin:~#

To display the ASCII hex copy of the sealed key, we can print it out as follows:

root@imx95-19x19-verdin:~# keyctl print 1062158069
0062a222c6e1aa9931f6d0cbc7cb571dc68d6cf011088b3def98414c643b8189d3787504d5bfa2101a1ae9f07d4c86c92c8219aa5de16171b1df400b831b35e30a
root@imx95-19x19-verdin:~#

The generated key will not persist across reboots, as the Trusted Application (TA) stores it in secure RAM rather than persistent storage. To retain the key after a reboot, we must request the TA to seal the key and return an encrypted key blob that only the TA can decrypt. This sealed key blob is then stored in persistent storage on the filesystem, as follows:

root@imx95-19x19-verdin:~# keyctl pipe 1062158069 > kmk.blob
root@imx95-19x19-verdin:~#

Create Encrypted Disk

Next, we need to create an encrypted device named cryp_dev on /dev/mmcblk1p3 using dm-crypt with AES-CBC encryption. The encryption key used will be the trusted key kmk that we just created. We can create the encrypted disk using the following command:

root@imx95-19x19-verdin:~# dmsetup -v create cryp_dev --table "0 $(blockdev --getsz /dev/mmcblk1p3) crypt capi:cbc(aes)-plain :32:trusted:kmk 0 /dev/mmcblk1p3 0 1 sector_size:1024"
Name:              cryp_dev
State:             ACTIVE
Read Ahead:        256
Tables present:    LIVE
Open count:        0
Event number:      0
Major, minor:      253, 0
Number of targets: 1

root@imx95-19x19-verdin:~#

To displays the current device-mapper table along with the encryption keys used for each mapped device run:

root@imx95-19x19-verdin:~# dmsetup table --showkeys
cryp_dev: 0 1024000 crypt capi:cbc(aes)-plain :32:trusted:kmk 0 179:99 0 1 sector_size:1024
root@imx95-19x19-verdin:~#

When we can also list all block devices we can see cryp_dev device attached to mmcblk1p3

root@imx95-19x19-verdin:~# lsblk
NAME         MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
mtdblock0     31:0    0   128M  0 disk 
mmcblk0      179:0    0  58.3G  0 disk 
|-mmcblk0p1  179:1    0 332.8M  0 part 
`-mmcblk0p2  179:2    0   1.3G  0 part 
mmcblk0boot0 179:32   0     4M  1 disk 
mmcblk0boot1 179:64   0     4M  1 disk 
mmcblk1      179:96   0  29.8G  0 disk 
|-mmcblk1p1  179:97   0 332.8M  0 part 
|-mmcblk1p2  179:98   0   1.3G  0 part /
`-mmcblk1p3  179:99   0   500M  0 part 
  `-cryp_dev 253:0    0   500M  0 dm   
root@imx95-19x19-verdin:~#

Now, we need to format the encrypted device /dev/mapper/cryp_dev with the ext4 file system to store files on the device.

root@imx95-19x19-verdin:~# mkfs.ext4 /dev/mapper/cryp_dev
mke2fs 1.47.0 (5-Feb-2023)
Creating filesystem with 512000 1k blocks and 128016 inodes
Filesystem UUID: 62f4822a-e725-42d1-9dc3-ea9cf92c2d1d
Superblock backups stored on blocks: 
    8193, 24577, 40961, 57345, 73729, 204801, 221185, 401409

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (8192 blocks): done
Writing superblocks and filesystem accounting information: done 

root@imx95-19x19-verdin:~#

Create a mount point:

root@imx95-19x19-verdin:~# mkdir /mnt/cryp_dev
root@imx95-19x19-verdin:~#

Mount /dev/mapper/cryp_dev to /mnt/cryp_dev/ as follows:

root@imx95-19x19-verdin:~# mount -t ext4 /dev/mapper/cryp_dev /mnt/cryp_dev/
[  201.111667] EXT4-fs (dm-0): mounted filesystem 62f4822a-e725-42d1-9dc3-ea9cf92c2d1d r/w with ordered data mode. Quota mode: none.
root@imx95-19x19-verdin:~#

When we list the block devices, we can see that cryp_dev has been successfully mounted to /mnt/cryp_dev as shown below:

root@imx95-19x19-verdin:~# lsblk
NAME         MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
mtdblock0     31:0    0   128M  0 disk 
mmcblk0      179:0    0  58.3G  0 disk 
|-mmcblk0p1  179:1    0 332.8M  0 part 
`-mmcblk0p2  179:2    0   1.3G  0 part 
mmcblk0boot0 179:32   0     4M  1 disk 
mmcblk0boot1 179:64   0     4M  1 disk 
mmcblk1      179:96   0  29.8G  0 disk 
|-mmcblk1p1  179:97   0 332.8M  0 part 
|-mmcblk1p2  179:98   0   1.3G  0 part /
`-mmcblk1p3  179:99   0   500M  0 part 
  `-cryp_dev 253:0    0   500M  0 dm   /mnt/cryp_dev
root@imx95-19x19-verdin:~#

Write some sensitive data to a text file to read back later to verify everything works:

root@imx95-19x19-verdin:~# echo "My sensitive data" > /mnt/cryp_dev/README.txt
root@imx95-19x19-verdin:~#

Ensures all data in memory is written to disk:

root@imx95-19x19-verdin:~# sync
root@imx95-19x19-verdin:~# 

Reboot the board so we can confirm that we can reload the encrypted disk:

root@imx95-19x19-verdin:~# reboot
root@imx95-19x19-verdin:~# 

Note: To verify that the data is encrypted, you can shut down the device, remove the SD card, and plug it into your host PC. When you attempt to access the data partition, you should be unable to read its contents.
Reload Encrypted Disk

Load and unseal the previously exported sealed trusted key blob kmk.blob from the filesystem into the session keyring:

root@imx95-19x19-verdin:~# keyctl add trusted kmk "load `cat kmk.blob`" @s
76913048
root@imx95-19x19-verdin:~#

Re-create encrypted device. Using the trusted key kmk key:

root@imx95-19x19-verdin:~# dmsetup -v create cryp_dev --table "0 $(blockdev --getsz /dev/mmcblk1p3) crypt capi:cbc(aes)-plain :32:trusted:kmk 0 /dev/mmcblk1p3 0 1 sector_size:1024"
Name:              cryp_dev
State:             ACTIVE
Read Ahead:        256
Tables present:    LIVE
Open count:        0
Event number:      0
Major, minor:      253, 0
Number of targets: 1

root@imx95-19x19-verdin:~#

Re-mount the encrypted device /dev/mapper/cryp_dev as an ext4 filesystem to the directory /mnt/cryp_dev/

root@imx95-19x19-verdin:~# mount -t ext4 /dev/mapper/cryp_dev /mnt/cryp_dev/
[   55.899399] EXT4-fs (dm-0): mounted filesystem 62f4822a-e725-42d1-9dc3-ea9cf92c2d1d r/w with ordered data mode. Quota mode: none.
root@imx95-19x19-verdin:~#

To confirm that the disk has been successfully decrypted, we can verify the contents of the README.txt by simply printing the contents as follows:

root@imx95-19x19-verdin:~# cat /mnt/cryp_dev/README.txt
My sensitive data
root@imx95-19x19-verdin:~#

Conclusion

Many embedded systems store sensitive or proprietary information, such as user data, configuration settings and intellectual property. Without proper protection, an attacker who gains physical access to the device, whether through unauthorised access or theft could attempt offline attacks in an attempt to read the storage using another device.

In this blog, we demonstrated how to build and customise a Yocto reference image for the i.MX 95 Verdin EVK with disk encryption support and Trusted Keys. We walked through the process of encrypting a disk using dm-crypt, leveraging trusted keys with OP-TEE as a secure backend. This ensures that only the device itself can access the encrypted data, as the decryption key is securely stored within the Trusted Execution Environment (TEE) which binds it to the device hardware, thus effectively mitigating this attack vector.

An alternative to TEE-based trusted keys, which we did not cover in this blog, is NXP’s Cryptographic Acceleration and Assurance Module (CAAM). CAAM enhances security through dedicated hardware protection and improves performance with cryptographic acceleration. While CAAM offers improved security for supported hardware, TEE-based trusted keys provide a more platform-agnostic solution.
Security Considerations

It’s important to note that security is only as secure as the underlying hardware and software configuration. Here are a few key considerations:

As previously mentioned the Hardware Unique Key (HUK) is a critical component of OP-TEE, as it serves as a non-extractable, device-specific key from which other cryptographic keys are derived. OP-TEE provides a default HUK value of all zeros as a stub for vendors to implement platform-specific HUK support. Therefore, it is crucial to choose a vendor that not only supports this feature but also implements a secure and reliable method for generating and protecting the HUK within the hardware such as i.MX 95 EdgeLock secure enclave.

Secure boot and chain of trust must be implemented to ensure that only authenticated and trusted software is executed on the device. This ensures that any unauthorised or tampered software is prevented from running, protecting the integrity of the Trusted Execution Environment (TEE) and the overall system.

Trusted Applications (TAs) are signed to ensure their authenticity and integrity. By default, OP-TEE OS includes a keypair (keys/default_ta.pem), with the public key being compiled into the OS to validate TA signatures during loading. This default keypair should be replaced with a custom one, ensuring the private key remains securely stored offline.

If you’re interested in learning more about file system encryption, check out this blog post in which we explore file encryption on the Jetson Nano.
