# Aegis Auth: entrega y demostracion local

## Estado y alcance

Grupo 10: Juan Rojas y Lucas Heidebrecht, segun README. Entrega: 11/12/2026, segun indicacion del grupo. La version original de la consigna no esta disponible en la ruta registrada; la lista de requisitos siguiente procede de un resumen historico, no de una nueva lectura. Antes de entregar, cotejar el PDF oficial vigente y confirmar integrantes y fecha con la catedra.

Aegis Auth es LFS 13.1-systemd sobre VirtualBox. La VM `Aegis-Auth-reparada-prueba` arranca desde disco mediante GRUB. `aegisd` autoriza credenciales y roles mediante `/run/aegisd.sock` restringido a root. El backend web corre como `aegislab`, bajo systemd, y se conecta al daemon mediante el relay restringido. El socket web es `/run/aegis-web/http.sock`; `ssh_http_forward.py` lo comunica con el proxy en Windows por SSH. Chrome accede solo a `http://127.0.0.1:8787/`. La base de usuarios y las credenciales permanecen en la VM. No se habilita Cloudflare ni acceso publico.

## Manual de uso y demostracion

Requisitos: VM funcional y respaldada; adaptadores ya configurados; Windows con Python 3, OpenSSH, clave autorizada `id_ed25519_lfs`, helper instalado en `/home/admin_aegis/ssh_http_forward.py` y cuentas demo existentes. No reconstruir el sistema ni restablecer credenciales durante la defensa. El repositorio contiene codigo y configuraciones, no una imagen instalable completa. Para reconstruir desde cero deben reunirse y verificarse instrucciones, fuentes y dependencias LFS; la reproducibilidad completa esta pendiente.

1. Arrancar la VM desde su disco, mostrar GRUB y elegir la entrada normal. En consola ejecutar:

```sh
cat /proc/1/comm
systemctl is-system-running
systemctl is-active aegisd.service aegis-lab-web.service aegis-lab-web.socket aegis-lab-relay.socket
ip -br addr
```

El relay es activado por socket y puede estar inactivo antes de una solicitud; comprobarlo despues de usar la web. No detener ni reiniciar servicios solo para la demostracion.
2. En consola local, con la sesion preparada para root, iniciar la interfaz con `startx -- -nolisten tcp`, mostrar Fluxbox y xterm, y retornar a la consola. La prueba historica fue como root local; no se afirma sesion grafica sin privilegios.
3. Como root, ejecutar la auditoria original, con la IP autorizada de esta VM:

```sh
env -u RAIZ -u PROC -u AEGIS_DUENO PUERTOS_AUTORIZADOS=192.168.56.111:22:dropbear bash /home/admin_aegis/aegis-audit-v1.1-original -o /root
```

Mostrar el resumen y guardar la ruta impresa. No cambiar los controles ni interpretar WARN como PASS. Confirmar primero que el archivo y la IP siguen siendo los esperados.
4. En PowerShell de Windows, desde el repositorio:

```powershell
cd C:\Users\achol\Codex-LFS\checkpoint-grupo10-20261116
python src\aegis-auth-lab\windows_ssh_proxy.py
```

Mantener la ventana abierta y abrir `http://127.0.0.1:8787/` en Chrome. Ingresar como administrador de demostracion, validar y revocar su credencial; despues iniciar como operador, validar y solicitar revocacion: se espera `ERR permiso`. No mostrar claves ni tokens. Cerrar el proxy con Ctrl+C. La revocacion de administrador invalida esa credencial; autenticar de nuevo para repetir. Cerrar la sesion web del operador no revoca su token.

### Alternativa si falla el navegador

En consola local root, `aegisctl` accede al socket protegido. La entrada de secretos es por stdin y la salida de autenticacion muestra un token: no proyectar ni registrar la consola durante estos pasos. `stty -echo` evita mostrar la clave mientras se escribe; restaurar con `stty echo` si se interrumpe. En una consola privada:

```sh
stty -echo; aegisctl autenticar admin_demo; stty echo
```

Comprobar previamente su ubicacion con `command -v aegisctl`. Escribir la clave y Enter; guardar temporalmente el token mostrado de forma privada. Ejecutar `aegisctl validar`, pegar el token y Enter; ejecutar `aegisctl revocar`, pegar primero el token administrador y despues el token a revocar, cada uno seguido de Enter. Para comprobar la denegacion, autenticar `operador_demo`, ejecutar `aegisctl revocar` e ingresar dos veces el token de operador: se espera `ERR permiso`. No pegar tokens en shell, historial, documentacion o chat. El socket exige permisos root y las credenciales vencen; no confundir un token vencido con una denegacion por rol.

