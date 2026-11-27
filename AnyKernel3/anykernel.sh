### AnyKernel3 Ramdisk Mod Script
## osm0sis @ xda-developers

### AnyKernel setup
# global properties
properties() { '
kernel.string=BrBL-Kernel by Brian & Blaze
do.devicecheck=1
do.modules=0
do.systemless=1
do.cleanup=1
do.cleanuponabort=0
device.name1=a70q
device.name2=a70
device.name3=
device.name4=
device.name5=
supported.versions=
supported.patchlevels=
supported.vendorpatchlevels=
'; } # end properties


### AnyKernel install
## boot files attributes
attributes() {
set_perm_recursive 0 0 755 644 $ramdisk/*;
set_perm_recursive 0 0 750 750 $ramdisk/init* $ramdisk/sbin;
} # end attributes

# boot shell variables
block=/dev/block/bootdevice/by-name/boot;
is_slot_device=0;
ramdisk_compression=auto;
patch_vbmeta_flag=auto;

# import functions/variables and setup patching - see for reference (DO NOT REMOVE)
. tools/ak3-core.sh;

# boot install
dump_boot;

device=$(file_getprop /system/build.prop ro.product.system.device);
android=$(file_getprop /system/build.prop ro.build.version.sdk);
oneui=$(file_getprop /system/build.prop ro.build.version.oneui);

patch_cmdline "android.is_aosp" "";

if [ -z "$device" ] && [ -z "$android" ]; then
    ui_print " ";
    ui_print "Skipping ROM detection...";
    ui_print " ";
else
    if [ -n "$oneui" ]; then
        ui_print " ";
        ui_print "OneUI ROM detected!";
        ui_print " ";
    elif [ "$device" == "generic" ]; then
        ui_print " ";
        ui_print "GSI ROM detected!";
        ui_print " ";
    else
        ui_print " ";
        ui_print "AOSP ROM detected!";
        ui_print " ";
        patch_cmdline "android.is_aosp" "android.is_aosp=1";
    fi
fi

# Patch fstab, this will change systems filesystem type from ext4 to erofs
for fstab in $ramdisk/fstab.*; do
    [ -f "$fstab" ] || continue
    if grep -q "erofs" "$fstab"; then
        sed -i '/\/system/s/ext4/erofs/' "$fstab"
        ui_print " - Patched $fstab: system ext4 -> erofs";
    fi
done

# Fix up RamDisk the ugly way
cat << 'EOF' > /tmp/anykernel/ramdisk/fstab.default
# Android fstab file.
#<src>                                                  <mnt_point>              <type>  <mnt_flags and options>                              <fs_mgr_flags>
/dev/block/platform/soc/1d84000.ufshc/by-name/system     /system                  erofs   ro                                                   wait,first_stage_mount
/dev/block/platform/soc/1d84000.ufshc/by-name/vendor     /vendor                  erofs   ro                                                   wait,first_stage_mount
/dev/block/platform/soc/1d84000.ufshc/by-name/omr        /metadata                ext4    noatime,nosuid,nodev,noauto_da_alloc,discard,journal_checksum,data=ordered,errors=panic   wait,formattable,check,first_stage_mount



/dev/block/by-name/vbmeta                               /vbmeta                  emmc    defaults                                             first_stage_mount
/dev/block/bootdevice/by-name/boot                      /boot                    emmc    defaults                                             recoveryonly
/dev/block/bootdevice/by-name/recovery                  /recovery                emmc    defaults                                             recoveryonly
/dev/block/bootdevice/by-name/cache                     /cache                   ext4    noatime,nosuid,nodev,noauto_da_alloc,discard,journal_checksum,data=ordered,errors=panic                wait,check
/dev/block/bootdevice/by-name/userdata                  /data                    f2fs    noatime,nosuid,nodev,discard,usrquota,grpquota,fsync_mode=nobarrier,reserve_root=32768,resgid=5678,inlinecrypt    latemount,wait,check,formattable,fileencryption=aes-256-xts:aes-256-cts:v2+inlinecrypt_optimized,metadata_encryption=aes-256-xts,keydirectory=/metadata/vold/metadata_encryption,quota,reservedsize=128M,checkpoint=fs
/dev/block/bootdevice/by-name/apnhlos                   /vendor/firmware_mnt     vfat    ro,shortname=lower,uid=0,gid=1000,dmask=227,fmask=337,context=u:object_r:firmware_file:s0 wait
/dev/block/bootdevice/by-name/modem                     /vendor/firmware-modem   vfat    ro,shortname=lower,uid=0,gid=1000,dmask=227,fmask=337,context=u:object_r:firmware_file:s0 wait
/dev/block/bootdevice/by-name/dsp                       /vendor/dsp              ext4    ro,nosuid,nodev,barrier=1                            wait
/dev/block/bootdevice/by-name/persist                   /mnt/vendor/persist      ext4    noatime,nosuid,nodev,noauto_da_alloc,discard,journal_checksum,data=ordered,errors=panic   wait,check
/dev/block/bootdevice/by-name/efs                       /mnt/vendor/efs          ext4    noatime,nosuid,nodev,noauto_da_alloc,discard,journal_checksum,data=ordered,errors=panic   wait,check
/dev/block/bootdevice/by-name/sec_efs                   /efs                     ext4    noatime,nosuid,nodev,noauto_da_alloc,discard,journal_checksum,data=ordered,errors=panic   wait,check
/dev/block/bootdevice/by-name/misc                      /misc                    emmc    defaults                                             defaults

# VOLD :: fstab_non_AB_variant.qti
/devices/platform/soc/8804000.sdhci/mmc_host*                           auto                auto    defaults        wait,voldmanaged=sdcard:auto,encryptable=userdata
/devices/platform/soc/a600000.ssusb/a600000.dwc3/xhci-hcd.0.auto*       auto                auto    defaults        voldmanaged=usb:auto
EOF

write_boot;
## end boot install
