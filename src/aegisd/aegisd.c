/*
 * aegisd — servicio de autenticación de Aegis Auth
 * Grupo 10, Sistemas Operativos 1 (2026)
 *
 * Versión legible del prototipo v2 (16/11/2026). Mismo comportamiento,
 * mismo protocolo y mismos formatos de archivo que la versión original;
 * solo cambian los nombres, el formato y los comentarios.
 *
 * QUÉ HACE
 *   Escucha en un socket Unix local (/run/aegisd.sock, solo root) y atiende
 *   un pedido por conexión. Cada pedido es una línea de texto:
 *
 *     BOOTSTRAP <usuario> <clave_hex>                  crea el primer administrador
 *     USER <token> <usuario> <rol> <clave_hex>         crea un usuario (solo administrador)
 *     AUTH <usuario> <clave_hex>                       autentica y devuelve un token
 *     STATUS <token>                                   estado del servicio y rol del token
 *     VALIDATE <token>                                 valida un token y devuelve su dueño
 *     REVOKE <token_admin> <token_a_revocar>           revoca un token (solo administrador)
 *
 *   Las contraseñas viajan en hexadecimal para que no tengan espacios.
 *   La respuesta es una línea que empieza con "OK" o "ERR".
 *
 * SEGURIDAD
 *   - Contraseñas: PBKDF2-HMAC-SHA256, 210 000 iteraciones, sal aleatoria de
 *     16 bytes por usuario. Nunca se guardan en texto plano.
 *   - Tokens: 32 bytes aleatorios. El servicio guarda solo su hash SHA-256,
 *     así que ni leyendo la memoria se obtiene un token válido.
 *   - Las comparaciones de secretos usan CRYPTO_memcmp (tiempo constante).
 *   - Las copias de contraseñas y tokens se borran con OPENSSL_cleanse.
 *   - umask 077: la base de usuarios y el registro se crean con permisos 0600.
 *   - El registro guarda acción, usuario y resultado; nunca secretos.
 *
 * LIMITACIONES CONOCIDAS (documentadas en auditoria/postura-inicial.md)
 *   - Corre como root y atiende una conexión a la vez: un cliente que se
 *     conecta y no envía nada bloquea el servicio.
 *   - Si el usuario no existe responde más rápido (no calcula PBKDF2).
 *   - Las sesiones viven en memoria: se pierden si el servicio se reinicia.
 *
 * COMPILACIÓN
 *   gcc -Wall -Wextra -Werror -O2 aegisd.c -o aegisd -lcrypto
 */

#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- Configuración (se puede cambiar al compilar con -DNOMBRE=valor) ---- */

#ifndef DIR
#define DIR "/var/lib/aegisd"           /* directorio de datos */
#endif
#ifndef SOCK
#define SOCK "/run/aegisd.sock"         /* socket de comunicación */
#endif
#define USERS DIR "/users"              /* base de usuarios */
#ifndef LOG
#define LOG "/var/log/aegisd.log"       /* registro de eventos */
#endif
#ifndef TTL
#define TTL 300                         /* vida de un token: 5 minutos */
#endif

#define ITERACIONES_PBKDF2 210000
#define MAX_SESIONES       128
#define LARGO_SAL          16           /* bytes */
#define LARGO_HASH         32           /* bytes (SHA-256) */
#define LARGO_TOKEN        32           /* bytes */
#define MAX_USUARIO        32           /* caracteres */
#define MAX_ROL            16           /* caracteres */
#define MIN_CLAVE          8            /* bytes */
#define MAX_CLAVE          128          /* bytes */

#define ROL_ADMIN    "administrador"
#define ROL_OPERADOR "operador"

/* ---- Sesiones activas (en memoria) -------------------------------------- */

struct sesion {
    unsigned char hash_token[LARGO_HASH];  /* SHA-256 del token, nunca el token */
    char usuario[MAX_USUARIO + 1];
    char rol[MAX_ROL + 1];
    time_t vence;                          /* instante de vencimiento (Unix) */
    int en_uso;
};

static struct sesion sesiones[MAX_SESIONES];

