# CyanogenMod 10 — common de los LG MT6575

Código y configuración compartidos por:

| Equipo | Codename | Device tree |
| --- | --- | --- |
| LG L4 II (E440g) | vee4ss | `device/lge/vee4ss` |
| LG L5 II (E450g) | vee5ss | `device/lge/vee5ss` |

Estructura tomada de `lge/msm7x27a-common` (TeamHackLG): cada equipo incluye
`BoardConfigCommon.mk` desde su `BoardConfig.mk` y hereda `mt6575-common.mk`
desde su `device.mk`. `Android.mk` solo entra en la compilación para los
equipos listados en su `filter`.

## Qué hay aquí

- `audio/`, `camera/`, `lights/`, `ril/`: HALs que envuelven los blobs stock
  (audio y cámara de MTK, LED LP5521 al estilo del EmotionalLedService de LG,
  shim de `mtk-ril.so`).
- `omx/`: envoltorio de `libstagefrighthw` para los codificadores y
  decodificadores de video por hardware de MTK.
- `lge-compat/`: clases de LG que enlaza `MtkBt.apk` (MDM desactivado) y el
  módulo `MtkBt`, que toma el APK del vendor de cada equipo.
- `libmtk_symbols/`: símbolos de las librerías MTK que esperan los blobs
  (va en `LD_PRELOAD`).
- `rootdir/root/`: `init.rc`, `init.mt6575.common.rc` (lo importa el
  `init.mt6575.rc` de cada equipo), `ueventd` y `fstab`.
- `rootdir/system/`: `init.d`, `vold.fstab`, `audio_policy.conf`,
  keymaps y el `media_codecs.xml` con los codecs de MTK (cada equipo elige
  qué `media_codecs.xml` instala).
- `patches/`: cambios temporales de plataforma CM (sin commit en los repos de
  CM). `patches/apply-patches.sh` los aplica.
- `releasetools.py`, `bootimg.mk`, `mtkbootimg/`: boot image con cabeceras MTK.

## Lo propio de cada equipo

Kernel, particiones, recovery, densidad de pantalla y heap de dalvik,
`media_profiles.xml`, el `init.mt6575.rc` con sus servicios (sensor
`bmm050d`/`bmc150d`, `bootlogoupdater` del L4), USB y los blobs que solo
tiene su vendor. Los blobs quedan por equipo (`vendor/lge/<equipo>`): vienen
de firmwares distintos (E44010a, E450g04a) y la mitad difiere.

Por ahora solo el L5 II activa los codecs OMX de MTK; el vendor del
L4 II todavía no tiene esos blobs.
