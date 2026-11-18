# Bloque 4: acceso SSH administrativo

Fecha: 18/11/2026. Estado: **aplicado; CAT-SSH-01 PASS en auditoría final**.

## Estado inicial y riesgo

Dropbear escucha solo en `192.168.56.110:22`, con autenticación por clave y sin `-w`. El acceso SSH directo de root concentra todos los privilegios. El init conserva autologin de root en la consola como recuperación.

## Cambios y comprobaciones

Se creó la cuenta local `admin_aegis`, sin contraseña utilizable, con las claves públicas previamente autorizadas para Juan y Lucas. Su directorio `.ssh` tiene modo 0700 y `authorized_keys` modo 0600. Se comprobó que ambas copias de claves eran idénticas y Juan inició sesión como `admin_aegis` por SSH. La conexión con la clave de Lucas todavía no se comprobó.

Se preservaron el init y las claves autorizadas de root bajo `/root/aegis-pre-hardening-20261118/`. Se comprobó su correspondencia con los originales. No se cambiaron contraseñas. Se comprobó que `su` solicita contraseña en una terminal; Juan informó después una elevación exitosa a root antes de habilitar `-w`.

## Reversión

Desde una sesión root o la consola local, una vez verificado que no hay trabajo legítimo que dependa de la cuenta, eliminarla con `userdel -r admin_aegis`. Antes, revisar cualquier archivo que Lucas o Juan hayan creado en su directorio. La creación de la cuenta no requiere restaurar el init ni las claves de root. Si en una etapa posterior se modifica el init, recuperar el archivo respaldado con `cp -p /root/aegis-pre-hardening-20261118/aegis-init /usr/sbin/aegis-init` y comprobar `bash -n /usr/sbin/aegis-init`; esto por sí solo no cambia las opciones del Dropbear que ya está ejecutándose.

## Aplicación de CAT-SSH-01

Se verificó la consola de VirtualBox ejecutando un comando como root. Antes de aplicar el cambio se probó una instancia temporal de Dropbear con `-w`: la clave de Juan ingresó como `admin_aegis` y fue rechazada como root. Se preservaron `/root/aegis-init-antes-ssh-w-20261118` y `/root/aegis-status-antes-ssh-w-20261118`; luego se añadió `-w` al init, se inició Dropbear con las mismas opciones y se ajustó el reconocimiento del proceso en `aegis-status`.

Una conexión nueva por el puerto 22 ingresó como `admin_aegis`; otra con la misma clave como root recibió `Permission denied (publickey)`. El socket y el proceso de `aegisd` continuaban activos. Juan confirmó el acceso administrativo y la elevación con `su` después del cambio; Lucas todavía debe probar su propia clave.

Para revertir desde la consola root, mantener la consola disponible, restaurar ambos archivos de respaldo, terminar únicamente el proceso principal de Dropbear que escucha en el puerto 22 e iniciarlo con las opciones anteriores. No matar procesos por nombre ni cerrar sesiones existentes sin identificarlos. Los respaldos contienen las líneas anteriores; comprobar `bash -n` de los dos scripts antes de reiniciar el servicio. El snapshot previo está disponible como último recurso, sin restaurarlo automáticamente.
Para recuperar el acceso root por SSH desde la consola, verificar visualmente que el proceso principal es el indicado y ejecutar:

```sh
cp -p /root/aegis-init-antes-ssh-w-20261118 /usr/sbin/aegis-init
cp -p /root/aegis-status-antes-ssh-w-20261118 /usr/bin/aegis-status
bash -n /usr/sbin/aegis-init && bash -n /usr/bin/aegis-status
# Identificar el proceso principal (sin argumento -2); no matar sesiones hijas.
for f in /proc/[0-9]*/cmdline; do
  a=$(tr '\0' ' ' < "$f" 2>/dev/null) || continue
  case "$a" in
    '/usr/sbin/dropbear -F -E -s -j -k -w -p 192.168.56.110:22 -r /etc/dropbear/dropbear_ed25519_host_key ')
      echo "Proceso principal: ${f%/cmdline}" ;;
  esac
done
# Tras comprobar el PID mostrado, terminar solo ese proceso:
kill -TERM PID_COMPROBADO
/usr/sbin/dropbear -F -E -s -j -k -p 192.168.56.110:22 \
  -r /etc/dropbear/dropbear_ed25519_host_key </dev/null >>/var/log/aegis-dropbear.log 2>&1 &
```

La prueba funcional de `aegisd` envió una autenticación inválida al socket: respondió `ERR autenticacion` y registró el fallo. Las sesiones SSH anteriores al cambio conservaron temporalmente las opciones antiguas; Juan confirmó que las cerró. Antes de la auditoría se verificó que no quedaran procesos con esas opciones.