/* ---- Utilidades ---------------------------------------------------------- */

/* Convierte n bytes a texto hexadecimal (salida de 2n+1 caracteres). */
static void a_hex(const unsigned char *bytes, size_t n, char *salida)
{
    const char *digitos = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        salida[2 * i]     = digitos[bytes[i] >> 4];
        salida[2 * i + 1] = digitos[bytes[i] & 15];
    }
    salida[2 * n] = 0;
}

/* Convierte texto hexadecimal a n bytes. Devuelve 0 si el texto no tiene
 * exactamente 2n dígitos hexadecimales válidos. */
static int desde_hex(const char *texto, unsigned char *salida, size_t n)
{
    if (strlen(texto) != n * 2)
        return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned valor;
        if (!isxdigit((unsigned char)texto[2 * i]) ||
            !isxdigit((unsigned char)texto[2 * i + 1]) ||
            sscanf(texto + 2 * i, "%2x", &valor) != 1)
            return 0;
        salida[i] = valor;
    }
    return 1;
}

/* Un nombre válido tiene entre 1 y max caracteres: letras, números, _ o -. */
static int nombre_valido(const char *nombre, size_t max)
{
    size_t largo = strlen(nombre);
    if (largo == 0 || largo > max)
        return 0;
    for (size_t i = 0; i < largo; i++)
        if (!isalnum((unsigned char)nombre[i]) && nombre[i] != '_' && nombre[i] != '-')
            return 0;
    return 1;
}

/* Agrega una línea al registro: FECHA action=ACCION user=USUARIO result=RESULTADO
 * Nunca recibe secretos: solo acción, usuario y resultado. */
static void registrar(const char *accion, const char *usuario, const char *resultado)
{
    FILE *registro = fopen(LOG, "a");
    if (!registro)
        return;
    time_t ahora = time(NULL);
    struct tm fecha;
    char marca[40];
    gmtime_r(&ahora, &fecha);                              /* hora UTC */
    strftime(marca, sizeof marca, "%Y-%m-%dT%H:%M:%SZ", &fecha);
    fprintf(registro, "%s action=%s user=%s result=%s\n", marca, accion, usuario, resultado);
    fclose(registro);
}

/* Deriva el hash de una contraseña con PBKDF2-HMAC-SHA256 y la sal dada. */
static int derivar_clave(const char *clave, const unsigned char *sal, unsigned char *hash)
{
    return PKCS5_PBKDF2_HMAC(clave, (int)strlen(clave), sal, LARGO_SAL,
                             ITERACIONES_PBKDF2, EVP_sha256(), LARGO_HASH, hash) == 1;
}

/* Busca un usuario en la base. Cada línea es: usuario rol sal_hex hash_hex.
 * Si lo encuentra, copia su rol, sal y hash y devuelve 1. */
static int buscar_usuario(const char *usuario, char *rol, unsigned char *sal, unsigned char *hash)
{
    FILE *base = fopen(USERS, "r");
    if (!base)
        return 0;
    char u[33], r[17], s[33], h[65];
    int encontrado = 0;
    while (fscanf(base, "%32s %16s %32s %64s", u, r, s, h) == 4) {
        if (!strcmp(u, usuario) && desde_hex(s, sal, LARGO_SAL) && desde_hex(h, hash, LARGO_HASH)) {
            strcpy(rol, r);
            encontrado = 1;
            break;
        }
    }
    fclose(base);
    return encontrado;
}

/* Indica si la base ya tiene al menos un administrador. */
static int existe_admin(void)
{
    FILE *base = fopen(USERS, "r");
    if (!base)
        return 0;
    char u[33], r[17], s[33], h[65];
    int hay = 0;
    while (fscanf(base, "%32s %16s %32s %64s", u, r, s, h) == 4) {
        if (!strcmp(r, ROL_ADMIN)) {
            hay = 1;
            break;
        }
    }
    fclose(base);
    return hay;
}

/* Busca la sesión de un token (en hexadecimal). Devuelve su índice, o -1 si
 * el token no es válido, no existe o venció (y en ese caso la libera). */
