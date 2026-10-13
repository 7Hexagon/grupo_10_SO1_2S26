# ia/registro-ia.md

| Fecha | Herramienta | Parte / tarea | Propósito | Qué verificó, adaptó o descartó el grupo |
| --- | --- | --- | --- | --- |
| 2026-10-12 | Claude | Parte 1 / Ejercicio 1 | Guía paso a paso para crear la VM en VirtualBox, instalar Rocky, preparar el disco de LFS y verificar el anfitrión | Se adaptaron los comandos del enunciado de /dev/sdb a /dev/sda después de verificar con lsblk -f; se ejecutó y comprobó cada paso con su salida (findmnt, echo $LFS, umask, reinicio) |
| 2026-10-12 | ChatGPT | Parte 1 / Ejercicio 1, Paso 4 | Orientación para verificar los requisitos del anfitrión mediante version-check.sh | Se revisaron los resultados, se identificó el problema con yacc, se aplicó la corrección y se volvió a ejecutar el script hasta obtener todos los resultados correctos |
