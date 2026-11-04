# Bloque 1 — Historial y traspaso a Juan

**Responsable del bloque:** Lucas Heidebrecht
**Fecha:** 03/11/2026 22:30 – 04/11/2026 10:04 (snapshot final)
**Libro:** LFS 13.1-systemd
**Estado:** toolchain completa y chroot accesible (criterio de "hecho" del Bloque 1 cumplido)

---

## 1. Resumen

| Capítulo | Qué se hizo | Estado |
| --- | --- | --- |
| 3 | Fuentes y parches descargados y verificados con `md5sum` (94/94 OK) + versiones de seguridad | ✅ |
| 4 | Estructura de `$LFS`, usuario `lfs` y su entorno | ✅ |
| 5 | Compilador cruzado: binutils (paso 1), GCC (paso 1), cabeceras de Linux, glibc y libstdc++ | ✅ |
| 6 | 17 herramientas temporales (M4 → GCC paso 2) | ✅ |
| 7.2 – 7.4 | Dueño cambiado a root, sistemas virtuales montados, ingreso al chroot | ✅ |
| 7.5 – 7.13 | Directorios, archivos esenciales, gettext, bison, perl, Python, texinfo, util-linux, limpieza | ⏳ **Sigue Juan** |

---

## 2. Historial de pasos

### Capítulo 3 — Fuentes

```bash
mkdir -v $LFS/sources && chmod -v a+wt $LFS/sources
cd $LFS/sources
wget https://www.linuxfromscratch.org/lfs/view/13.1-systemd/wget-list-systemd
wget https://www.linuxfromscratch.org/lfs/view/13.1-systemd/md5sums
wget --input-file=wget-list-systemd --continue --directory-prefix=$LFS/sources
md5sum -c md5sums     # 94 archivos: todos OK
```

**Versiones de seguridad** (avisos sa-13.1 de paquetes de LFS), descargadas en `$LFS/sources` y **no usadas todavía**. Corresponden al capítulo 8:

| Paquete | Libro | Corregida | Avisos |
| --- | --- | --- | --- |
| expat | 2.8.3 | 2.9.0 | 022, 045, 064 |
| Python | 3.14.7 | 3.14.8 | 044 (088/089 requieren 3.15.2) |
| OpenSSL | 4.0.1 | 4.0.3 | 021, 046 |
| util-linux | 2.42.2 | 2.42.4 | 048 |
| xz | 5.8.3 | 5.8.4 | 024 |
| pcre2 | 10.47 | 10.49 | 023, 047 |
| patch | 2.8 | 2.8.1 | 087 |
| vim | 9.2.1025 | 9.2.1201 | 065, 086 |
| glibc | 2.44 + `upstream_fixes-1` | revisar si hay parche `-2` | 043 |

Checksums SHA-256 de estas versiones: `evidencias/parte1/sources-seguridad.txt`.

### Capítulo 4 — Usuario `lfs`

```bash
mkdir -pv $LFS/{etc,var} $LFS/usr/{bin,lib,sbin}
for i in bin lib sbin; do ln -sv usr/$i $LFS/$i; done
mkdir -pv $LFS/lib64 $LFS/tools
groupadd lfs
useradd -s /bin/bash -g lfs -m -k /dev/null lfs
chown -v lfs $LFS/{usr{,/*},var,etc,tools,lib64}
```

Entorno de `lfs` (`~/.bash_profile` y `~/.bashrc`) según el libro, más `MAKEFLAGS=-j4`. Verificado: `LFS=/mnt/lfs`, `LFS_TGT=x86_64-lfs-linux-gnu`, `PATH=/mnt/lfs/tools/bin:/usr/bin`, `umask 0022`.

### Capítulo 5 — Compilador cruzado

| Sección | Paquete | Verificación |
| --- | --- | --- |
| 5.2 | binutils 2.47 (paso 1) | `x86_64-lfs-linux-gnu-as` y `-ld` en `$LFS/tools/bin` |
| 5.3 | GCC 16.2.0 (paso 1) | `x86_64-lfs-linux-gnu-gcc (GCC) 16.2.0` |
| 5.4 | Cabeceras de Linux 7.1.8 | `asm`, `drm`, etc. en `$LFS/usr/include` |
| 5.5 | glibc 2.44 | Prueba del compilador OK (ver abajo) |
| 5.6 | libstdc++ | `libstdc++.so.6.0.36` en `$LFS/usr/lib` |

**Prueba del compilador (5.5):**

```
[Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
/mnt/lfs/lib/../lib/Scrt1.o succeeded
/mnt/lfs/lib/../lib/crti.o succeeded
/mnt/lfs/lib/../lib/crtn.o succeeded
```

### Capítulo 6 — Herramientas temporales

M4, Ncurses, Bash, Coreutils, Diffutils, File, Findutils, Gawk, Grep, Gzip, Make, Patch, Sed, Tar, Xz, binutils (paso 2) y GCC (paso 2). Todas instaladas en `$LFS/usr`. `sh → bash` y `cc → gcc` creados.

### Capítulo 7.2 – 7.4 — Chroot

