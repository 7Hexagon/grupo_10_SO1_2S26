# Fichas de controles — Aegis Auth

**Grupo 10:** Juan Rojas, Lucas Heidebrecht
**Herramienta:** `aegis-audit` (solo lectura: no modifica el sistema)
**Sistema auditado:** VM `Aegis-Auth-demo`, hostname `aegis-auth`, kernel 7.1.8

Aegis Auth autentica usuarios y entrega credenciales temporales. Por eso los controles se concentran en tres superficies: **las cuentas del sistema**, **el acceso remoto por SSH** y **los datos del propio servicio `aegisd`** (base de usuarios y registro).

## Resumen

| ID | Nombre | Área | Origen |
| --- | --- | --- | --- |
| CAT-IDN-01 | Única cuenta privilegiada | Identidad | Catálogo |
| CAT-IDN-02 | Sin contraseñas vacías | Identidad | Catálogo |
| CAT-PER-01 | Archivos de identidad protegidos | Permisos | Catálogo |
| CAT-PER-02 | Escritura global restringida | Permisos | Catálogo |
| CAT-SSH-01 | Root sin acceso remoto directo | SSH | Catálogo |
| CAT-RED-01 | Puertos autorizados | Red | Catálogo |
| PROP-AEGIS-01 | Base de usuarios protegida | Servicio | Propio |
| PROP-AEGIS-02 | Registro sin secretos | Servicio | Propio |

**Estados:**
- **PASS:** hay evidencia de que se cumple.
- **FAIL:** hay evidencia de que no se cumple.
- **WARN:** no se puede concluir (falta un archivo, un permiso o una dependencia). Nunca se trata como PASS.

### Cambios respecto de la propuesta del 30/10

La propuesta elegía CAT-IDN-03, CAT-SRV-01 y CAT-LOG-02. CAT-SRV-01 y CAT-LOG-02 dependen de systemd y journald, que el sistema no tiene (el PID 1 es `aegis-init`). Por eso, con un resultado garantizado como WARN, no aportarían información. Se reemplazaron por CAT-IDN-01, CAT-IDN-02 y CAT-PER-02, que se pueden verificar en el sistema real.

Los controles propios también se redefinieron. La propuesta suponía una cuenta `aegis` sin privilegios, pero el prototipo de `aegisd` corre como root con un socket local. Los nuevos controles propios comprueban lo que el servicio realmente protege: sus credenciales y su registro.

---

## CAT-IDN-01 — Única cuenta privilegiada

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | CAT-IDN-01 — Única cuenta privilegiada |
| Aplicabilidad y riesgo | Cualquier cuenta con UID 0 tiene todos los privilegios de root. En un sistema que administra credenciales de otros, una cuenta UID 0 extra (creada por error o por un atacante) permitiría leer la base de usuarios y suplantar al administrador sin aparecer como `root`. |
| Condición esperada | En `/etc/passwd`, solo la cuenta `root` tiene UID 0. |
| Verificación | `awk -F: '$3 == 0 {print $1}' /etc/passwd`. Puede ejecutarse sin privilegios. |
| PASS | La salida es exactamente `root`. |
| FAIL | Aparece cualquier otra cuenta con UID 0, o `root` no tiene UID 0. |
| WARN | `/etc/passwd` no existe o no se puede leer. |
| En Rocky | Se comprobaba igual; el comando es el mismo porque el formato de `/etc/passwd` no cambia. |

## CAT-IDN-02 — Sin contraseñas vacías

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | CAT-IDN-02 — Sin contraseñas vacías |
| Aplicabilidad y riesgo | Una cuenta con contraseña vacía permite entrar sin conocer ninguna credencial. Es incompatible con un sistema cuyo propósito es verificar identidades. |
| Condición esperada | Ninguna cuenta tiene vacío el campo de contraseña. Las cuentas bloqueadas (`!`, `*`, `!!`) se consideran correctas. |
| Verificación | `awk -F: '$2 == "" {print $1}' /etc/shadow`, y la misma prueba sobre `/etc/passwd` (por si la contraseña está ahí). Requiere root para leer `/etc/shadow`. |
| PASS | Ninguna cuenta aparece con el campo vacío en ninguno de los dos archivos. |
| FAIL | Alguna cuenta tiene el campo vacío. |
| WARN | `/etc/shadow` no existe y `/etc/passwd` usa `x` (no se puede saber dónde está la contraseña), o no hay permisos para leerlo. |
| En Rocky | Igual, aunque Rocky usa `/etc/shadow` desde la instalación. En el LFS hay que confirmar primero que exista. |

