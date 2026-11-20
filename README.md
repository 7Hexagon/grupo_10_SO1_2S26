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

## Estado de Aegis Auth - 20/11/2026

Aegis Auth está construida sobre LFS 13.1-systemd. La VM recuperada arranca independientemente desde su disco con GRUB 2.14, kernel Linux 7.1.8 y systemd 261.2 como PID 1. Dispone de red host-only, administración SSH por clave mediante Dropbear y el servicio local de autenticación `aegisd`.

Se aplicó hardening y `aegis-audit` v1.1 verificó ocho controles: PASS=8, FAIL=0, WARN=0 después de instalar GRUB y nuevamente después de instalar Xorg. Se probó una sesión gráfica manual con Xorg 1.21.1.24, Fluxbox 1.3.7 y xterm 411, y el regreso a consola.

**Limitaciones:** la causa del kernel panic anterior no se determinó. La sesión gráfica se probó como root local; falta comprobarla después de un reinicio y como usuario sin privilegios. La entrada de recuperación de GRUB aún no se probó desde el disco. No se demostró estabilidad prolongada con red y entorno gráfico. El sistema todavía no representa el cumplimiento completo de todos los requisitos originales.

El código del servicio está en `src/aegisd/`; las configuraciones en `config/`, las auditorías en `auditoria/`, las capturas en `evidencias/` y el historial en `diario.md`. No se incluyen discos, ISO, credenciales ni bases de usuarios.