## Seguridad y recuperacion

El servicio conserva decisiones de autorizacion en aegisd; los tokens son efimeros y `CLOCK_BOOTTIME` se usa para vencimiento. La web tiene controles Host/Origin, cookie HttpOnly/SameSite y sesiones en memoria; el proxy local tiene limites de peticion y concurrencia. No es una instalacion HTTPS ni esta preparada para acceso publico. El confinamiento del relay y las defensas ante abuso requieren mas revision. El cierre web no revoca la credencial de operador.

Ante fallo del navegador, detener el proxy con Ctrl+C, comprobar `Get-NetTCPConnection -LocalPort 8787 -State Listen`, iniciar solo una instancia y verificar SSH, helper y socket; no matar procesos ajenos. Si falla la web, usar la alternativa anterior. Copias verificadas: documentacion en `config/aegis-auth-lab/README.md`; respaldo de recursos informado `/root/aegis-static-backup-c71294f006e34cf695589ee4972d7050`. Ante problemas de arranque consultar `config/systemd-experimental/README.md`; la entrada alternativa de GRUB no tiene prueba registrada y restaurar snapshots o MBR requiere evaluar el estado. No borrar ni restaurar respaldos a ciegas.

## Matriz de evidencias y pendientes

| Requisito | Evidencia disponible | Limite |
| --- | --- | --- |
| GRUB, arranque, systemd | `evidencias/parte1/grub-disco-20261119/`, `config/systemd-experimental/README.md` | 19/11; no verificado de nuevo en esta sesion |
| Servicios, SSH y root restringido | `diario.md`, `config/hardening/acceso-ssh.md`, `config/aegis-auth-lab/README.md` | Evidencias historicas; repetir en defensa |
| Host-only, NAT, DNS e Internet | `diario.md` avance 25/11, `config/aegis-auth-lab/README.md` | Historico; verificar conectividad antes de defensa |
| Xorg, Fluxbox, xterm | `config/xorg-minimo/README.md`, `evidencias/parte1/xorg-20261120/fluxbox-xterm.png` | 20/11, root local; sin prueba registrada como usuario no privilegiado |
| Autenticacion, validacion, roles y revocacion | `src/aegisd/README.md`, `config/aegis-auth-lab/README.md` y cinco capturas en `evidencias/parte2/aegis-auth-lab/` | Capturas aportadas por el usuario; no se repitio la prueba durante su revision |
| Vencimiento y salto de reloj | `pruebas/aegis-auth-lab/test-clock-isolated.py`, `src/aegisd/README.md` | Prueba aislada historica; no repetir contra datos reales |
| Auditoria ocho controles | `auditoria/aegis-lab/aegis-audit-entrega-20261126.txt`, `auditoria/aegis-lab/socket-unix-reinicio-20261126.txt`, `controles/fichas-controles.md` | Ejecucion historica 26/11 a las 16:36:27; SHA-256 local coincide con el original segun el usuario |
| Proxy reforzado | `src/aegis-auth-lab/test_windows_ssh_proxy.py` | Pruebas simuladas; Chrome confirmado por usuario antes de retirar codigo HTTPS; repetir con version final |
| Plan de pruebas por ID y evidencia, informe unificado, manual reproducible | Esta matriz y documentos enlazados | Son un inicio: faltan procedimientos, resultados obtenidos y evidencia trazable por cada requisito del PDF |
| Operaciones systemd, recursos graficos, estabilidad prolongada y desafio de auditoria | Ver documentacion anterior | No se dispone aqui de evidencia completa; no afirmar cumplimiento |
| Entrada de recuperacion GRUB, sesion grafica sin privilegios, causa del panic y divergencia del reloj | `config/systemd-experimental/README.md`, `config/xorg-minimo/README.md`, `diario.md` | Pendientes o causas desconocidas |

Revisar tambien snapshot previsto para el desafio de auditoria, duracion y reglas de defensa, integrantes y fecha contra la consigna vigente. No alterar el sistema o sus controles para mejorar el resultado.

El plan y los resultados de las cinco capturas de Chrome y del informe historico figuran en `docs/plan-evidencias-lab.md`. El informe y las imagenes fueron aportados por el usuario y revisados; no se realizo una nueva auditoria ni se repitieron las operaciones durante esa revision.
