https://www.thegoodpenguin.co.uk/blog/secure-storage-with-i-mx-95-verdin-evk-using-trusted-keys-with-op-tee/


1. Article purpose↑

This article shows the steps to execute the dynamic file encryption on block device storage (like SD card, eMMC) with Linux®. It shows how to use "dmsetup" and "keyutils" tools in Linux® user space.

The encrypting key can be protected. It means that the encrypting key is wrapped by a derived key from the internal Hardware Unique Key (HUK) hidden in the SAES internal peripheral. The wrapped value is usually stored in a blob in the filesystem. Unique key brings the advantage to prevent offline decryption of a storage volume. Furthermore, the SAES internal peripheral is tamper protected, meaning if a reboot happens after an intrusion (tamper event), the HUK is no more accessible, so the encrypting key can no more be unwrapped from the blob and the storage volume cannot be decrypted anymore.
2. Prerequisites↑

You are already familiar with the Distribution package or with the Developer package and OpenSTLinux distribution.
3. Introduction↑

Below you will find the process to encrypt dynamically a block device, here an SD card partition, with "dm-crypt"[1]. We exercise the Linux® in-kernel key management to create a Linux® "trusted key" key type as the block device-encrypting key. A "trusted key" is an encrypting key that is generated and sealed (encrypted/wrapped) & unsealed (decrypted/unwrapped) in the secure OP-TEE environment (with the Trusted Key TA). The wrapping key used to seal & unseal the encrypting key is a DHUK (Derived Hardware Unique Key) computed internally with the SAES peripheral from a secret key HUK (Hardware Unique Key). The encrypting key is generated with the RNG peripheral.

The Linux® user-space application, "keyctl"[2], manages the keyrings. It controls the key blobs containing the wrapped encrypting key that can be stored in the filesystem on flash memory.

The encrypting key is used in plain (unencrypted) and wrapped formats inside the Kernel keyrings. Considering the security, if the Linux® Kernel gets compromised due to a vulnerability, the disk encryption key can be exposed since it appears in plain inside Kernel space. In shared key mode (see 6.3), an improvement of the default linux framework allows to completely hide the key from the linux nonsecure context.

dm-crypt for disk encryption uses AES encryption algorithm with CBC or ESSIV or XTS mode. ESSIV and XTS mode provide more protection for disk encryption[3] but these modes are not performed by the CRYP internal peripheral. The AES CBC encryption can be accelerated with the CRYP internal peripheral.
4. Architecture overview↑
Alternate text
dm_crypt

The Hardware Unique Key (Hardware Unique Key overview) is a symmetric encryption key stored in the OTP of the platform. Each chip produced has a unique HUK. The Hardware Unique Key (HUK) is secret and is only accessible internally in the SAES internal peripheral. The HUK is provisioned in ST Manufacturing.
4.1. DM-Crypt overview↑

DM-Crypt[4] is a kernel module that makes data encryption in the storage devices. DM-Crypt module is interleaved in the kernel Device Mapper framework that allows developers to implement many use cases over different block device storage type (disk, SD card, eMMC memories). DM-Crypt uses the kernel Crypto API that uses the ST Crypto hardware internal peripheral (see Crypto API overview).
4.2. Linux Trusted Key with OP-TEE overview↑

Refer to slide 13 of "Linaro connect TEE based Trusted Keys in Linux presentation"[5], or the Video associated to Linaro presentation"[6].
5. Configuration↑
5.1. Kernel configuration↑

Depending on the developer package or the distribution package you are using see menuconfig to configure the kernel as follow: (See also: Menuconfig or how to configure kernel).


5.1.1. Enable crypto using ST CRYP hardware accelerator and AES algorithm↑

 Cryptographic API--- >
   -- Hardware crypto devices
     --- Hardware crypto devices                                  
            <*>   Support for STM32 crc accelerators
             ...
             {M} Support for amlogic cryptographic offloader NEW
             ...
            -*-   Support for Aspeed cryptographic engine driver
            <*>   Support for Aspeed Crypto debug messages
            <*>   Support for Aspeed Hash Crypto Engine HACE hash
            <*>   Support for Aspeed Hash Crypto Engine HACE crypto

