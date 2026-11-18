# Bloque 3 — Resumen y traspaso a Juan

**Responsable:** Lucas Heidebrecht
**Fecha:** 17/11/2026
**Estado:** Bloque 3 completo. Sigue el Bloque 4 (hardening, auditoría final, informe y defensa).
**Commits:** `e95b7a9`, `e12fa63`, `4270b5c`

---

## 1. Qué se hizo

| Tarea | Resultado | Dónde está |
| --- | --- | --- |
| Revisión del commit del 16/11 y del código de `aegisd` | Código comprendido; limitaciones documentadas | `auditoria/postura-inicial.md` |
| Fichas de los 8 controles | 6 del catálogo + 2 propios, con condición, verificación y reglas PASS/FAIL/WARN | `controles/fichas-controles.md` |
| Herramienta de auditoría | `aegis-audit` v1.1, en Bash, solo lectura | `auditoria/aegis-audit` |
| Auditoría inicial | **PASS=6 · FAIL=2 · WARN=0** | `auditoria/inicial/aegis-audit-aegis-auth-20261117-222143.txt` |
| Postura inicial | 7 hallazgos con área, estado, evidencia y riesgo | `auditoria/postura-inicial.md` |
| Verificación de código y binario | El código del repo es el que corre en Aegis (hashes comprobados) | `src/aegisd/README.md` |
| Arranque automático | Confirmado: tras reiniciar, `aegis-init` lanza Dropbear y `aegisd` | `auditoria/postura-inicial.md` |
| `aegisd` v3 | Versión legible de la v2, mismo comportamiento (32 pedidos idénticos), **instalada en Aegis** | `src/aegisd/aegisd.c`, `src/aegisd/README.md` |

### Los 8 controles

| ID | Qué comprueba | Inicial |
| --- | --- | --- |
| CAT-IDN-01 | Solo `root` tiene UID 0 | PASS |
| CAT-IDN-02 | Ninguna cuenta con contraseña vacía | PASS |
| CAT-PER-01 | Permisos de `passwd`, `group`, `shadow`, `gshadow` | PASS |
| CAT-PER-02 | Sin escritura global; `/tmp` y `/var/tmp` en 1777 | PASS |
| CAT-SSH-01 | Dropbear rechaza a root (opción `-w`) | **FAIL** |
| CAT-RED-01 | Solo escucha `192.168.56.110:22` (Dropbear) | PASS |
| PROP-AEGIS-01 | Base de usuarios 0600, formato válido, sin texto plano | **FAIL** |
| PROP-AEGIS-02 | El log de `aegisd` no contiene tokens ni hashes | PASS |

**Cambio respecto de la propuesta:** CAT-SRV-01 y CAT-LOG-02 dependen de systemd y journald, que no existen. Se reemplazaron por controles verificables, y está justificado en las fichas.

---

## 2. Estado actual de Aegis

- **VM:** `Aegis-Auth-demo`, IP `192.168.56.110`, PID 1 `aegis-init`, kernel 7.1.8.
- **Dropbear:** PID 73, escucha en `192.168.56.110:22`, **acepta root** (sin `-w`).
- **`aegisd` v3:** PID 1657, arrancado a mano con `setsid` el 17/11 a las 22:39. Al próximo reinicio lo arranca `aegis-init`, como siempre.
- **Respaldo de la v2:** `/root/aegisd-respaldo-v2-20261117`.
- **Acceso de Lucas:** clave propia `id_ed25519_aegis`, cargada en `/root/.ssh/authorized_keys`.

---

## 3. Qué tiene que corregir el hardening

| # | Hallazgo | Medida | Control que debería pasar a PASS |
| --- | --- | --- | --- |
| 1 | Root entra directo por SSH | Usuario administrador con las claves de los dos + `-w` en Dropbear | CAT-SSH-01 |
| 2 | Cuenta `lab_auditor` (rol `auditor`) residuo de la v1 | Eliminarla de la base | PROP-AEGIS-01 |
| 3 | Autologin de root en la consola | Quitar `--autologin root`, **solo si el login con contraseña funciona** (ver advertencia) | — (fuera de los 8; documentar) |
| 4 | `aegisd` corre como root | Fuera del alcance: requiere rediseñar el servicio. Se documenta como limitación | — |

### Antes de empezar

1. **Snapshot de la VM Aegis** antes de tocar nada (por ejemplo, `aegis-03-pre-hardening`).
2. Cada cambio va con su procedimiento de reversión en `config/hardening/`, como pide el TP.

### Medida 1 — SSH sin root

> ⚠️ **Riesgo de quedarse sin acceso.** El 16/11, `login` rechazaba a root sin pedir contraseña (faltaba `/etc/pam.d/`). Si `su` tampoco funciona, después de agregar `-w` no habría forma de pasar de administrador a root por SSH. **Probá `su` antes de activar `-w`**, y mantené la consola con autologin hasta comprobar que todo funciona.

