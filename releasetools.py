# MT6575 (MT6575) OTA release tools override.
#
# The generic OTA code rebuilds boot.img from target_files with plain
# mkbootimg, so the ramdisk lacks MediaTek's 512-byte "ROOTFS" header the
# MT6575 bootloader needs. CM10 has no hook to replace that image, so
# FullOTA_InstallEnd builds boot-mtk.img with mtkbootimg from the same
# kernel and ramdisk, adds it to the package and flashes it with dd at the
# boot partition's raw offset (block 36864 = 0x1200000 bytes on mmcblk0),
# overriding whatever the standard boot.img line wrote.

import os
import subprocess
import tempfile

import common


def _read_boot_arg(boot_dir, name):
    path = os.path.join(boot_dir, name)
    if not os.path.exists(path):
        return None
    f = open(path, "r")
    try:
        return f.read().strip()
    finally:
        f.close()


def _build_mtk_boot_image(input_tmp):
    boot_dir = os.path.join(input_tmp, "BOOT")
    kernel = os.path.join(boot_dir, "kernel")
    ramdisk = os.path.join(boot_dir, "ramdisk.img")

    if not os.path.exists(kernel) or not os.path.exists(ramdisk):
        raise common.ExternalError(
            "target_files has no BOOT/kernel or BOOT/ramdisk.img")

    out_name = os.path.join(tempfile.mkdtemp(), "boot-mtk.img")

    cmd = [
        "mtkbootimg",
        "--kernel", kernel,
        "--ramdisk", ramdisk,
        "--ramdisk_offset", "0x01000000",
        "--tags_offset", "0x00000100",
        "--mtk", "1",
        "--output", out_name,
    ]

    cmdline = _read_boot_arg(boot_dir, "cmdline")
    if cmdline:
        cmd.extend(["--cmdline", cmdline])

    base = _read_boot_arg(boot_dir, "base")
    if base:
        cmd.extend(["--base", base])

    pagesize = _read_boot_arg(boot_dir, "pagesize")
    if pagesize:
        cmd.extend(["--pagesize", pagesize])

    p = common.Run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()
    if p.returncode != 0:
        raise common.ExternalError(
            "mtkbootimg failed while building boot-mtk.img:\n%s%s" %
            (stdout, stderr))

    f = open(out_name, "rb")
    try:
        return f.read()
    finally:
        f.close()


def FullOTA_InstallEnd(info):
    data = _build_mtk_boot_image(info.input_tmp)
    common.CheckSize(data, "boot.img", info.info_dict)
    common.ZipWriteStr(info.output_zip, "boot-mtk.img", data)

    info.script.AppendExtra('package_extract_file("boot-mtk.img", "/tmp/boot-mtk.img");')
    info.script.AppendExtra(
        'run_program("/sbin/sh", "-c",'
        ' "dd if=/tmp/boot-mtk.img of=/dev/block/mmcblk0 bs=512 seek=36864");')
    info.script.AppendExtra('delete("/tmp/boot-mtk.img");')