## CAT-PER-01 — Archivos de identidad protegidos

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | CAT-PER-01 — Archivos de identidad protegidos |
| Aplicabilidad y riesgo | Si `/etc/shadow` se puede leer sin ser root, cualquier usuario puede copiar los hashes e intentar romperlos fuera del sistema. Si `/etc/passwd` o `/etc/group` se pueden escribir, cualquiera puede darse UID 0 o entrar a un grupo privilegiado. |
| Condición esperada | Todos pertenecen a `root:root`. `/etc/passwd` y `/etc/group`: `644` o más restrictivo. `/etc/shadow` y `/etc/gshadow`: `600`, `640` o `000`. |
| Verificación | `stat -c '%a %U %G %n' /etc/passwd /etc/group /etc/shadow /etc/gshadow`. Puede ejecutarse sin privilegios. |
| PASS | Los cuatro existen, pertenecen a `root:root` y no superan los permisos indicados. |
| FAIL | Alguno tiene otro dueño o permisos más abiertos (por ejemplo, `shadow` con lectura para todos o `passwd` con escritura para el grupo). |
| WARN | Falta alguno de los cuatro archivos: no se puede afirmar que esté protegido algo que no existe. |
| En Rocky | Rocky usa `shadow` con permisos `000` y SELinux como capa extra. En el LFS no hay SELinux, así que los permisos clásicos son la única protección. |

## CAT-PER-02 — Escritura global restringida

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | CAT-PER-02 — Escritura global restringida |
| Aplicabilidad y riesgo | Un archivo con escritura global (por ejemplo, un script que ejecuta el init) permite que cualquier usuario cambie lo que hace el sistema. Un directorio con escritura global sin sticky bit permite que un usuario borre o reemplace archivos de otro. |
| Condición esperada | No hay archivos regulares con escritura global. Los directorios con escritura global tienen sticky bit; en particular `/tmp` y `/var/tmp` tienen modo `1777`. |
| Verificación | `find / -xdev \( -path /proc -o -path /sys -o -path /dev -o -path /run \) -prune -o \( -type f -perm -0002 -print \)`, `find / -xdev ... -type d -perm -0002 ! -perm -1000 -print` y `stat -c '%a' /tmp /var/tmp`. Requiere root para recorrer todo el disco. |
| PASS | Ninguno de los dos `find` devuelve resultados y `/tmp` y `/var/tmp` tienen `1777`. |
| FAIL | Aparece algún archivo o directorio en esas búsquedas, o `/tmp` o `/var/tmp` no tienen sticky bit. |
| WARN | `/tmp` o `/var/tmp` no existen, o la búsqueda no puede leer algún directorio por permisos. |
| En Rocky | Igual. Rocky monta `/tmp` como tmpfs; en el LFS es un directorio del disco, así que el sticky bit lo tiene que poner el sistema. |

## CAT-SSH-01 — Root sin acceso remoto directo

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | CAT-SSH-01 — Root sin acceso remoto directo |
| Aplicabilidad y riesgo | Aegis Auth se administra por SSH. Si root puede entrar directamente, comprometer una sola clave da control total, y en los registros no queda qué persona entró. |
| Condición esperada | El servidor SSH en ejecución rechaza el inicio de sesión de root. |
| Verificación | El sistema usa **Dropbear**, no OpenSSH, y Dropbear se configura con opciones de línea de comandos, no con un `sshd_config`. Se lee la línea de comandos del proceso en ejecución (`/proc/<pid>/cmdline`) y se busca la opción `-w` (rechazar root). Se audita la configuración **efectiva**, la del proceso que corre, no la de un archivo. Requiere root para leer `/proc` de otros procesos. |
| PASS | Dropbear está en ejecución con la opción `-w`. |
| FAIL | Dropbear está en ejecución sin `-w`. **Este es el estado inicial esperado:** hoy se entra como `root@192.168.56.110`. |
| WARN | No hay ningún proceso Dropbear en ejecución (no se puede verificar la configuración efectiva). |
| En Rocky | Con OpenSSH se usa `PermitRootLogin no` en `sshd_config` y se verifica con `sshd -T`. En el LFS hubo que adaptarlo a Dropbear: la opción equivalente es `-w` y la configuración efectiva se lee del proceso. |

