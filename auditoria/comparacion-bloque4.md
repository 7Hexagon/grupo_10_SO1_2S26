# Comparación de auditorías — Bloque 4

Las tres ejecuciones usaron `aegis-audit` v1.1 sin modificar los controles. PASS indica una condición verificada; FAIL, un incumplimiento observado; WARN, una condición no concluyente.

| Control | Inicial 17/11 | Intermedia 18/11 | Final 18/11 | Cambio o comprobación |
| --- | --- | --- | --- | --- |
| CAT-IDN-01 | PASS | PASS | PASS | Solo root tiene UID 0. |
| CAT-IDN-02 | PASS | PASS | PASS | Sin contraseñas vacías. |
| CAT-PER-01 | PASS | PASS | PASS | Archivos de identidad protegidos. |
| CAT-PER-02 | PASS | PASS | PASS | Sin escritura global injustificada. |
| CAT-SSH-01 | FAIL | FAIL | PASS | Cuenta administrativa comprobada y Dropbear con `-w`; root rechazado en conexión nueva. |
| CAT-RED-01 | PASS | PASS | PASS | Solo Dropbear escucha en la IP y puerto autorizados. |
| PROP-AEGIS-01 | FAIL | PASS | PASS | Se respaldó la base y se retiró la cuenta residual con rol no admitido. |
| PROP-AEGIS-02 | PASS | PASS | PASS | Registro de `aegisd` sin tokens ni hashes detectados. |
| **Total** | **6 PASS, 2 FAIL, 0 WARN** | **7 PASS, 1 FAIL, 0 WARN** | **8 PASS, 0 FAIL, 0 WARN** | |

Informes: [inicial](inicial/aegis-audit-aegis-auth-20261117-222143.txt), [intermedia](intermedia/aegis-audit-aegis-auth-20261118-175053.txt) y [final](final/aegis-audit-aegis-auth-20261118-181738.txt). La [salida de ejecución final](final/ejecucion-aegis-audit-20261118.txt) también registra la comprobación de procesos de Dropbear y la prueba de respuesta de `aegisd`.

Entre la intermedia y la final hubo otra ejecución con 7 PASS y 1 FAIL: una sesión SSH anterior conservaba los argumentos previos a `-w`. Se conservó su [salida](intermedia/aegis-audit-despues-ssh-20261118-ejecucion.txt); la sesión se cerró antes de la auditoría final.
