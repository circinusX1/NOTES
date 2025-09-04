
# NXP How to encrypt partition

### prerequisites
-- --

* HAB enabled, u-boot and kernel signed, Using Yocto abomination echosystem: 
   * https://dev.variscite.com/dart-mx8m-mini/mx8mm-yocto-scarthgap-6.6.52_2.2.0-v1.0/
       * https://dev.variscite.com/dart-mx8m-mini/yocto/high-assurance-boot-mx8/
   * u-boot messages hab status 0 events  
   * FUSE burned for HAB
   * Device closed
* Extra packages
   * local.conf
      * MACHINE_FEATURES:append = " optee"  
      * DISTRO_FEATURES:append = " optee"
      * IMAGE_INSTALL:append = " optee-os optee-test "
   * your image bb
      * CORE_IMAGE_EXTRA_INSTALL+="coreutils keyutils lvm2 e2fsprogs-mke2fs util-linux keyctl-caam"
* Kernel config
   * linux-variscite_6_6.bbapend
```
SRC_URI += " \
    file://spectra-kernel.cfg \
"
spectra-kernel.cfg
CONFIG_KEYS=y
CONFIG_TRUSTED_KEYS=m
CONFIG_TRUSTED_KEYS_CAAM=y
CONFIG_SECURITY=y
CONFIG_SECURITYFS=y
CONFIG_CRYPTO_HASH=y
CONFIG_CRYPTO_CRYP=y
CONFIG_CRYPTO_CRYPTD=y
CONFIG_CRYPTO_AES=y
CONFIG_CRYPTO_CBC=m
```

## CAAM TRUSTED

*  bootup into iMX shell
```

odprobe trusted

# create a block file
DEV=/dev/loop0
BLOCKS=20
fallocate -l $((BLOCKS*512)) ./loop0.img
losetup -P $DEV ./loop0.img

# encrypt
KEYNAME=dm_test2
KEY="$(keyctl add trusted $KEYNAME 'new 32' @s)"
keyctl pipe $KEY >./$KEYNAME.blob
DEV=/dev/loop0
ALGO="capi:cbc(aes)-plain"
BLOCKS=20
TARGET=crypt
TABLE="0 $BLOCKS $TARGET $ALGO :32:trusted:$KEYNAME 0 $DEV 0 1 allow_discards"
echo $TABLE | dmsetup create testenc
echo $TABLE | dmsetup load testenc
dd if=/dev/zero of=/dev/mapper/testenc || true
echo "It works" 1<> /dev/mapper/testenc
umount /mnt/testenc/
dmsetup remove testenc


```
   * reboot
   * check it out 

```
KEYNAME=dm_test2
keyctl add trusted $KEYNAME "load $(cat ./$KEYNAME.blob)" @s
DEV=/dev/loop0
ALGO="capi:cbc(aes)-plain"
BLOCKS=20
TARGET=crypt
TABLE="0 $BLOCKS $TARGET $ALGO :32:trusted:$KEYNAME 0 $DEV 0 1 allow_discards"
losetup -P $DEV ./loop0.img
echo $TABLE | dmsetup create testenc
echo $TABLE | dmsetup load testenc
hexdump -C /dev/mapper/testenc


```
   * move to another macine the /$KEYNAME.blob and loop0.img
   * scond procedure should not succeed

# encrypt a partition

```

# run every bootup
#!/bin/bash

readonly MNT_DIR="/encrypted_disk"
readonly PART_TO_ENC=mmcblkp2  							# change with your partition
readonly DEV_MPR_SEC="/dev/mapper/secured"
readonly BLB_FILE=~/dmtrust_kn
readonly ALGO='capi:cbc(aes)-plain'
readonly BLOCKS=$(( $(lsblk -lb | grep ${PART_TO_ENC} | awk '{print $4}') / 512 ))
readonly TABLE="0 ${BLOCKS} crypt ${ALGO} :32:trusted:dmtrust_kn 0 ${ICP_SEC_PART} 0 1 allow_discards"


if [[ ! -f ${BLB_FILE}.blob ]];then
	# first time
    klog "Encrypting ${BLOCKS} blocks of: ${ICP_SEC_PART}"
    readonly KEY="$(keyctl add trusted dmtrust_kn 'new 32' @s)"
    keyctl pipe $KEY > ${BLB_FILE}.blob
    if [[ ! -f ${BLB_FILE}.blob ]];then
        exit 1
    fi
    chattr +i ${BLB_FILE}.blob
    echo ${TABLE} | dmsetup create secured
    echo ${TABLE} | dmsetup load secured
    mkfs.ext4 -F ${DEV_MPR_SEC} -L secret
    sync
else
	# every time
    keyctl add trusted dmtrust_kn "load $(cat ${BLB_FILE}.blob)" @s
    echo $TABLE | dmsetup create secured
    echo $TABLE | dmsetup load secured
fi
mount -t ext4 ${DEV_MPR_SEC} ${MNT_DIR}



```
   

