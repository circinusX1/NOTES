
# iMX8, DM-Crypt with Trusted keys backed by CAAM
### Reference: i.MX Linux Users guide.pdf

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

## Burn fuses


```
# a uuu burnfuses.uuu/lst, replace fuses HEX values with yours from SRK_1_2_3_4_fuse.bin.u-boot-cmds
uuu_version 1.4.149
SDP: boot -f _flash.bin
SDPV: delay 2000
FB: ucmd fuse prog -y 6 0 0xDA6B9AD0
FB: ucmd fuse prog -y 6 1 0xDC9B5501
FB: ucmd fuse prog -y 6 2 0x93D10134
FB: ucmd fuse prog -y 6 3 0x5CDC0DA3
FB: ucmd fuse prog -y 7 0 0x100709F8
FB: ucmd fuse prog -y 7 1 0xC00305A7
FB: ucmd fuse prog -y 7 2 0x3E700DA2
FB: ucmd fuse prog -y 7 3 0x00000000
FB: Done

```

and

```
uuu_version 1.4.149
FB: ucmd fuse prog -y 1 3 0x02000000
# FB: ucmd fuse prog -y 1 3 0x200000   # to disable JTAG
FB: Done
```



## CAAM TRUSTED

*  bootup into iMX shell
```

modprobe trusted

### test it first
##### create a block file

DEV=/dev/loop0
BLOCKS=20
fallocate -l $((BLOCKS*512)) ./loop0.img
losetup -P $DEV ./loop0.img

#### 1. encrypt, create blob, then enc masterkey@kernel+blob

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
#### 2. mount after boot 
   * Mount encrypted using mster key@ kernel + blob
      * reboot
      * Mount using the blob + masterkey@kernel  

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
#### 3. Check if another dev can decrypt using blob and file 
The step 1 should be performed on a device with HAB fuses burned and activated, like all fuses from below to be burnd.


   * move /$KEYNAME.blob and loop0.img to another machine or bootup from another media with a different kernel or un HAB-ed kernel
   * Try to revive the key using the blob.
      * Test ssucceded, we cannot decrypt.
```
keyctl add trusted $KEYNAME "load $(cat ./$KEYNAME.blob)" @s
[   50.928421] caam_jr 30902000.jr: using insecure test key, enable HAB to use unique device key!
[   50.937112] trusted_key: key_unseal failed (-74)
add_key: Bad message

```

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
   

