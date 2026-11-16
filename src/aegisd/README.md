# Prototipo local

Fuentes de la versión compilada y probada en la VM Aegis Auth el 16/11/2026. El servicio usa libcrypto y un socket Unix accesible por root. Los tokens duran cinco minutos y viven en memoria; desaparecen al reiniciar el servicio. El primer administrador se inicializa localmente; las operaciones administrativas posteriores requieren su credencial.

Compilación utilizada:

```sh
gcc -Wall -Wextra -Werror -O2 aegisd.c -o aegisd -lcrypto
gcc -Wall -Wextra -Werror -O2 aegisctl.c -o aegisctl -lcrypto
```

Es un prototipo para laboratorio: faltan la revisión de Lucas, las operaciones completas previstas por la propuesta, una interfaz de red propia y pruebas de persistencia del inicio automático. No guardar contraseñas, bases de usuarios ni tokens en el repositorio.