static int buscar_sesion(const char *token_hex)
{
    unsigned char token[LARGO_TOKEN], hash[LARGO_HASH];
    if (!desde_hex(token_hex, token, LARGO_TOKEN) ||
        !EVP_Digest(token, LARGO_TOKEN, hash, NULL, EVP_sha256(), NULL))
        return -1;
    OPENSSL_cleanse(token, sizeof token);
    for (int i = 0; i < MAX_SESIONES; i++) {
        if (sesiones[i].en_uso && CRYPTO_memcmp(sesiones[i].hash_token, hash, LARGO_HASH) == 0) {
            if (sesiones[i].vence <= time(NULL)) {
                sesiones[i].en_uso = 0;
                return -1;
            }
            return i;
        }
    }
    return -1;
}

static int es_admin(int sesion)
{
    return sesion >= 0 && !strcmp(sesiones[sesion].rol, ROL_ADMIN);
}

/* ---- Operaciones --------------------------------------------------------- */

/* BOOTSTRAP y USER: crear un usuario.
 * Al llegar acá: usuario, rol y clave_hex ya están en su lugar. */
static void crear_usuario(FILE *cliente, const char *usuario, const char *rol, const char *clave_hex)
{
    unsigned char sal[LARGO_SAL], hash[LARGO_HASH], clave[256];
    char sal_hex[2 * LARGO_SAL + 1], hash_hex[2 * LARGO_HASH + 1], rol_existente[MAX_ROL + 1];
    size_t largo_clave = strlen(clave_hex) / 2;

    if (!nombre_valido(usuario, MAX_USUARIO) ||
        (strcmp(rol, ROL_ADMIN) && strcmp(rol, ROL_OPERADOR)) ||   /* solo dos roles válidos */
        largo_clave < MIN_CLAVE || largo_clave > MAX_CLAVE ||
        !desde_hex(clave_hex, clave, largo_clave)) {
        fprintf(cliente, "ERR entrada\n");
        return;
    }
    clave[largo_clave] = 0;

    /* La clave no puede contener bytes nulos, el usuario no puede existir,
     * y se genera una sal aleatoria nueva para derivar el hash. */
    if (memchr(clave, 0, largo_clave) ||
        buscar_usuario(usuario, rol_existente, sal, hash) ||
        RAND_bytes(sal, LARGO_SAL) != 1 ||
        !derivar_clave((char *)clave, sal, hash)) {
        OPENSSL_cleanse(clave, sizeof clave);
        fprintf(cliente, "ERR usuario\n");
        return;
    }
    OPENSSL_cleanse(clave, sizeof clave);
    a_hex(sal, LARGO_SAL, sal_hex);
    a_hex(hash, LARGO_HASH, hash_hex);

    FILE *base = fopen(USERS, "a");
    if (!base) {
        fprintf(cliente, "ERR almacenamiento\n");
        return;
    }
    int ok = fprintf(base, "%s %s %s %s\n", usuario, rol, sal_hex, hash_hex) > 0;
    if (fclose(base))
        ok = 0;
    registrar("crear_usuario", usuario, ok ? "ok" : "error");
    fprintf(cliente, ok ? "OK usuario_creado\n" : "ERR almacenamiento\n");
}

