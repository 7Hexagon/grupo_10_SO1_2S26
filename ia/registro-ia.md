# ia/registro-ia.md

| Fecha | Herramienta | Parte / tarea | Propósito | Qué verificó, adaptó o descartó el grupo |
| --- | --- | --- | --- | --- |
| 2026-10-12 | Claude | Parte 1 / Ejercicio 1 | Guía paso a paso para crear la VM en VirtualBox, instalar Rocky, preparar el disco de LFS y verificar el anfitrión | Se adaptaron los comandos del enunciado de /dev/sdb a /dev/sda después de verificar con lsblk -f; se ejecutó y comprobó cada paso con su salida (findmnt, echo $LFS, umask, reinicio) |
| 2026-10-12 | ChatGPT | Parte 1 / Ejercicio 1, Paso 4 | Orientación para verificar los requisitos del anfitrión mediante version-check.sh | Se revisaron los resultados, se identificó el problema con yacc, se aplicó la corrección y se volvió a ejecutar el script hasta obtener todos los resultados correctos |
| 2026-11-03 | Claude | Parte 1 / Bloque 1 (cap. 3 a 7.4) | Guía paso a paso para descargar y verificar fuentes, revisar avisos de seguridad, preparar el usuario lfs, construir la toolchain y entrar al chroot; redacción del documento de traspaso | Cada paso se verificó con su salida (md5sum, prueba del compilador, binarios instalados, prompt del chroot); el configure de Diffutils indicado inicialmente falló y se corrigió con gl_cv_func_strcasecmp_works=y; las versiones corregidas se tomaron de los avisos oficiales de LFS |
