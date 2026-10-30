# Aegis Auth — Propuesta de distribución

2026-10-30 · Grupo 10

Grupo 10 propone **Aegis Auth**: una distribución LFS mínima que verifica la identidad de los usuarios del laboratorio y les entrega credenciales temporales según su rol, para que otros servicios no gestionen contraseñas.

## Ficha de propuesta

| Campo | Contenido |
| --- | --- |
| Grupo e integrantes | Grupo 10: Juan Rojas, Lucas Heidebrecht |
| Nombre | Aegis Auth |
| Propósito | Sistema mínimo que verifica la identidad de los usuarios del laboratorio y les entrega credenciales de acceso temporales, con permisos según su rol, para que otros servicios no gestionen contraseñas |
| Usuario objetivo | Administra: el encargado del laboratorio (rol admin), que gestiona las cuentas. Utilizan: los alumnos (rol user) y los servicios del laboratorio que validan credenciales |
| Hardware objetivo | VM x86_64 con 1–2 vCPU, 1 GB de RAM, 10 GB de disco y una interfaz en la red del laboratorio |
| Componentes principales | Del LFS base: Python 3 con su biblioteca estándar, systemd y journald. Desde BLFS: OpenSSH (administración remota) y nftables (firewall). La sesión gráfica de la Parte 1 se instala pero no arranca por defecto |
| Servicio principal | aegisd: programa propio, corre con la cuenta aegis (sin shell ni contraseña), escucha en el puerto TCP 8080 solo en la interfaz del laboratorio. Unidad aegisd.service escrita por el grupo. Sin TLS, documentado como limitación |
| Cómo se prueba | Desde Rocky, en la red del laboratorio y sin internet: iniciar sesión devuelve una credencial; consultarla responde con el usuario y su rol; cerrar sesión la invalida; un alumno que intenta borrar una cuenta es rechazado |
| Superficie y paquetes retirados | Sin servidor web, base de datos ni intérpretes extra. Se deshabilitan las unidades que no se usan. Queda expuesto: puerto 8080 (credenciales sin cifrar dentro del laboratorio) y puerto 22 (SSH) |
| Postura inicial esperada | Servicio: aegisd corre como root y puede modificar su configuración; riesgo de compromiso total. Red: el puerto 8080 escucha en todas las interfaces, incluida la NAT; exposición innecesaria. Registros: journald no persiste entre reinicios; se pierde el rastro de accesos fallidos |
| Controles del catálogo | CAT-IDN-03, CAT-PER-01, CAT-SSH-01, CAT-RED-01, CAT-SRV-01, CAT-LOG-02 |
| Controles propios | PROP-AEGIS-01: el archivo de cuentas pertenece a aegis, tiene permisos 0600 y solo guarda contraseñas cifradas con hash, nunca en texto plano. PROP-AEGIS-02: la cuenta aegis puede leer /etc/aegis pero no modificarlo, y solo puede escribir en /var/lib/aegis |
| Versión más simple | Sin gestión de cuentas por red: el admin edita las cuentas en un archivo por SSH. Se mantienen iniciar sesión, cerrar sesión, consultar credencial, la unidad systemd y los 8 controles |

## Funcionalidades

Aegis Auth ofrece siete funciones: cuatro para cualquier usuario con cuenta y tres exclusivas del administrador.

| Función | Qué hace | Quién |
| --- | --- | --- |
| Iniciar sesión | Recibe usuario y contraseña; si son correctos, entrega una credencial válida por 1 hora | Cualquiera con cuenta |
| Consultar credencial | Dice si una credencial sigue siendo válida y a qué usuario y rol pertenece. Es lo que usan los otros servicios del laboratorio | Cualquiera con credencial |
| Cerrar sesión | Invalida la credencial al instante, aunque no haya vencido | El dueño de la credencial |
| Cambiar contraseña | Cambia la propia contraseña e invalida todas las sesiones abiertas | El dueño de la cuenta |
| Crear cuenta | Da de alta un usuario con su rol | Admin |
| Editar cuenta | Cambia el rol, restablece la contraseña o bloquea la cuenta | Admin |
| Eliminar cuenta | Borra la cuenta y cierra todas sus sesiones | Admin |

Además, cada intento de inicio de sesión (exitoso o fallido) y cada acción del admin queda registrada en el registro del sistema, con fecha, usuario y origen. Después de 5 intentos fallidos en 10 minutos desde un mismo equipo, ese equipo queda bloqueado temporalmente.

## Roles y permisos

Hay dos roles; admin puede todo lo que puede user, más la gestión de cuentas.