/* AUTH: verificar usuario y contraseña, y emitir un token temporal. */
static void autenticar(FILE *cliente, const char *usuario, const char *clave_hex)
{
    unsigned char sal[LARGO_SAL], esperado[LARGO_HASH], calculado[LARGO_HASH];
    unsigned char clave[256], token[LARGO_TOKEN], hash_token[LARGO_HASH];
    char rol[MAX_ROL + 1], token_hex[2 * LARGO_TOKEN + 1];
    size_t largo_clave = strlen(clave_hex) / 2;

    if (!nombre_valido(usuario, MAX_USUARIO) || largo_clave > MAX_CLAVE ||
        !desde_hex(clave_hex, clave, largo_clave)) {
        fprintf(cliente, "ERR entrada\n");
        return;
    }
    clave[largo_clave] = 0;

    int valida = !memchr(clave, 0, largo_clave) &&
                 buscar_usuario(usuario, rol, sal, esperado) &&
                 derivar_clave((char *)clave, sal, calculado) &&
                 CRYPTO_memcmp(calculado, esperado, LARGO_HASH) == 0;
    OPENSSL_cleanse(clave, sizeof clave);
    if (!valida) {
        registrar("autenticacion", usuario, "fallo");
        fprintf(cliente, "ERR autenticacion\n");
        return;
    }

    /* Buscar un lugar libre (o vencido) para la sesión nueva */
    time_t ahora = time(NULL);
    int i;
    for (i = 0; i < MAX_SESIONES; i++)
        if (!sesiones[i].en_uso || sesiones[i].vence <= ahora)
            break;
    if (i == MAX_SESIONES ||
        RAND_bytes(token, LARGO_TOKEN) != 1 ||
        !EVP_Digest(token, LARGO_TOKEN, hash_token, NULL, EVP_sha256(), NULL)) {
        fprintf(cliente, "ERR capacidad\n");
        return;
    }

    /* Se guarda el hash del token; el token solo se le entrega al cliente */
    memcpy(sesiones[i].hash_token, hash_token, LARGO_HASH);
    strcpy(sesiones[i].usuario, usuario);
    strcpy(sesiones[i].rol, rol);
    sesiones[i].vence = ahora + TTL;
    sesiones[i].en_uso = 1;
    a_hex(token, LARGO_TOKEN, token_hex);
    OPENSSL_cleanse(token, sizeof token);

    registrar("autenticacion", usuario, "ok");
    fprintf(cliente, "OK token=%s rol=%s vence_unix=%lld\n", token_hex, rol, (long long)sesiones[i].vence);
}

/* STATUS: confirmar que el servicio está activo y devolver el rol del token. */
static void estado(FILE *cliente, const char *token_hex)
{
    int sesion = buscar_sesion(token_hex);
    if (sesion < 0) {
        fprintf(cliente, "ERR credencial\n");
        return;
    }
    registrar("estado", sesiones[sesion].usuario, "ok");
    fprintf(cliente, "OK servicio=activo rol=%s\n", sesiones[sesion].rol);
}

/* REVOKE: invalidar un token. Solo un administrador puede hacerlo. */
static void revocar(FILE *cliente, const char *token_admin, const char *token_objetivo)
{
    int actor = buscar_sesion(token_admin);
    int objetivo = buscar_sesion(token_objetivo);
    if (!es_admin(actor)) {
        registrar("revocacion", "desconocido", "denegado");
        fprintf(cliente, "ERR permiso\n");
        return;
    }
    if (objetivo < 0) {
        fprintf(cliente, "ERR credencial\n");
        return;
    }
    sesiones[objetivo].en_uso = 0;
    registrar("revocacion", sesiones[actor].usuario, "ok");
    fprintf(cliente, "OK revocada\n");
}

/* VALIDATE: comprobar un token y devolver usuario, rol y vencimiento.
 * A diferencia de buscar_sesion, registra si el token venció. */
static void validar(FILE *cliente, const char *token_hex)
{
    unsigned char token[LARGO_TOKEN], hash[LARGO_HASH];
    if (!desde_hex(token_hex, token, LARGO_TOKEN) ||
        !EVP_Digest(token, LARGO_TOKEN, hash, NULL, EVP_sha256(), NULL)) {
        fprintf(cliente, "ERR credencial\n");
        return;
    }
    OPENSSL_cleanse(token, sizeof token);

    for (int i = 0; i < MAX_SESIONES; i++) {
        if (sesiones[i].en_uso && CRYPTO_memcmp(sesiones[i].hash_token, hash, LARGO_HASH) == 0) {
            if (sesiones[i].vence <= time(NULL)) {
                sesiones[i].en_uso = 0;
                registrar("validacion", sesiones[i].usuario, "vencida");
                fprintf(cliente, "ERR vencida\n");
            } else {
                registrar("validacion", sesiones[i].usuario, "ok");
                fprintf(cliente, "OK usuario=%s rol=%s vence_unix=%lld\n",
                        sesiones[i].usuario, sesiones[i].rol, (long long)sesiones[i].vence);
            }
            return;
        }
    }
    registrar("validacion", "desconocido", "fallo");
    fprintf(cliente, "ERR credencial\n");
}