5.1.2. Disable crypto software↑

 --- ARM Accelerated Cryptographic Algorithms                      
          {M}   Hash functions SHA 1 NEW
          <M> Hash functions SHA 1 neon
          <M>  Hash functions SHA 1 Armv8 crypto extensions
          <M>  Hash functions SHA 224 and SHA 256 Armv8 crypto extensions
          {M}    Hash functions SHA 224 and SHA 256 neon NEW
          <M>  Hash functions SHA 384 and SHA 512 neon
          < >Ciphers :  AES                                   
          < >Ciphers :  AES modes ECB CBC CTR XTS bit sliced NEON                                   
          < >Ciphers :  AES modes ECB CBC CTR XTS bit sliced ARMv8 crypto extension                                   
          < >Ciphers :  Chacha20 xChacha20 xChaCha12 NEON                                   
          <M> CRC32C CRC32

5.1.3. Enable the support of the device mapper and the crypt target↑

Device Drivers
 < > Parallel port support  --->
    -*- Plug and Play support  --->                        
    [*] Block devices  --->                                            
        NVME Support  --->                                             
        Misc devices  --->                                              
        SCSI device support  --->                                       
    <*> Serial ATA and Parallel ATA drivers (libata)  --->              
    [*] Multiple devices driver support (RAID and LVM)  ---> 
    [*] Multiple devices driver support (RAID and LVM)  ---> 
            < > RAID support
            < > Block device as cache
            <*> Device mapper support 
            [ ] Device mapper debugging support
            < > Unstriped target
            <M> Crypt target support

5.1.4. Enable the TEE trusted key↑

  Security options  ---> 
    -*-  Enable access key retention support                           
     [ ]  Enable temporary caching of the last request_key() result   
     [ ]  Enable register of persistent per-UID keyrings              
     <M>  TRUSTED KEYS                                                
     [ ]  TPM based trusted keys 
     [*]  TEE based trusted keys NEW              
     <*>  ENCRYPTED KEYS

5.1.5. Check your kernel .config file↑

After building the kernel with the updated configuration, you can check the flag's values on your .config file at <Linux kernel build directory>:

    Check that the crypto using ST CRYP and HASH hardware accelerator and AES algorithm is enabled:

CONFIG_CRYPTO_DEV_STM32_HASH=y
CONFIG_CRYPTO_DEV_STM32_CRYP=y 
CONFIG_CRYPTO_AES=y

    Check that the crypt arm accelerators are disabled:

CONFIG_CRYPTO_AES_ARM=n
CONFIG_CRYPTO_AES_ARM_BS=n
CONFIG_CRYPTO_AES_ARM_CE=n

    Check that the device mapper and crypt function can be used by "dmsetup":

CONFIG_MD=y 
CONFIG_DM_CRYPT=y
CONFIG_BLK_DEV_DM=y

    Check that the encrypted and trusted key can be used:

CONFIG_KEYS=y
CONFIG_TRUSTED_KEYS=m
CONFIG_ENCRYPTED_KEYS=y
CONFIG_TRUSTED_KEYS_TEE=y

5.2. OP-TEE and image configuration in Developer Package↑
5.2.1. OP-TEE configuration↑

OP-TEE must have been compiled with "trusted key" TA in CFG_IN_TREE_EARLY_TAS option.

    Go to OP-TEE OS source directory:

cd <OP-TEE OS source directory>

    Open the makefile.sdk file, and add the line:

EXTRA_OEMAKE += "CFG_IN_TREE_EARLY_TAS=trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c"

    Compile OP-TEE OS and flash it into the board: see How to cross-compile with the Developer Package.

5.2.2. Image configuration with apt-get on board↑

You have to connect the board to the internet and install the needed packages:

apt-get install keyutils

Selecting previously unselected package keyutils.                               
(Reading database ... 15652 files and directories currently installed.)         
Preparing to unpack .../keyutils_1.6.3-r0_armhf.deb ...                         
Unpacking keyutils (1.6.3-r0) ...                                               
Setting up keyutils (1.6.3-r0) ...      

apt-get install libdevmapper

