# systemd-resolved en la VM experimental

El 19/11/2026 la copia `Aegis-Auth-systemd-experimental` arranco con systemd 261.2 como PID 1. La auditoria detecto listeners de systemd-resolved en 127.0.0.53:53, 127.0.0.54:53 y el puerto 5355 de todas las interfaces. La red host-only usa IP estatica y aegisd un socket Unix; no se requiere un resolvedor para la operacion local comprobada.

Se respaldo la configuracion en `/root/aegis-resolved-pre-hardening-20261119`. Se detuvieron y deshabilitaron `systemd-resolved.service`, `systemd-resolved-varlink.socket` y `systemd-resolved-monitor.socket`; `/etc/resolv.conf` quedo como archivo regular sin servidor DNS y la linea hosts de `/etc/nsswitch.conf` sin resolve y dns. No se proporciona resolucion DNS de nombres externos.

El procedimiento verifico ausencia de listeners TCP y UDP en puertos 53 y 5355, networkd, Dropbear y aegisd activos y resolucion de localhost. La salida comunicada de `aegis-audit` v1.1, con `PUERTOS_AUTORIZADOS=192.168.56.111:22:dropbear`, fue `PASS=8 FAIL=0 WARN=0`, codigo 0. El informe indicado es `/root/aegis-audit-aegis-auth-20261119-143419.txt`; falta copiarlo al repositorio porque la VM sufrio un kernel panic antes de recuperarlo. No se modifico el auditor.

## Reversion

Desde una sesion root en la VM experimental y tras comprobar el respaldo y `estado-unidades.txt`:

```sh
b=/root/aegis-resolved-pre-hardening-20261119
test -f "$b/nsswitch.conf" && test -L "$b/resolv.conf" || exit 1
cp -a --remove-destination "$b/nsswitch.conf" /etc/nsswitch.conf
cp -a --no-dereference --remove-destination "$b/resolv.conf" /etc/resolv.conf
systemctl enable --now systemd-resolved.service systemd-resolved-varlink.socket systemd-resolved-monitor.socket
```

La reversion vuelve a abrir los sockets DNS/LLMNR y puede hacer fallar CAT-RED-01. No corrige el kernel panic.
