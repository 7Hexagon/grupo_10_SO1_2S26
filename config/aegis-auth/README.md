# Configuración conservada

- `kernel-config-7.1.8` es la configuración recuperada de `/boot/config-7.1.8` en la VM demostradora. Las fuentes corresponden a Linux 7.1.8 del conjunto de LFS 13.1-systemd: https://www.linuxfromscratch.org/lfs/view/13.1-systemd/chapter10/kernel.html.
- `grub-auxiliar.cfg` es el archivo local preparado para la ISO auxiliar. Sus argumentos coinciden con `/proc/cmdline` observado; no se volvió a extraer la configuración de la ISO utilizada.
- `aegis-init`, `aegis-status` y `fstab` son copias del sistema demostrador. El init incluye red host-only y el inicio preparado de `aegisd`. No se verificó su ejecución tras reiniciar.
- La captura de primer arranque precede a la corrección del control de trabajos y muestra sus advertencias originales. Faltan capturas posteriores de identidad, red, SSH y pruebas de autenticación.