Selecting previously unselected package libdevmapper.                                                
(Reading database ... 15662 files and directories currently installed.)                              
Preparing to unpack .../libdevmapper_2.03.16-r0_armhf.deb ...                                        
Unpacking libdevmapper (2.03.16-r0) ...                                                              
Setting up libdevmapper (2.03.16-r0) ...                                                             

apt-get install lvm2

Selecting previously unselected package lvm2-udevrules.                                              
(Reading database ... 15665 files and directories currently installed.)                              
Preparing to unpack .../lvm2-udevrules_2.03.16-r0_armhf.deb ...                                      
Unpacking lvm2-udevrules (2.03.16-r0) ...                                                            
Selecting previously unselected package lvm2.                                                        
Preparing to unpack .../lvm2_2.03.16-r0_armhf.deb ...                                                
Unpacking lvm2 (2.03.16-r0) ...                                                                      
Selecting previously unselected package lvm2-scripts.                                                
Preparing to unpack .../lvm2-scripts_2.03.16-r0_armhf.deb ...                                        
Unpacking lvm2-scripts (2.03.16-r0) ...                                                              
Setting up lvm2-udevrules (2.03.16-r0) ...                                                           
Setting up lvm2 (2.03.16-r0) ...                                                                     
Setting up lvm2-scripts (2.03.16-r0) ...                       

5.3. OP-TEE and image configuration in Distribution Package↑
5.3.1. OP-TEE configuration↑

OP-TEE should have been compiled with the "trusted key" TA in the CFG_IN_TREE_EARLY_TAS option.

    Update the file '<working directory path of distribution>/layers/meta-st/meta-st-stm32mp/recipes-security/optee/optee-os-stm32mp-common.inc' with:

 
  EXTRA_OEMAKE += "CFG_IN_TREE_EARLY_TAS=trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c" 

5.3.2. Image configuration↑

    Update the file '<build_dir>/conf/local.conf' with:

    IMAGE_INSTALL:append= " keyutils "
    IMAGE_INSTALL:append= " lvm2 "

5.3.3. OpenSTLinux Image building↑

    Compile the OpenSTLinux image: see Building the OpenSTLinux distribution.

6. Process↑
6.1. Encrypted disk creation↑

    Create a secure volume. It could be a physical partition, in this example, use an image file and mount it later.

dd if=/dev/zero of=encrypted.img bs=1M count=10

10+0 records in
10+0 records out
10485760 bytes (10 MB, 10 MiB) copied, 0.149163 s, 70.3 MB/s                    

losetup /dev/loop0 encrypted.img

[   43.466773] loop0: detected capacity change from 0 to 20480

    Check the just created partition.

lsblk

 NAME         MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
 loop0          7:0    0   10M  0 loop 
 mmcblk0      179:0    0 14.8G  0 disk 
 |-mmcblk0p1  179:1    0  256K  0 part 
 |-mmcblk0p2  179:2    0  256K  0 part 
 |-mmcblk0p3  179:3    0  256K  0 part 
 |-mmcblk0p4  179:4    0  256K  0 part 
 |-mmcblk0p5  179:5    0    4M  0 part 
 |-mmcblk0p6  179:6    0    4M  0 part 
 |-mmcblk0p7  179:7    0  512K  0 part 
 |-mmcblk0p8  179:8    0   64M  0 part /boot                                       
 |-mmcblk0p9  179:9    0   16M  0 part /vendor                                     
 |-mmcblk0p10 179:10   0    4G  0 part /                                           
-mmcblk0p11 179:11   0 10.7G  0 part /usr/local  

    Trusted Key creation in keyring session.

keyctl add trusted my_key "new 32" @s

Here, this command generates a new key "my_key" with the OP-TEE random generator RNG internal peripheral.

 *** TRACE FROM: trusted_keys/entry.c, function: get_random ***  

"my_key" is trusted. Therefore, "my_key" is "sealed" (encrypted/wrapped) with a derived key from a HUK hidden in the SAES internal peripheral.

The below trace is the OP-TEE Trusted Key TA source code (added manually), shows the "Derived HUK" derivation by SAES internal peripheral. The OP-TEE SAES huk_subkey_derive function uses the internal SAES HUK and an input constant. Then this "Derived HUK" is used to seal "my_key" in an encrypted format.

                                                                                           
*** TRACE FROM: trusted_keys/entry.c, function: seal_trusted_key ***                     
                                                                                           