## CAT-RED-01 — Puertos autorizados

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | CAT-RED-01 — Puertos autorizados |
| Aplicabilidad y riesgo | Cada puerto abierto es superficie de ataque. Aegis Auth solo debería exponer el servicio SSH de administración, y solo en la red del laboratorio. |
| Condición esperada | Los únicos puertos TCP en escucha son los de la lista autorizada: `192.168.56.110:22` (Dropbear). Cada uno pertenece a un proceso conocido. |
| Verificación | El LFS no tiene `ss` ni `netstat`, así que se leen `/proc/net/tcp` y `/proc/net/tcp6` (estado `0A` = escucha). El inodo de cada socket se busca en `/proc/<pid>/fd` para saber qué proceso lo abrió. Requiere root para leer los descriptores de otros procesos. |
| PASS | Todos los puertos en escucha están en la lista autorizada y pertenecen al proceso esperado. |
| FAIL | Hay un puerto en escucha fuera de la lista, o uno autorizado escucha en todas las interfaces (`0.0.0.0` o `::`) en lugar de la IP del laboratorio. |
| WARN | No se puede leer `/proc/net/tcp`, o un puerto en escucha no se puede asociar a ningún proceso. |
| En Rocky | Se usaba `ss -tlnp`. En el LFS se obtiene lo mismo leyendo `/proc` directamente, que es de donde `ss` saca la información. |

## PROP-AEGIS-01 — Base de usuarios protegida

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | PROP-AEGIS-01 — Base de usuarios protegida |
| Aplicabilidad y riesgo | `/var/lib/aegisd/users` es el activo más valioso del sistema: contiene los usuarios, sus roles y los datos para verificar sus contraseñas. Si otro usuario puede leerla, puede intentar romper los hashes fuera del sistema; si puede escribirla, puede darse el rol de administrador. |
| Condición esperada | El directorio `/var/lib/aegisd` tiene modo `700` y el archivo `users`, modo `600`; ambos pertenecen al dueño del proceso `aegisd`. Cada línea tiene el formato `usuario rol sal hash`, con una sal de 32 caracteres hexadecimales y un hash PBKDF2 de 64. Así se comprueba que no hay contraseñas en texto plano. |
| Verificación | `stat -c '%a %U' /var/lib/aegisd /var/lib/aegisd/users` y una validación con `awk` de que todas las líneas tienen 4 campos, el rol es `administrador` u `operador`, y la sal y el hash tienen la longitud y el formato esperados. **La herramienta nunca muestra el contenido del archivo**: solo informa cuántas líneas cumplen y cuáles no (por número de línea). Requiere root. |
| PASS | Permisos y dueño correctos y todas las líneas con formato válido. |
| FAIL | Permisos más abiertos, otro dueño, o alguna línea con otro formato (posible contraseña en texto plano o archivo alterado). |
| WARN | El archivo no existe (por ejemplo, todavía no se inicializó el administrador) o no hay permisos para leerlo. |
| Vínculo con el propósito | Comprueba directamente que el servicio de autenticación protege las credenciales que administra. |

## PROP-AEGIS-02 — Registro sin secretos

| Campo | Contenido |
| --- | --- |
| Identificador y nombre | PROP-AEGIS-02 — Registro sin secretos |
| Aplicabilidad y riesgo | Los registros se copian, se revisan y a veces se comparten para diagnosticar. Si `/var/log/aegisd.log` contuviera tokens, cualquiera que lo lea podría usar una sesión vigente; si contuviera hashes, podría intentar romperlos. Además, el registro es la evidencia de auditoría: si otros pueden modificarlo, deja de ser confiable. |
| Condición esperada | `/var/log/aegisd.log` tiene modo `600` y pertenece al dueño de `aegisd`. Ninguna línea contiene secuencias de 64 caracteres hexadecimales (tamaño de un token o de un hash) ni el texto `token=`. Todas las líneas siguen el formato `FECHA action=... user=... result=...`. |
| Verificación | `stat -c '%a %U' /var/log/aegisd.log`, `grep -cE '[0-9a-f]{64}\|token=' /var/log/aegisd.log` y una validación del formato de cada línea con `grep -vcE`. **La herramienta solo informa cantidades y números de línea**, nunca el contenido. Requiere root. |
| PASS | Permisos correctos, ninguna línea con secretos y todas con el formato esperado. |
| FAIL | Permisos más abiertos o alguna línea que contiene un posible token o hash. |
| WARN | El registro no existe o está vacío (no hay eventos que revisar), o no hay permisos para leerlo. |
| Vínculo con el propósito | Comprueba que el servicio no filtra las credenciales que emite. La revisión del código fuente (`src/aegisd/aegisd.c`) muestra que solo registra acción, usuario y resultado; este control lo verifica en el sistema real. |
