# aegisd y aegisctl

Servicio local de autenticación de Aegis Auth. `aegisd` escucha en el socket Unix `/run/aegisd.sock` (permisos 0600, solo root) y `aegisctl` es el cliente de línea de comandos. Los tokens duran cinco minutos y viven en memoria: desaparecen si el servicio se reinicia. El primer administrador se inicializa localmente; las operaciones administrativas posteriores requieren su credencial.

El funcionamiento, las medidas de seguridad y las limitaciones conocidas están explicados en el comentario inicial de `aegisd.c`.

## Versiones

| Versión | Fecha | Cambios | Estado |
| --- | --- | --- | --- |
| v1 | 16/11/2026 18:27 | Prototipo inicial. No validaba roles ni restringía la revocación | Reemplazada. Dejó la cuenta `lab_auditor` (rol `auditor`) en la base: ver `auditoria/postura-inicial.md` |
| v2 | 16/11/2026 18:37 | Solo acepta los roles `administrador` y `operador`; solo un administrador puede revocar | Reemplazada; respaldo en `/root/aegisd-respaldo-v2-20261117` |
| **v3** | **17/11/2026 22:39** | **Versión legible de la v2:** nombres y comentarios en español, una función por operación, constantes con nombre y sin el código muerto de `VALIDATE`. **Mismo comportamiento** | Reemplazada el 26/11 por la corrección de vencimiento descrita abajo |

## Verificación de la v3

Antes de instalarla se compilaron la v2 y la v3 con un TTL de 2 segundos y se les envió la misma secuencia de 32 pedidos, que recorre todas las ramas: creación del administrador, usuarios repetidos, roles inválidos, nombres y claves fuera de rango, bytes nulos, permisos por rol, revocación y vencimiento.

- Las 32 respuestas fueron idénticas.
- Las 17 líneas del registro fueron idénticas (sin contar la fecha).
- La base de usuarios tiene el mismo formato y los mismos permisos (directorio 700, archivo 600).
- La v3 autentica correctamente a usuarios creados por la v2.

## Hashes históricos de la v3 instalada el 17/11/2026

| Archivo | SHA-256 |
| --- | --- |
| `src/aegisd/aegisd.c` (v3) | `7fbff0cf0b0f41da907bb1374c247d150a6dcb0361293fe41c3a15b4539a4d0f` |
| `/usr/sbin/aegisd` en Aegis (v3) | `c6b16b4b65ab56feead2f91cd4b9f8032205c3516d0595ca4bd8bcf3a5b5c33f` |
| `src/aegisd/aegisctl.c` | `33e26ffef0aab05e9ba7c841339e3a8960cde531bada5f36ee275ae3c1e000f6` |

## Compilar e instalar

```sh
gcc -Wall -Wextra -Werror -O2 aegisd.c -o aegisd -lcrypto
gcc -Wall -Wextra -Werror -O2 aegisctl.c -o aegisctl -lcrypto
```

Para reemplazar el servicio en funcionamiento (como root):

```sh
cp -p /usr/sbin/aegisd /root/aegisd-respaldo-$(date +%Y%m%d)    # respaldo
for p in /proc/[0-9]*; do [ "$(cat $p/comm 2>/dev/null)" = aegisd ] && kill ${p##*/}; done
rm -f /run/aegisd.sock          # aegisd no reemplaza un socket existente
install -m 755 aegisd /usr/sbin/aegisd
setsid /usr/sbin/aegisd </dev/null >>/var/log/aegisd-service.log 2>&1 &
```

Para volver atrás, se repiten los mismos pasos instalando el respaldo en lugar del binario nuevo.

## Advertencias

No guardar contraseñas, bases de usuarios ni tokens en el repositorio. La base (`/var/lib/aegisd/users`) y el registro (`/var/log/aegisd.log`) quedan solo en la VM.

## Corrección del vencimiento — 26/11/2026

La versión instalada compara los plazos con `CLOCK_BOOTTIME`, que incluye la suspensión y no retrocede al ajustar el reloj de calendario. `vence_unix` sigue siendo informativo para el cliente. El código v3 utilizaba `time(NULL)` para autorizar y un retroceso del reloj podía prolongar la validez. La regresión se reprodujo mediante una instancia aislada con el reloj de calendario simulado, sin alterar la hora de la VM. Con el código corregido el token venció; también pasaron las pruebas aisladas de autenticación, roles y revocación.

El binario anterior se conserva en `/root/aegis-clock-backup-20261126-retry/aegisd-old`. Reiniciar el servicio invalida las credenciales que estén en memoria. La primera instalación se revirtió porque el control del socket se hizo antes de que el proceso lo creara; el segundo intento incorporó una espera acotada.