M/TC: *** TRACE FROM: stm32_saes.c, function: huk_subkey_derive *** 
53139771

    Check the key "my_key" is created. Show all the keys of the keyring session. Print the blob containing "my_key" encrypted at creation as seen above (key id 53139771).

keyctl show @s

result:
 Keyring
 989387511 --alswrv      0     0  keyring: _ses
 809952222 ----s-rv      0     0   \_ user: invocation_id
  53139771 --alswrv      0     0   \_ trusted: my_key

keyctl print 53139771

result:
009e09d9eb49852cc33d289e29dbcfea67b03a52afd38fb35cfb3f6d966918659fe6a29cabbd3a79e8dfd4c0f2248c0847

    Save the Key blob into a file.

The key generated is not persistent and is lost on reboot or logoff. Hence, export and save the encrypted key blob to persistent storage. The encryption key is encrypted by OP-TEE using a key derived from HUK and is exported as a blob to user space and is then saved on the filesystem on flash memory.

keyctl pipe 53139771 > my_key.blob

dmsetup -v create cryp_dev --table "0 $(blockdev --getsz /dev/loop0) crypt capi:cbc(aes)-plain :32:trusted:my_key 0 /dev/loop0 0 1 sector_size:1024"

 Name:              cryp_dev
State:             ACTIVE
Read Ahead:        256
Tables present:    LIVE
Open count:        0
Event number:      0
Major, minor:      253, 0
Number of targets: 1

The following is a brief description of the command.
Alternate text
dm_crypt command explanation

    setting 0 as start value means that the encrypting begins at 0.

    size is the size of the volume in sectors.

    blockdev gets the device's number of sectors.

    target is crypt.

    cipher is set in the kernel crypto API format to use the tagged key.

    IV is the initialization vector defined to plain64.

    key size is the size of the key.

    key type is the keyring key service type.

    key name is the key description to identify the key to load.

    IV offset is the value to add to the sector number to compute the IV value.

    device path is the path to the device to be used as back-end. It contains the encrypted data. 

    offset value as 0 means that encrypted data begins at sector 0 of the device.

For more information, see "DM-Setup documentation[7].

    Let us see the created device.

dmsetup table --showkeys

result:
cryp_dev: 0 20480 crypt capi:cbc(aes)-plain :32:trusted:my_key 0 7:0 0 1 sector_size:1024

    Delete the keys from the keyring session (optional).

keyctl clear @s

    Create a file system on the device.

mkfs.ext4 /dev/mapper/cryp_dev

mke2fs 1.46.5 (30-Dec-2021)
mke2fs 1.47.0 (5-Feb-2023)
Creating filesystem with 10240 1k blocks and 2560 inodes
Filesystem UUID: 7e0719ee-86b8-4812-9f78-e9d337f5dc12
Superblock backups stored on blocks: 
        8193

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done

    Setup a mount point.

mkdir /mnt/cryp_dev

    Mount the mapped device.

mount -t ext4 /dev/mapper/cryp_dev /mnt/cryp_dev/

[ 1436.179168] EXT4-fs (dm-0): mounted filesystem with ordered data mode. Opts:.

    Check the mountpoint.

lsblk

result:
NAME         MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS                               
loop0          7:0    0   32M  0 loop                                           
`-cryp_dev   253:0    0   10M  0 dm   /mnt/cryp_dev                             
mmcblk0      179:0    0 14.8G  0 disk                                           
|-mmcblk0p1  179:1    0  256K  0 part                                           
|-mmcblk0p2  179:2    0  256K  0 part                                           
|-mmcblk0p3  179:3    0  256K  0 part                                           
|-mmcblk0p4  179:4    0  256K  0 part                                           
|-mmcblk0p5  179:5    0    4M  0 part                                           
|-mmcblk0p6  179:6    0    4M  0 part                                           
|-mmcblk0p7  179:7    0  512K  0 part                                           
|-mmcblk0p8  179:8    0   64M  0 part /boot                                     
|-mmcblk0p9  179:9    0   16M  0 part /vendor                                   
|-mmcblk0p10 179:10   0    4G  0 part /                                         
`-mmcblk0p11 179:11   0 10.7G  0 part /usr/local

    Write some data to the device.

