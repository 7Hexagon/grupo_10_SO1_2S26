# Trabajo Práctico Integrador - Sistemas Operativos 1

**Grupo:** 10  
**Año:** 2026 - Segundo semestre

## Integrantes

- Juan Rojas
- Lucas Heidebrecht

## Entorno de trabajo

- Hipervisor: VirtualBox
- Sistema anfitrión de LFS: Rocky Linux 10.2 Minimal
- Memoria RAM: 8 GB
- Procesadores virtuales: 4
- Disco de Rocky Linux: 25 GB
- Disco dedicado a LFS: 30 GB
- Arranque: BIOS

## Linux From Scratch

- Manual: Linux From Scratch 13.1-systemd
- Edición: systemd
- Sitio oficial: https://www.linuxfromscratch.org/lfs/

## Organización

- `docs/`: documentación del proyecto.
- `config/`: archivos de configuración.
- `auditoria/`: resultados de auditorías.
- `controles/`: controles implementados.
- `pruebas/`: pruebas realizadas.
- `informes/`: informes técnicos.
- `evidencias/`: salidas y capturas de las distintas etapas.
- `ia/`: registro de utilización de inteligencia artificial.
- `diario.md`: seguimiento cronológico del trabajo.

## Estado de Aegis Auth — 16/11/2026

Aegis Auth arrancó en una VM independiente de Rocky mediante una ISO auxiliar de GRUB, con el kernel Linux 7.1.8 compilado desde LFS, raíz ext4 propia, init provisional y Bash. Se comprobó la identidad del sistema, la red host-only y el acceso SSH por clave mediante Dropbear. El comando `aegis-status` consulta los eventos disponibles de SSH.

Existe un prototipo local en C de `aegisd` y `aegisctl`: autentica usuarios de laboratorio, emite credenciales de cinco minutos y permite validarlas y revocarlas. Se probaron permisos por rol en una instancia aislada. El prototipo usa un socket local accesible por root; no ofrece todavía una API de red propia. Las contraseñas se almacenan derivadas con PBKDF2, no en texto plano.

**Pendiente:** systemd no es PID 1, no hay sesión gráfica Xorg y el arranque requiere la ISO auxiliar. La integración de `aegisd` al init y la persistencia de los registros de SSH están preparadas, pero faltan pruebas posteriores a un reinicio. El prototipo requiere revisión y continuación, incluida la coordinación con Lucas, responsable previsto del bloque 3. El sistema aún no cumple todos los requisitos originales.

El código está en `src/aegisd/`; las configuraciones y la configuración real del kernel están en `config/aegis-auth/`. No se incluyen discos, ISO, credenciales ni bases de usuarios.
