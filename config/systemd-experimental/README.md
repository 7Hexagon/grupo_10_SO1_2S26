# Migracion experimental de systemd

Las unidades y el menu GRUB son los archivos locales preparados para la VM experimental; no se cotejaron de nuevo con la VM tras el ultimo cambio porque la conexion dejo de responder. La red usa la MAC y la IP exclusivas de esa VM. `grub-prueba.cfg` es el menu de la ISO auxiliar, no una instalacion en disco; ofrece `aegis-init` y systemd.

Systemd 261.2 y D-Bus 1.16.2 se instalaron en la copia. El usuario confirmo el arranque con systemd como PID 1 y la operacion de red, Dropbear, aegisd y journal. Cuatro pruebas de namespaces de usuario seguian fallando con CONFIG_USER_NS desactivado; las funciones que dependen de esa opcion no se dan por verificadas. La auditoria posterior se describe en `config/hardening/systemd-resolved-experimental.md`.

## Incidente posterior

Al intentar repetir las verificaciones el 19/11, SSH agoto el tiempo de espera y la consola de VirtualBox mostro `Kernel panic - not syncing: Fatal exception in interrupt`. La causa no esta determinada. VirtualBox indicaba `running`; no se reinicio ni apago la VM. No se dispone de un respaldo actualizado validado posterior a la auditoria ni se obtuvo una copia local de su informe. El snapshot `systemd-261.2-instalado-pre-primer-arranque-20261119` es anterior al primer arranque systemd. No restaurarlo automaticamente: perderia los cambios posteriores.

## Siguiente etapa: GRUB en disco

La VM usa firmware BIOS y la ISO auxiliar. El disco conectado es la hoja diferencial `88c48381-9d1b-4b72-b274-65b7e1673007`, con padre `f6ebea58-2d7a-422d-9ce0-c5257f56f8a3`. No tocar la VM original ni modificar snapshots. El menu de la ISO identifica la raiz con UUID `73dc87ee-e227-4f48-939f-67f81f6556d3` y PARTUUID `3c533006-01`, que deben comprobarse dentro de la VM recuperada. La tabla de particiones, espacio de insercion BIOS y disponibilidad de GRUB en el invitado no se comprobaron durante este incidente.

Antes de instalar: recuperar la VM y extraer el informe; comprobar integridad sin reparaciones automaticas; crear y verificar un punto de recuperacion coherente; identificar el disco de la copia y examinar con `lsblk -o NAME,PATH,SERIAL,PTTYPE,PARTTYPE,PARTUUID,FSTYPE,UUID,MOUNTPOINTS` y `sfdisk -l` como root en modo lectura; verificar esquema y espacio de instalacion BIOS. No usar nombres `/dev/sdX` supuestos. Si no existe espacio seguro, detenerse y no modificar particiones. Mantener la ISO y la entrada de recuperacion.

Con autorizacion expresa, GRUB para BIOS se instalaria en el disco completo experimental, no en una particion ni en Rocky; escribe el sector de arranque y areas asociadas. Se prepararia `/boot/grub/grub.cfg` con entradas separadas `init=/usr/lib/systemd/systemd` e `init=/usr/sbin/aegis-init`, con busqueda por UUID; comprobar sintaxis y respaldar configuracion existente. Probar solo en la copia conservando la ISO. Referencia: https://www.linuxfromscratch.org/lfs/view/13.1-systemd/chapter10/grub.html
