# Plan de evidencias Aegis Auth Lab

Estado: informe historico y cinco capturas aportados por el usuario y revisados en el repositorio. Las imagenes muestran el resultado de cada operacion; su procedencia del sistema real fue informada por el usuario. No se repitio la auditoria historica.

| ID | Accion en la aplicacion real | Resultado observado | Archivo | Estado |
| --- | --- | --- | --- | --- |
| LAB-01 | Autenticar administrador y mostrar panel | Usuario y rol de administrador, autenticacion aceptada | evidencias/parte2/aegis-auth-lab/01-administrador.png | Completado |
| LAB-02 | Validar la credencial del administrador | Credencial valida segun aegisd | evidencias/parte2/aegis-auth-lab/02-validacion.png | Completado |
| LAB-03 | Solicitar revocacion como administrador | OK revocada, visible en el borde inferior | evidencias/parte2/aegis-auth-lab/03-revocacion.png | Completado |
| LAB-04 | Autenticar operador y mostrar panel | Usuario y rol de operador, autenticacion aceptada | evidencias/parte2/aegis-auth-lab/04-operador.png | Completado |
| LAB-05 | Solicitar revocacion como operador con credencial vigente | ERR permiso | evidencias/parte2/aegis-auth-lab/05-permiso-denegado.png | Completado |
| AUD-01 | Copiar el informe original 20261126-163627 sin modificarlo | Ocho controles, PASS=8 FAIL=0 WARN=0 | auditoria/aegis-lab/aegis-audit-entrega-20261126.txt | Completado |

La revision visual no encontro contrasenas, tokens ni datos personales visibles. Las capturas muestran cuentas de demostracion. Ninguno de los PNG contiene bloques de metadatos de texto. El SHA-256 local del informe es `c1f25222ab499f7bf54bd794ff2f85255c9ca2b3a9593e55e9e8dc53af2d5be0`; el usuario informo la coincidencia con el original de la VM. El informe identifica la ejecucion del 26/11/2026 a las 16:36:27.

## Procedimiento historico de copia manual

En la consola local de la VM, como root, comprobar el informe y revisar que no contenga datos sensibles antes de transferirlo. Estos comandos leen el original y crean una copia privada para el usuario administrativo; no cambian el original ni sus permisos:

```sh
src=/root/aegis-audit-aegis-auth-20261126-163627.txt
dst=/home/admin_aegis/aegis-audit-aegis-auth-20261126-163627.txt
test -f "$src" && sha256sum "$src" && cat "$src"
```

Si la revision detecta informacion sensible, detener el procedimiento. Si es apto para transferir, crear la copia privada con:

```sh
if test -f "$src" && test ! -e "$dst" && test ! -L "$dst"; then
  install -m 0600 -o admin_aegis -g root "$src" "$dst" && sha256sum "$dst"
else
  echo 'Informe ausente o destino existente; no se ha copiado'
fi
```

Si los hashes son distintos, detener el procedimiento. La copia en el directorio administrativo contiene informacion del sistema; conservarla privada y decidir su limpieza posteriormente sin tocar el original. No repetir `install` sobre un destino existente.

En PowerShell de Windows, desde el repositorio, traer la copia mediante la configuracion SSH documentada:

```powershell
cd C:\Users\achol\Codex-LFS\checkpoint-grupo10-20261116
scp -O -i "$env:USERPROFILE\.ssh\id_ed25519_lfs" -o IdentitiesOnly=yes -o HostKeyAlias=192.168.56.110 admin_aegis@192.168.56.111:/home/admin_aegis/aegis-audit-aegis-auth-20261126-163627.txt .\auditoria\aegis-lab\aegis-audit-aegis-auth-20261126-163627.txt
Get-FileHash -Algorithm SHA256 .\auditoria\aegis-lab\aegis-audit-aegis-auth-20261126-163627.txt
```

Comparar el hash de Windows con el del original; verificar fecha 2026-11-26 16:36:27, version, host, ocho controles y resumen. Si `scp -O` no funciona, conservar la copia y comunicar el error; no cambiar Dropbear ni abrir permisos para solucionarlo. No guardar la salida de `cat` con redireccion para reconstruir el archivo: podria cambiar sus bytes.

## Capturas manuales

Abrir en Windows la URL local con el proxy del repositorio activo y la VM real conectada. Crear `evidencias/parte2/aegis-auth-lab/` en el repositorio y usar la herramienta Recortes de Windows para capturar unicamente la ventana de Chrome. Antes de cada recorte cerrar ventanas ajenas y ocultar cualquier dato sensible. No capturar durante la introduccion de contrasenas. Guardar los cinco PNG con los nombres de la tabla; la sesion de administrador debe revocarse solo despues de la validacion y el operador debe tener una credencial vigente al probar la denegacion. Revisar cada imagen completa antes de incorporarla. Si falla una prueba, conservar el resultado real y no etiquetarla como exitosa.