```bash
# como root
chown --from lfs -R root:root $LFS/{usr,lib,var,etc,bin,sbin,tools,lib64}
mkdir -pv $LFS/{dev,proc,sys,run}
# montajes de 7.3 (ver sección 4 de este documento)
# ingreso de 7.4
```

Resultado: prompt `(lfs chroot) I have no name!:/#` (normal: `/etc/passwd` todavía no existe).

---

## 3. Problemas encontrados y soluciones

| # | Problema | Causa | Solución |
| --- | --- | --- | --- |
| 1 | Un paquete no se descargó (95 de 96 archivos) | `ftpmirror.gnu.org` rechazó la conexión (`Connection refused`) | Se descargó desde `ftp.gnu.org` reemplazando el host en la URL |
| 2 | `configure` de Diffutils (6.6) falló: `cannot run test program while cross compiling` | En compilación cruzada `configure` no puede ejecutar la prueba de `strcasecmp` | Agregar `gl_cv_func_strcasecmp_works=y` al `configure`, como indica el libro |
| 3 | Los paquetes de la lista del libro tienen avisos de seguridad posteriores | El libro 13.1 es anterior a los avisos sa-13.1 | Versiones corregidas descargadas para usarlas en el capítulo 8 |

---

## 4. Indicaciones para Juan — dónde seguir

### Antes de empezar

1. **Snapshots:** `00-host-listo-version-check-ok`, `01-fuentes-y-usuarios-lfs` y `02-toolchain-chroot-ok` (tomado el 04/11/2026 10:04). Si algo se rompe, volvé al `02`.
2. **Los nombres `sda`/`sdb` cambian entre reinicios.** El disco de LFS se monta por UUID desde `fstab`, pero antes de cualquier comando sobre discos, ejecutá `lsblk -f`.
3. **Usá tmux** (ya está instalado) para que una compilación no se corte si se cae el SSH: `tmux new -s lfs`. Para volver: `tmux attach -t lfs`.

### Volver a entrar al chroot (después de cada reinicio)

Los montajes del 7.3 **no sobreviven a un reinicio**. Cada vez que enciendas la VM, como root (`sudo -i`):

```bash
echo $LFS                     # tiene que decir /mnt/lfs
findmnt /mnt/lfs              # el disco de LFS montado

mount -v --bind /dev $LFS/dev
mount -vt devpts devpts -o gid=5,mode=0620 $LFS/dev/pts
mount -vt proc proc $LFS/proc
mount -vt sysfs sysfs $LFS/sys
mount -vt tmpfs tmpfs $LFS/run
if [ -h $LFS/dev/shm ]; then
  install -v -d -m 1777 $LFS$(realpath /dev/shm)
else
  mount -vt tmpfs -o nosuid,nodev tmpfs $LFS/dev/shm
fi

chroot "$LFS" /usr/bin/env -i \
  HOME=/root TERM="$TERM" PS1='(lfs chroot) \u:\w\$ ' \
  PATH=/usr/bin:/usr/sbin MAKEFLAGS="-j$(nproc)" TESTSUITEFLAGS="-j$(nproc)" \
  /bin/bash --login
```

Para salir y apagar sin problemas:

```bash
exit
umount -v $LFS/dev/shm $LFS/dev/pts $LFS/{sys,proc,run,dev}
shutdown -h now
```

### Próximo paso: capítulo 7.5 del libro

Dentro del chroot, seguí el libro desde la **sección 7.5**:

| Sección | Qué hace |
| --- | --- |
| 7.5 | Crea la jerarquía de directorios (`/etc`, `/home`, `/var`...) |
| 7.6 | Crea `/etc/passwd`, `/etc/group` y los logs; después de esto desaparece el "I have no name!" |
| 7.7 – 7.12 | Gettext, Bison, Perl, Python, Texinfo y util-linux (temporales) |
| 7.13 | Limpieza y **snapshot recomendado** (por ejemplo, `03-cap7-completo`) |

Después sigue el **capítulo 8 (sistema base)**, que es tu tarea del 06/11.

### Notas para el capítulo 8

- **Versiones de seguridad:** decidí si usás las versiones corregidas de la tabla de la sección 2 en lugar de las del libro, y documentalo en el diario.
- **Python 3.14.8:** el parche `Python-3.14.7-openssl_4-1.patch` es para la 3.14.7. Si usás la 3.14.8, verificá que aplique (o si ya no hace falta) antes de compilar.
- **Logs:** guardá la salida de cada paquete con `| tee` y anotá los errores en el momento. Sirven para el diario y para el informe.

---

## 5. Evidencias del bloque

| Evidencia | Ubicación |
| --- | --- |
| Verificación de fuentes (94/94 OK) | `evidencias/parte1/md5sums-check.txt` |
| SHA-256 de las versiones de seguridad | `evidencias/parte1/sources-seguridad.txt` |
| Logs de compilación (cap. 5 y 6) y notas | `evidencias/parte1/logs/` |
| Snapshot antes de la toolchain | `evidencias/parte1/snapshot-01.jpg` |
| Chroot accesible (hostname y fecha visibles) | `evidencias/parte1/chroot-ok.png` |
| Snapshot al cerrar el bloque | `evidencias/parte1/snapshot-02.png` |