/* ---- Atención de un pedido ----------------------------------------------- */

static void atender(FILE *cliente)
{
    char linea[2048];
    char operacion[24] = {0};
    char arg1[257] = {0}, arg2[513] = {0}, arg3[513] = {0}, arg4[513] = {0};

    /* El pedido es una sola línea terminada en salto de línea */
    if (!fgets(linea, sizeof linea, cliente) || !strchr(linea, '\n')) {
        fprintf(cliente, "ERR solicitud\n");
        return;
    }
    int campos = sscanf(linea, "%23s %256s %512s %512s %512s", operacion, arg1, arg2, arg3, arg4);

    /* BOOTSTRAP <usuario> <clave_hex>: solo funciona si todavía no hay administrador */
    if (!strcmp(operacion, "BOOTSTRAP") && campos == 3) {
        if (existe_admin()) {
            fprintf(cliente, "ERR inicializacion_completada\n");
            return;
        }
        crear_usuario(cliente, arg1, ROL_ADMIN, arg2);
        return;
    }

    /* USER <token> <usuario> <rol> <clave_hex>: requiere un token de administrador */
    if (!strcmp(operacion, "USER") && campos == 5) {
        int actor = buscar_sesion(arg1);
        if (!es_admin(actor)) {
            registrar("crear_usuario", "desconocido", "denegado");
            fprintf(cliente, "ERR permiso\n");
            return;
        }
        if (strlen(arg2) > MAX_USUARIO || strlen(arg3) > MAX_ROL) {
            fprintf(cliente, "ERR entrada\n");
            return;
        }
        crear_usuario(cliente, arg2, arg3, arg4);
        return;
    }

    if (!strcmp(operacion, "AUTH") && campos == 3) {
        autenticar(cliente, arg1, arg2);
        return;
    }
    if (!strcmp(operacion, "STATUS") && campos == 2) {
        estado(cliente, arg1);
        return;
    }
    if (!strcmp(operacion, "REVOKE") && campos == 3) {
        revocar(cliente, arg1, arg2);
        return;
    }
    if (!strcmp(operacion, "VALIDATE") && campos == 2) {
        validar(cliente, arg1);
        return;
    }
    fprintf(cliente, "ERR operacion\n");
}

/* ---- Programa principal -------------------------------------------------- */

int main(void)
{
    /* Todo archivo que cree el servicio será 0600 (solo el dueño) */
    umask(077);

    /* El directorio de datos tiene que existir, ser un directorio real (no un
     * enlace), pertenecer al usuario del servicio y no tener permisos para
     * grupo ni otros. Si no, el servicio no arranca. */
    if (mkdir(DIR, 0700) && errno != EEXIST) {
        perror("mkdir");
        return 1;
    }
    struct stat info;
    if (lstat(DIR, &info) || !S_ISDIR(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 077)) {
        fprintf(stderr, "Directorio inseguro\n");
        return 1;
    }

    /* Socket Unix local con permisos 0600: solo root puede conectarse */
    int servidor = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (servidor < 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_un direccion = {.sun_family = AF_UNIX};
    strcpy(direccion.sun_path, SOCK);
    if (bind(servidor, (void *)&direccion, sizeof direccion) || chmod(SOCK, 0600) || listen(servidor, 16)) {
        perror("bind/listen: socket existente no se elimina automaticamente");
        return 1;
    }
    registrar("servicio", "-", "iniciado");

    /* Atiende las conexiones de a una: un pedido y una respuesta por conexión */
    for (;;) {
        int conexion = accept4(servidor, NULL, NULL, SOCK_CLOEXEC);
        if (conexion < 0) {
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }
        FILE *cliente = fdopen(conexion, "r+");
        if (!cliente) {
            close(conexion);
            continue;
        }
        atender(cliente);
        fclose(cliente);
    }
    close(servidor);
    return 1;
}