Orden recomendado (como root en Aegis):

```bash
# 1) Contraseña de root (hace falta para su)
passwd root

# 2) Usuario administrador con las claves autorizadas actuales (la tuya y la de Lucas)
useradd -m -s /bin/bash -G wheel admin_aegis
passwd admin_aegis
install -d -m 700 -o admin_aegis -g admin_aegis /home/admin_aegis/.ssh
install -m 600 -o admin_aegis -g admin_aegis /root/.ssh/authorized_keys /home/admin_aegis/.ssh/authorized_keys
```

3) **Probá, sin cerrar la sesión de root**, desde Windows:

```powershell
ssh -i "$env:USERPROFILE\.ssh\id_ed25519_lfs" admin_aegis@192.168.56.110
su -          # tiene que pedir la contraseña de root y dar un prompt de root
```

4) **Solo si el paso 3 funcionó**, agregá `-w` a la línea de Dropbear en `/usr/sbin/aegis-init`:

```bash
cp -p /usr/sbin/aegis-init /root/aegis-init-antes-hardening-$(date +%Y%m%d)
sed -i 's|/usr/sbin/dropbear -F -E -s -j -k |/usr/sbin/dropbear -F -E -s -j -k -w |' /usr/sbin/aegis-init
grep dropbear /usr/sbin/aegis-init
```

El cambio aplica al reiniciar, porque Dropbear lo arranca el init. Para aplicarlo sin reiniciar, se detiene el Dropbear principal (el PID que escucha, hoy el 73) y se lanza con la línea nueva. Hacelo desde la **consola** de la VM, no por SSH, porque se corta la conexión.

**Reversión:** restaurar `/root/aegis-init-antes-hardening-<fecha>` y reiniciar Dropbear.

### Medida 2 — Eliminar `lab_auditor`

```bash
cp -p /var/lib/aegisd/users /root/users-antes-hardening-$(date +%Y%m%d)
sed -i '/^lab_auditor /d' /var/lib/aegisd/users
stat -c '%a %U' /var/lib/aegisd/users    # tiene que seguir en 600 root
wc -l /var/lib/aegisd/users              # una línea menos
```

`aegisd` lee la base en cada pedido, así que no hace falta reiniciarlo.

**Reversión:** `cp -p /root/users-antes-hardening-<fecha> /var/lib/aegisd/users`.

### Medida 3 — Autologin (opcional, con cuidado)

Quitar `--autologin root` de la línea de `agetty` en `aegis-init`, **solo si probaste que el login de la consola funciona con contraseña**. Si no funciona, dejalo y documentalo como limitación: es preferible a dejar la VM sin acceso por consola para la defensa.

---

## 4. Auditoría final y comparación

Usá **la misma versión** de la herramienta (v1.1), sin cambios, para que la comparación sea válida:

```powershell
scp -O -i "$env:USERPROFILE\.ssh\id_ed25519_lfs" auditoria/aegis-audit admin_aegis@192.168.56.110:/tmp/
```

En Aegis, como root (`su -`):

```bash
bash /tmp/aegis-audit -o /root
```

Guardá el informe en `auditoria/final/` y armá la comparación:

| ID | Inicial | Final | Medida aplicada |
| --- | --- | --- | --- |
| CAT-SSH-01 | FAIL | ? | `-w` en Dropbear + `admin_aegis` |
| PROP-AEGIS-01 | FAIL | ? | Eliminación de `lab_auditor` |
| (los otros 6) | PASS | ? | — (tienen que seguir en PASS) |

**Notas sobre la herramienta:**
- **CAT-PER-02** recorre todo el disco: tarda de 1 a 2 minutos.
- **CAT-SSH-01** puede mostrar dos PID de Dropbear: el principal y el de tu sesión. Los dos tienen que tener `-w`. La sesión abierta antes del cambio conserva la línea vieja, así que **auditá desde una sesión nueva**.
- Si un control da **WARN**, no es PASS: hay que revisar por qué (falta un archivo o un permiso).

---

## 5. Pendientes del grupo

| Tarea | Responsable | Fecha |
| --- | --- | --- |
| Hardening (medidas 1 a 3) y reversión documentada | Juan | Jue 19/11 |
| Auditoría final y comparación | Juan (Lucas acompaña) | Jue 19/11 |
| Sección de la auditoría en el manual técnico | Lucas | Mié 18/11 |
| Informe técnico y manual | Juan | Jue 19/11 |
| Consultar a la cátedra sobre systemd y Xorg (Parte 1, regla del 80 %) | Los dos | Mié 18/11 |
| Snapshot de la defensa y ensayo | Los dos | Jue 19/11 |
| Corregir el diario del 16/11: tabla y participantes, y unificar el nombre de Git (`Hexagon` → `Juan Rojas`) | Juan | Mié 18/11 |
