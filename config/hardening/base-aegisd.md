# Bloque 4: base de usuarios aegisd

Fecha: 18/11/2026. Estado: **medida aplicada; PROP-AEGIS-01 PASS en auditoría final**.

## Estado inicial y riesgo

`/var/lib/aegisd/users` contenía dos entradas: una era `lab_auditor` con rol `auditor`, incompatible con los roles permitidos por PROP-AEGIS-01. Los registros consultados muestran su creación, autenticación, validación y revocación como prueba el 16/11. No se observó actividad posterior de esa cuenta; el servicio se inició nuevamente al arrancar la VM el 18/11.

## Cambio y comprobación

Se copió la base anterior a `/root/aegisd-users-pre-hardening-20261118` y se comparó la copia. Se retiró exactamente la entrada `lab_auditor`, sin mostrar ni registrar sal o hash. La base quedó con una entrada; el directorio permanece `700 root:root`, y el archivo `600 root:root`. La entrada restante pasó la comprobación de cuatro campos, rol permitido y longitudes y caracteres hexadecimales de sal y hash. La auditoría intermedia original v1.1 registró PROP-AEGIS-01 como PASS; el conjunto quedó PASS=7, FAIL=1, WARN=0 porque CAT-SSH-01 sigue pendiente. No es una auditoría final.

## Reversión

Como root, restaurar `cp -p /root/aegisd-users-pre-hardening-20261118 /var/lib/aegisd/users` y verificar propietario, modo y formato sin mostrar su contenido. Restaurar la copia vuelve a introducir la cuenta residual y, salvo cambios adicionales, el incumplimiento original. No copiar la base ni su respaldo al repositorio.
