# Parches temporales de plataforma (CM10)

Cambios a repos de CyanogenMod que todavía no se pudieron llevar al device
tree. Se aplican **sin commit** sobre `m/jellybean`; nunca se commitean en los
repos de CM.

```
device/lge/mt6575-common/patches/apply-patches.sh    # aplica las series
device/lge/mt6575-common/patches/revert-patches.sh   # las quita
```

`projects.sh` dice a qué repo va cada carpeta y comprueba si una serie ya
está aplicada completa.

| Parche | Equipos | Para qué | Estado |
| --- | --- | --- | --- |
| `frameworks_base/0001` GSMPhone | ambos | reintenta baseband e IMEI al encender la radio | del Build 2 del L5 (llamadas OK); sin probar con el shim del RIL |
| `frameworks_base/0002` IccCard | ambos | consulta la SIM mientras la radio sigue UNAVAILABLE (PIN) | ídem |
| `frameworks_base/0003` IccCard | ambos | `setRadioPower` cada 15 s si la SIM sigue UNKNOWN | ídem |
| `frameworks_base/0004` UsbDeviceManager | ambos | no escribe la MAC RNDIS (el kernel no tiene `ethaddr`) | quita un error de log |
| `frameworks_base/0005` MediaProfiles | ambos | no registra los nativos de VideoEditorProfile | viene del Y210; no se comprobó si MT6575 lo necesita |
| `frameworks_base/0006` BlueAngel | ambos | stack Bluetooth de MediaTek (MT6620) en el framework | necesario; incluye `cutils/xlog.h` de `mt6575-common/include` |
| `frameworks_base/0007` BluetoothDevice | ambos | `getLinkKeyPersistState()` que usa MtkBt al recibir archivos | necesario para recibir por OPP |
| `frameworks_native/0001` HWC ABI | solo vee4ss | GraphicBuffer y SurfaceFlinger con la ABI HWC de MTK | necesario para la pantalla del L4 II; el L5 II compone por GLES |
| `hardware_libhardware/0001` hwcomposer.h | solo vee4ss | campos privados de capa y lista del HWC de MTK | va con el anterior |
| `packages_apps_Phone/0001` BluetoothHandsfree | ambos | no cierra un socket SCO que BlueAngel nunca crea | evita el crash de Phone al apagar Bluetooth |

Ya no hay parches para `packages/apps/Bluetooth` (MtkBt reemplaza a la app de
CM en los dos equipos) ni para `hardware/ril` (lo reemplazó el shim
`mt6575-common/ril`).
