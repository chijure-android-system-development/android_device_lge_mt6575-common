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

| Parche | Equipos | Para qué | Comprobado |
| --- | --- | --- | --- |
| `frameworks_base/0001` GSMPhone | ambos | reintenta baseband e IMEI al encender la radio | ver nota |
| `frameworks_base/0002` IccCard | ambos | consulta la SIM mientras la radio sigue UNAVAILABLE (PIN, bloqueo de red) | necesario: sin 0001–0003 una SIM bloqueada queda en UNKNOWN |
| `frameworks_base/0003` IccCard | ambos | `setRadioPower` cada 15 s si la SIM sigue UNKNOWN | ídem |
| `frameworks_base/0004` BlueAngel | ambos | stack Bluetooth de MediaTek (MT6620) en el framework | necesario: el framework de CM espera BlueZ, que este chip no tiene |
| `frameworks_base/0005` BluetoothDevice | ambos | `getLinkKeyPersistState()` que usa MtkBt al recibir archivos | necesario: sin él MtkBt se cae al recibir |
| `frameworks_native/0001` HWC ABI | solo vee4ss | GraphicBuffer y SurfaceFlinger con la ABI HWC de MTK | necesario para la pantalla del L4 II; el L5 II compone por GLES |
| `hardware_libhardware/0001` hwcomposer.h | solo vee4ss | campos privados de capa y lista del HWC de MTK | va con el anterior |
| `packages_apps_Phone/0001` BluetoothHandsfree | ambos | no cierra un socket SCO que BlueAngel nunca crea | evita el crash de Phone al apagar Bluetooth |

Nota (2026-10-25, L4 II): sin 0001–0003, con una SIM de otra operadora
la radio se queda RADIO_NOT_AVAILABLE, la SIM en UNKNOWN y no aparece ni
el aviso de desbloqueo de red ni la baseband; con ellos la SIM pasa a
NETWORK_LOCKED. Sin SIM, la baseband y el IMEI aparecen igual.

Se quitaron dos parches que resultaron innecesarios en estos equipos: el
de la MAC RNDIS de UsbDeviceManager (solo silenciaba un error de log) y el
que no registraba VideoEditorProfile (venía del Y210; sin él el L4 II
arranca normal).

Ya no hay parches para `packages/apps/Bluetooth` (MtkBt reemplaza a la app de
CM en los dos equipos) ni para `hardware/ril` (lo reemplazó el shim
`mt6575-common/ril`).
