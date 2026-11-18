# Postura de seguridad inicial — Aegis Auth

**Fecha:** 17/11/2026 22:21 (-03:00)
**Sistema:** VM `Aegis-Auth-demo`, hostname `aegis-auth`, kernel 7.1.8, PID 1 `aegis-init`
**Herramienta:** `aegis-audit` v1.1, ejecutada como root
**Informe completo:** `auditoria/inicial/aegis-audit-aegis-auth-20261117-222143.txt`
**Responsable:** Lucas Heidebrecht

## Resultado de la auditoría inicial

**PASS = 6 · FAIL = 2 · WARN = 0**

| ID | Estado | Evidencia |
| --- | --- | --- |
| CAT-IDN-01 | PASS | Solo `root` tiene UID 0 |
| CAT-IDN-02 | PASS | 15 cuentas en `/etc/shadow`, ninguna con contraseña vacía |
| CAT-PER-01 | PASS | `passwd` y `group` 644, `shadow` y `gshadow` 400, todos `root:root` |
| CAT-PER-02 | PASS | Sin archivos ni directorios con escritura global injustificada; `/tmp` y `/var/tmp` en 1777 |
| CAT-SSH-01 | **FAIL** | Dropbear corre sin la opción `-w`: acepta el inicio de sesión de root |
| CAT-RED-01 | PASS | Único puerto en escucha: `192.168.56.110:22` (Dropbear) |
| PROP-AEGIS-01 | **FAIL** | Línea 2 de la base de usuarios con un rol no válido (ver hallazgo 2) |
| PROP-AEGIS-02 | PASS | `/var/log/aegisd.log` 600 root, 15 líneas, ninguna con tokens ni hashes |

## Hallazgos

| Área | Estado observado | Evidencia | Riesgo |
| --- | --- | --- | --- |
| Acceso remoto (SSH) | Root puede iniciar sesión directamente por SSH con clave | CAT-SSH-01 = FAIL; línea de comandos del proceso Dropbear (PID 73) sin `-w` | Comprometer una sola clave da control total, y los registros no muestran qué persona entró |
| Servicio del perfil (base de usuarios) | La base contiene la cuenta `lab_auditor` con el rol `auditor`, que el servicio instalado no reconoce | PROP-AEGIS-01 = FAIL (línea 2). El log registra su creación el 16/11 21:27:31 UTC (18:27 local), la misma hora del binario v1 (`/root/aegisd-lab-20261116/aegisd`). En el código v1 no hay validación de rol ni restricción de revocación; en el v2 sí | Una cuenta creada con una versión sin controles sigue presente. Si una versión futura aceptara ese rol, obtendría permisos que nadie le asignó a propósito |
| Consola local | La consola entra como root sin contraseña (`agetty --autologin root`) | `config/aegis-auth/aegis-init`, línea final | Cualquiera con acceso a la consola de la VM tiene control total sin autenticarse. Fue una decisión provisional para la demo |
| Servicio del perfil (privilegios) | `aegisd` corre como root (lo lanza el init) y su socket `/run/aegisd.sock` es `0600` de root | `src/aegisd/aegisd.c` (`chmod(SOCK,0600)`); `/usr/sbin/aegisd` de root | Un fallo en `aegisd` compromete todo el sistema. Los roles se comprueban dentro del servicio, pero no hay separación de privilegios entre usuarios del sistema |
| Red | No hay firewall: el único filtro es que Dropbear escucha solo en la IP del laboratorio | CAT-RED-01 = PASS; nftables no instalado | Si se abre otro servicio por error, no hay una segunda barrera |
| Servicios e inicio | No hay systemd: `aegis-init` arranca los servicios una vez y no los supervisa | PID 1 = `aegis-init` | Si `aegisd` o Dropbear se detienen, no se reinician solos |
| Registros | Los registros son archivos (`/var/log/aegisd.log`, `/var/log/aegis-dropbear.log`), sin journald ni rotación | PROP-AEGIS-02 = PASS (permisos y contenido correctos) | Crecen sin límite; no hay registro centralizado |

## Verificaciones adicionales del 17/11

- **El código del repositorio es el que corre en Aegis.** Los SHA-256 de `src/aegisd/aegisd.c` (`41bef99e…342747`) y `src/aegisd/aegisctl.c` (`33e26ffe…e000f6`) coinciden con los fuentes de `/root/aegisd-lab-v2-20261116/`. El binario instalado `/usr/sbin/aegisd` es idéntico al compilado en esa carpeta (SHA-256 `09dfafd7…a461d3`).
- **El arranque automático funciona.** Después del reinicio, `aegis-init` lanzó Dropbear (PID 73, escuchando en `192.168.56.110:22`) y `aegisd` (PID 74, socket `/run/aegisd.sock` creado a las 18:58 del 17/11). Esto cierra el pendiente "no probado tras reiniciar" del 16/11.

## Qué corrige el hardening (Bloque 4)

| Hallazgo | Medida propuesta | Control que debería pasar a PASS |
| --- | --- | --- |
| Root por SSH | Crear un usuario administrador con su clave y agregar `-w` a Dropbear en `aegis-init` | CAT-SSH-01 |
| Cuenta `lab_auditor` | Eliminarla de la base (o migrarla a `operador`), documentando la decisión | PROP-AEGIS-01 |
| Autologin de root | Quitar `--autologin root` del init | — (fuera de los 8 controles; documentar) |
| `aegisd` como root | Fuera del alcance: requiere cambiar el diseño del servicio. Se documenta como limitación | — |