| Acción | Sin credencial | user | admin |
| --- | --- | --- | --- |
| Iniciar sesión | Sí | Sí | Sí |
| Consultar credencial | No | Sí | Sí |
| Cerrar sesión | No | Sí (la propia) | Sí (la propia) |
| Cambiar contraseña | No | Sí (la propia) | Sí (la propia) |
| Ver lista de cuentas | No | No | Sí |
| Crear, editar o eliminar cuentas | No | No | Sí |

Reglas que protegen al sistema de errores del propio admin:

- El rol se consulta en cada pedido, no queda guardado en la credencial: si el admin le quita permisos a alguien, rige al instante.
- Un admin no puede eliminarse ni quitarse el rol a sí mismo, y no se puede eliminar al último admin.
- Bloquear, eliminar o cambiar la contraseña de una cuenta cierra todas sus sesiones.
- No existe ninguna cuenta por defecto: el primer admin lo crea root desde la consola del servidor.

## Cómo se interactúa

Todo se usa por la red del laboratorio con pedidos HTTP; en la demostración se hace con curl desde Rocky. En los ejemplos, el servidor Aegis tiene la IP 192.168.56.110.

1. **El admin crea una cuenta para un alumno.**

    ```bash
    curl -X POST http://192.168.56.110:8080/admin/usuarios \
         -H "Authorization: Bearer <credencial-del-admin>" \
         -d '{"usuario":"ana","contrasena":"...","rol":"user"}'
    ```

2. **El alumno inicia sesión** y recibe su credencial con el vencimiento.

    ```bash
    curl -X POST http://192.168.56.110:8080/login -d '{"usuario":"ana","contrasena":"..."}'
    # {"credencial":"Xk3p...","vence":"2026-11-20T15:00:00"}
    ```

3. **Otro servicio del laboratorio verifica la credencial** antes de dejar pasar al alumno.

    ```bash
    curl http://192.168.56.110:8080/me -H "Authorization: Bearer Xk3p..."
    # {"usuario":"ana","rol":"user"}
    ```

4. **El alumno cierra sesión.** Si repite el paso 3 con la misma credencial, el sistema responde 401.

    ```bash
    curl -X POST http://192.168.56.110:8080/logout -H "Authorization: Bearer Xk3p..."
    ```

5. **Prueba de permisos:** el alumno intenta borrar una cuenta y el sistema responde 403.

    ```bash
    curl -X DELETE http://192.168.56.110:8080/admin/usuarios/juan -H "Authorization: Bearer <credencial-de-ana>"
    ```

Los cinco pasos forman la prueba funcional del servicio en la defensa: no necesitan internet ni servicios externos.

## Respuestas del sistema

| Código | Cuándo | Ejemplo |
| --- | --- | --- |
| 200 | El pedido se cumplió | Inicio de sesión correcto, credencial válida |
| 201 | Se creó una cuenta | El admin da de alta a un alumno |
| 400 | Faltan datos o están mal formados | Inicio de sesión sin contraseña |
| 401 | No se pudo verificar la identidad | Contraseña incorrecta, credencial vencida o cerrada |
| 403 | La identidad es válida pero el rol no alcanza | Un alumno intenta eliminar una cuenta |
| 404 | La cuenta no existe | Editar un usuario borrado |
| 409 | La acción rompería una regla del sistema | Eliminar al último admin, crear un usuario repetido |
| 429 | Demasiados intentos fallidos | Sexto intento fallido en 10 minutos |

Un 401 nunca dice si falló el usuario o la contraseña, para no revelar qué cuentas existen.

## Las cinco preguntas de aprobación

| N.º | Pregunta | Respuesta |
| --- | --- | --- |
| 1 | ¿Se describe sin nombrar tecnologías? | Sí: verifica identidades y entrega credenciales temporales según el rol |
| 2 | ¿Qué dependencias de BLFS requiere? | Solo OpenSSH y nftables; el servicio usa Python 3, que ya viene en LFS |
| 3 | ¿Se prueba completo desde otra máquina, sin internet? | Sí: los cinco pasos con curl desde Rocky por la red del laboratorio |
| 4 | ¿Qué superficie genera y qué la cubre? | Puerto 8080 y el archivo de cuentas; los cubren PROP-AEGIS-01 y PROP-AEGIS-02, con apoyo de CAT-RED-01 y CAT-LOG-02 |
| 5 | ¿Cuál es la versión más simple? | Cuentas editadas por archivo vía SSH, sin gestión por red; mantiene el servicio, la unidad systemd y los 8 controles |
