# Migracion experimental de systemd

Las unidades y el menu GRUB son los archivos locales preparados para la VM experimental; no se cotejaron de nuevo con la VM tras el ultimo cambio porque la conexion dejo de responder. La red usa la MAC y la IP exclusivas de esa VM. `grub-prueba.cfg` es el menu de la ISO auxiliar, no una instalacion en disco; ofrece `aegis-init` y systemd.

Systemd 261.2 y D-Bus 1.16.2 se instalaron en la copia. El usuario confirmo el arranque con systemd como PID 1 y la operacion de red, Dropbear, aegisd y journal. Cuatro pruebas de namespaces de usuario seguian fallando con CONFIG_USER_NS desactivado; las funciones que dependen de esa opcion no se dan por verificadas. La auditoria posterior se describe en `config/hardening/systemd-resolved-experimental.md`.

## Incidente posterior

Al intentar repetir las verificaciones el 19/11, SSH agoto el tiempo de espera y la consola de VirtualBox mostro `Kernel panic - not syncing: Fatal exception in interrupt`. La causa no esta determinada. VirtualBox indicaba `running`; no se reinicio ni apago la VM. No se dispone de un respaldo actualizado validado posterior a la auditoria ni se obtuvo una copia local de su informe. El snapshot `systemd-261.2-instalado-pre-primer-arranque-20261119` es anterior al primer arranque systemd. No restaurarlo automaticamente: perderia los cambios posteriores.

## Siguiente etapa: GRUB en disco

La VM usa firmware BIOS y la ISO auxiliar. El disco conectado es la hoja diferencial `88c48381-9d1b-4b72-b274-65b7e1673007`, con padre `f6ebea58-2d7a-422d-9ce0-c5257f56f8a3`. No tocar la VM original ni modificar snapshots. El menu de la ISO identifica la raiz con UUID `73dc87ee-e227-4f48-939f-67f81f6556d3` y PARTUUID `3c533006-01`, que deben comprobarse dentro de la VM recuperada. La tabla de particiones, espacio de insercion BIOS y disponibilidad de GRUB en el invitado no se comprobaron durante este incidente.

Antes de instalar: recuperar la VM y extraer el informe; comprobar integridad sin reparaciones automaticas; crear y verificar un punto de recuperacion coherente; identificar el disco de la copia y examinar con `lsblk -o NAME,PATH,SERIAL,PTTYPE,PARTTYPE,PARTUUID,FSTYPE,UUID,MOUNTPOINTS` y `sfdisk -l` como root en modo lectura; verificar esquema y espacio de instalacion BIOS. No usar nombres `/dev/sdX` supuestos. Si no existe espacio seguro, detenerse y no modificar particiones. Mantener la ISO y la entrada de recuperacion.

Con autorizacion expresa, GRUB para BIOS se instalaria en el disco completo experimental, no en una particion ni en Rocky; escribe el sector de arranque y areas asociadas. Se prepararia `/boot/grub/grub.cfg` con entradas separadas `init=/usr/lib/systemd/systemd` e `init=/usr/sbin/aegis-init`, con busqueda por UUID; comprobar sintaxis y respaldar configuracion existente. Probar solo en la copia conservando la ISO. Referencia: https://www.linuxfromscratch.org/lfs/view/13.1-systemd/chapter10/grub.html

## Recuperacion y punto previo a GRUB - 2026-11-19

El RAW de analisis reparado paso `e2fsck -f -n` sin errores con la particion desmontada. Se convirtio a un VDI independiente y se compararon los 30 GiB logicos con el RAW: coincidencia byte a byte. En `Aegis-Auth-reparada-prueba`, primero arranco `aegis-init` y luego systemd como PID 1. La prueba aislada duro mas de 74 minutos sin repetir el panic; despues se conecto host-only, con IP 192.168.56.111. Se verificaron SSH administrativo, rechazo de root, aegisd, journal y systemd sin unidades fallidas. El usuario confirmo la prueba de autenticacion invalida. Una consulta posterior del kernel no mostro coincidencias de panic, fallos de E/S o errores ext4; esto no determina la causa del fallo anterior ni demuestra estabilidad definitiva.

La auditoria v1.1 posterior, usando su parametro de IP experimental, dio PASS=8, FAIL=0, WARN=0: `auditoria/systemd-recuperada/aegis-audit-aegis-auth-20261119-193746.txt`. SHA-256 a50f61af13daa443630e118e6f4ffcd0155e0f53379da4872e4970635b085c01. El estado anterior `aegis-audit-aegis-auth-20261119-143419.txt` se conserva en el directorio local del incidente.

Se sincronizo la VM, se solicito apagado ACPI y se confirmo `poweroff`. El snapshot `systemd-recuperado-auditoria-8pass-pre-grub-20261119` UUID `0c10d4e8-fc89-4fc4-b092-d252191a47ad` se creo con la VM apagada. El VDI base es `fe4c36c6-8c79-40d2-a718-8cc26ce5cd40`; la nueva hoja es `a612d595-2c5f-4691-9438-abc115cfab95`. Es un punto de VirtualBox, no una copia independiente; requiere ambos discos y no sustituye el respaldo del incidente. La VM quedo apagada.

## Candidato de arranque en disco

La VM usa BIOS y la tabla de particiones observada es DOS, con UUID de tabla `3c533006`. Antes del snapshot, la particion ext4 UUID `73dc87ee-e227-4f48-939f-67f81f6556d3` era `/dev/sda1`, iniciaba en el sector 2048 y ocupaba 62912512 sectores de 512 bytes. El disco observado tenia serial `VBfe4c36c6-40cde56c`; tras crear el snapshot la hoja activa cambio y debe comprobarse nuevamente la identidad y el serial. No asumir que el espacio anterior a la particion esta libre de datos de arranque ni escribirlo sin verificar.

No se encontraron grub-install, grub-mkconfig ni `/usr/lib/grub/i386-pc`; existe `/sources/grub-2.14.tar.xz`. La instalacion del paquete BIOS segun LFS requiere unos 245 MB de espacio de compilacion y verificar fuentes y dependencias; no se ha realizado. El espacio libre del anfitrion tras el snapshot fue aproximadamente 18.53 GiB.

`grub-disco-candidato.cfg` prepara una entrada systemd por defecto y otra de recuperacion mediante aegis-init. Solo se reviso estaticamente; `grub-script-check` no estaba disponible y no se ha probado en disco. Para el siguiente paso: arrancar desde la ISO auxiliar, confirmar UUID, PARTUUID y medio activo, validar y respaldar el punto de restauracion, verificar paquete y espacio de insercion BIOS; despues preparar la instalacion del paquete y solicitar autorizacion especifica antes de cualquier escritura del cargador al disco. Mantener la ISO conectada para rescate. Instalar GRUB BIOS escribe el MBR y areas asociadas; no ejecutar sobre Rocky ni elegir el destino por un nombre `/dev/sdX` sin verificar. La MAC y direccion IP son exclusivas mientras las otras copias estan apagadas.

Referencias: https://www.linuxfromscratch.org/lfs/view/13.1-systemd/chapter08/grub.html y https://www.linuxfromscratch.org/lfs/view/13.1-systemd/chapter10/grub.html.