echo "This is a test of full disk encryption" > /mnt/cryp_dev/readme.txt
sync
6.2. Encrypted disk reload↑

    Reboot the board.

    Reload the trusted key in the keyring session.

keyctl add trusted my_key "load `cat my_key.blob`" @s

Here, my_key.blob contains the trusted key "my_key". This command unseals (decrypt) "my_key" with a derived HUK hidden in SAES internal peripheral. The below trace is the OP-TEE Trusted Key TA source code (added manually), shows the "Derived HUK" derivation by SAES internal peripheral. The OP-TEE SAES huk_subkey_derive function uses the internal SAES HUK and an input constant. Then this "Derived HUK" is used to unseal my_key.blob to retrieve "my_key" in clear text.

 *** TRACE FROM: trusted_keys/entry.c, function : unseal_trusted_key ***

M/TC: *** TRACE FROM : stm32_saes.c, function : huk_subkey_derive ***
990951249

    Redefine with dmsetup the logical device.

losetup /dev/loop0 encrypted.img

loop0: detected capacity change from 0 to 2048

dmsetup -v create cryp_dev --table "0 $(blockdev --getsz /dev/loop0) crypt capi:cbc(aes)-plain :32:trusted:my_key 0 /dev/loop0 0 1 sector_size:1024" Name: cryp_dev State: ACTIVE Read Ahead: 256 Tables present: LIVE Open count: 0 Event number: 0 Major, minor: 253, 0

Number of targets: 1

    Remount the device.

mount -t ext4 /dev/mapper/cryp_dev /mnt/cryp_dev/

[ 1673.451800] EXT4-fs (dm-0): recovery complete
[ 1673.466522] EXT4-fs (dm-0): mounted filesystem with ordered data mode. Opts.

    Read from device the data stored.

cat /mnt/cryp_dev/readme.txt

result:
This is a test of full disk encryption

6.3. Shared key mode↑

In this mode, in a provisioning phase in the factory, the trusted key used for encryption with dm_setup, is given in clear text as an input of keyctl and is wrapped using SAES and the SAES internal HUK. This wrapped key is stored in OPTEE memory and is no more used in clear in Linux kernel memory when using dm_setup. The wrapped key can as before be exported into a blob. After a cold boot, the wrapped key can be reloaded in OPTEE from the blob with keyctl. When decryption is needed to decrypt the files with dm_crypt with the trusted key, CRYPT IP is used for decryption. This wrapped key stored in OPTEE is unwrapped by SAES IP and loaded to be shared to CRYPT by HW bus. This mode hardens the encryption key because the key is never accessible in clear text to CPU. Patch is available on demand to use this mode.
6.4. Error Cases↑

root@stm32mp13-disco:~# keyctl add trusted my_key "new 64" @s                   
add_key: No such device

    Happens when the "trusted key" TA has not been correctly compiled as "early TA" (see chapter above).

If you set ST_OPTEE_DEBUG_LOG_LEVEL = 3 (in optee-os-stm32mp-archiver.inc) you should see the below OP-TEE log:

D/TC:0 0 early_ta_init:56 Early TA f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c size 59224

    Happens when the below kernel flags are not set in .config

CONFIG_KEYS, CONFIG_TRUSTED_KEYS, CONFIG_ENCRYPTED_KEYS
7. References↑

dm-crypt tool a kernel disk encryption subsystem
keyctl tool trusted and encrypted key, keyctl tool usage
AES CBC ESSIV, XTS Disk Encryption theory
DMCrypt Linux kernel device-mapper crypto target
https://static.linaro.org/connect/san19/presentations/san19-413.pdf Linaro connect TEE based Trusted Keys in Linux presentation
https://www.youtube.com/watch?v=kJdI_flEMR4&t=979s Video associated to Linaro presentation

    https://gitlab.com/cryptsetup/cryptsetup/-/wikis/DMCrypt DM-Setup documentation


    Platform security
    STM32MP13x

