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
- `ril/`: además de darle a `mtk-ril.so` el `RIL_Env` de MediaTek, el shim
  hace lo que hacía el framework de telefonía de MediaTek y el de CM no:
  `SET_GPRS_TRANSFER_TYPE` y `SET_GPRS_CONNECT_TYPE` al encender la radio,
  acepta las llamadas entrantes (`+EAIC` → `SET_CALL_INDICATION`), agrega el
  cid a `SETUP_DATA_CALL` y traduce los URC de MTK (progreso de llamada,
  estado PS, SIM) a los de AOSP. Así llamadas, SMS y datos funcionan sin
  parchear el framework de CM.
- `omx/`: envoltorio de `libstagefrighthw` para los codificadores y
  decodificadores de video por hardware de MTK. Publica los decodificadores
  como `OMX.google.MTK.*` (CM copia su salida a la pantalla) y les devuelve a
  los frames de H.264 el timestamp que el decodificador de MTK pierde.
- `FmRadio/`: app de radio FM propia (Apache 2.0, diseño e íconos de la app
  FM Radio de AOSP 5.1) sobre las librerías FM stock (`libfmjni` y demás, en
  cada vendor) y el HAL de audio stock (`fm_audio`, `fm_speakerphone`,
  `SetFmVolume`). `/dev/fm` lo crea `init.d/10mtk-nodes` y
  `etc/permissions/com.android.fmradio.xml` le da a la app el grupo `media`.
- `lge-compat/`: clases de LG que enlaza `MtkBt.apk` (MDM desactivado) y el
  módulo `MtkBt`, que toma el APK del vendor de cada equipo.
- `libmtk_symbols/`: símbolos de las librerías MTK que esperan los blobs
  (va en `LD_PRELOAD`).
- `rootdir/root/`: `init.rc`, `init.mt6575.common.rc` (lo importa el
  `init.mt6575.rc` de cada equipo), `ueventd` y `fstab`.
- `rootdir/system/etc/init.d/05serialno`: le pone al USB el serial que lee
  el `init` stock de LG (`/sys/sys_info/serial_number`), así adb muestra el
  mismo serial en stock, CM y TWRP en vez de `0123456789ABCDEF`.
- `rootdir/system/`: `init.d`, `vold.fstab`, `audio_policy.conf`,
  keymaps y el `media_codecs.xml` con los codecs de MTK (cada equipo elige
  qué `media_codecs.xml` instala).
- `patches/`: cambios temporales de plataforma CM (sin commit en los repos de
  CM). `patches/apply-patches.sh` los aplica.
- `releasetools.py`, `bootimg.mk`, `mtkbootimg/`: boot image con cabeceras MTK.
  El zip escribe `boot-mtk.img` con `dd` y hace `sync` al final: sin eso, un
  reinicio con el botón justo después de instalar dejaba la cabecera nueva
  sobre el ramdisk viejo y el teléfono no arrancaba.

## Lo propio de cada equipo

Kernel, particiones, recovery, densidad de pantalla y heap de dalvik,
`media_profiles.xml`, el `init.mt6575.rc` con sus servicios (sensor
`bmm050d`/`bmc150d`, `bootlogoupdater` del L4), USB y los blobs que solo
tiene su vendor. Los blobs quedan por equipo (`vendor/lge/<equipo>`): vienen
de firmwares distintos (E44010a, E450g04a) y la mitad difiere.

Los dos equipos usan los codecs OMX de MTK; el vendor del L4 II los toma del
firmware del E450g (mismo MT6575 y misma versión de LG) y corren sobre su
propio `vcodec_kernel_driver.ko`.

## Referencia del stock

`/mnt/fuentes/backups/lg-l5ii-stock-snapshot-20261025/` tiene una foto del
stock del L5 II (propiedades, `/system/etc`, ramdisk, `dumpsys`, logs, sesión
de radio FM y consumo en reposo) para comparar sin volver a restaurarlo. En
reposo, sin WiFi ni SIM, el stock queda en deep sleep el 98,7 % del tiempo y
CM10 el 99,4 %.
