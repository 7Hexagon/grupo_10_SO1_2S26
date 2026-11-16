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

#ifndef DIR
#define DIR "/var/lib/aegisd"
#endif
#ifndef SOCK
#define SOCK "/run/aegisd.sock"
#endif
#define USERS DIR "/users"
#ifndef LOG
#define LOG "/var/log/aegisd.log"
#endif
#ifndef TTL
#define TTL 300
#endif
#define ITER 210000
struct session { unsigned char hash[32]; char user[33], role[17]; time_t until; int used; };
static struct session sessions[128];
static void hex(const unsigned char *p,size_t n,char *out) { const char *d="0123456789abcdef"; for(size_t i=0;i<n;i++){out[2*i]=d[p[i]>>4];out[2*i+1]=d[p[i]&15];} out[2*n]=0; }
static int unhex(const char *s,unsigned char *out,size_t n) { if(strlen(s)!=n*2)return 0; for(size_t i=0;i<n;i++){unsigned x; if(!isxdigit((unsigned char)s[2*i])||!isxdigit((unsigned char)s[2*i+1])||sscanf(s+2*i,"%2x",&x)!=1)return 0;out[i]=x;}return 1; }
static int name_ok(const char *s,size_t max) { size_t n=strlen(s); if(!n||n>max)return 0; for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)s[i])&&s[i]!='_'&&s[i]!='-')return 0;return 1; }
static void event(const char *action,const char *user,const char *result) { FILE *f=fopen(LOG,"a"); if(!f)return; time_t now=time(NULL); struct tm tm; char stamp[40]; gmtime_r(&now,&tm); strftime(stamp,sizeof stamp,"%Y-%m-%dT%H:%M:%SZ",&tm); fprintf(f,"%s action=%s user=%s result=%s\n",stamp,action,user,result); fclose(f); }
static int derive(const char *pw,const unsigned char *salt,unsigned char *out){return PKCS5_PBKDF2_HMAC(pw,(int)strlen(pw),salt,16,ITER,EVP_sha256(),32,out)==1;}
static int lookup(const char *user,char *role,unsigned char *salt,unsigned char *digest){ FILE *f=fopen(USERS,"r"); if(!f)return 0; char u[33],r[17],s[33],d[65];int found=0; while(fscanf(f,"%32s %16s %32s %64s",u,r,s,d)==4){if(!strcmp(u,user)&&unhex(s,salt,16)&&unhex(d,digest,32)){strcpy(role,r);found=1;break;}} fclose(f);return found; }
static int session_find(const char *s) { unsigned char token[32],digest[32]; if(!unhex(s,token,32)||!EVP_Digest(token,32,digest,NULL,EVP_sha256(),NULL))return -1;OPENSSL_cleanse(token,sizeof token); for(int i=0;i<128;i++)if(sessions[i].used&&CRYPTO_memcmp(sessions[i].hash,digest,32)==0){if(sessions[i].until<=time(NULL)){sessions[i].used=0;return -1;}return i;}return -1; }
static int have_admin(void){FILE *f=fopen(USERS,"r");if(!f)return 0;char u[33],r[17],s[33],d[65];int yes=0;while(fscanf(f,"%32s %16s %32s %64s",u,r,s,d)==4)if(!strcmp(r,"administrador")){yes=1;break;}fclose(f);return yes;}
static void respond(FILE *io) {
 char line[2048],op[24]={0},a[257]={0},b[513]={0},c[513]={0},d[513]={0};
 if(!fgets(line,sizeof line,io)||!strchr(line,'\n')){fprintf(io,"ERR solicitud\n");return;}
 int n=sscanf(line,"%23s %256s %512s %512s %512s",op,a,b,c,d);
 if((!strcmp(op,"USER")&&n==5)||(!strcmp(op,"BOOTSTRAP")&&n==3)){
  if(!strcmp(op,"BOOTSTRAP")){if(have_admin()){fprintf(io,"ERR inicializacion_completada\n");return;}strcpy(d,b);strcpy(b,"administrador");strcpy(c,d);}else {int actor=session_find(a);if(actor<0||strcmp(sessions[actor].role,"administrador")){event("crear_usuario","desconocido","denegado");fprintf(io,"ERR permiso\n");return;}if(strlen(b)>32||strlen(c)>16){fprintf(io,"ERR entrada\n");return;}memmove(a,b,strlen(b)+1);memmove(b,c,strlen(c)+1);memmove(c,d,strlen(d)+1);}
  unsigned char salt[16],digest[32],password[256]; char sh[33],dh[65],role[17];
  size_t plen=strlen(c)/2;
  if(!name_ok(a,32)||(strcmp(b,"administrador")&&strcmp(b,"operador"))||plen<8||plen>128||!unhex(c,password,plen)){fprintf(io,"ERR entrada\n");return;} password[plen]=0;
  if(memchr(password,0,plen)||lookup(a,role,salt,digest)||RAND_bytes(salt,16)!=1||!derive((char*)password,salt,digest)){OPENSSL_cleanse(password,sizeof password);fprintf(io,"ERR usuario\n");return;}
  OPENSSL_cleanse(password,sizeof password);hex(salt,16,sh);hex(digest,32,dh);
  FILE *f=fopen(USERS,"a");if(!f){fprintf(io,"ERR almacenamiento\n");return;} int ok=fprintf(f,"%s %s %s %s\n",a,b,sh,dh)>0; if(fclose(f))ok=0;
  event("crear_usuario",a,ok?"ok":"error");fprintf(io,ok?"OK usuario_creado\n":"ERR almacenamiento\n");return;
 }
 if(!strcmp(op,"AUTH")&&n==3){
  unsigned char salt[16],expected[32],actual[32],password[256],token[32],digest[32];char role[17],tokenhex[65];size_t plen=strlen(b)/2;
  if(!name_ok(a,32)||plen>128||!unhex(b,password,plen)){fprintf(io,"ERR entrada\n");return;}password[plen]=0;
  int valid=!memchr(password,0,plen)&&lookup(a,role,salt,expected)&&derive((char*)password,salt,actual)&&CRYPTO_memcmp(actual,expected,32)==0;
  OPENSSL_cleanse(password,sizeof password); if(!valid){event("autenticacion",a,"fallo");fprintf(io,"ERR autenticacion\n");return;}
  int i;time_t now=time(NULL);for(i=0;i<128;i++)if(!sessions[i].used||sessions[i].until<=now)break;
  if(i==128||RAND_bytes(token,32)!=1||!EVP_Digest(token,32,digest,NULL,EVP_sha256(),NULL)){fprintf(io,"ERR capacidad\n");return;}
  memcpy(sessions[i].hash,digest,32);strcpy(sessions[i].user,a);strcpy(sessions[i].role,role);sessions[i].until=now+TTL;sessions[i].used=1;hex(token,32,tokenhex);OPENSSL_cleanse(token,sizeof token);
  event("autenticacion",a,"ok");fprintf(io,"OK token=%s rol=%s vence_unix=%lld\n",tokenhex,role,(long long)sessions[i].until);return;
 }
 if(!strcmp(op,"STATUS")&&n==2){int actor=session_find(a);if(actor<0){fprintf(io,"ERR credencial\n");return;}event("estado",sessions[actor].user,"ok");fprintf(io,"OK servicio=activo rol=%s\n",sessions[actor].role);return;}
 if(!strcmp(op,"REVOKE")&&n==3){int actor=session_find(a),target=session_find(b);if(actor<0||strcmp(sessions[actor].role,"administrador")){event("revocacion","desconocido","denegado");fprintf(io,"ERR permiso\n");return;}if(target<0){fprintf(io,"ERR credencial\n");return;}sessions[target].used=0;event("revocacion",sessions[actor].user,"ok");fprintf(io,"OK revocada\n");return;}
 if(!strcmp(op,"VALIDATE")&&n==2){
  unsigned char token[32],digest[32];if(!unhex(a,token,32)||!EVP_Digest(token,32,digest,NULL,EVP_sha256(),NULL)){fprintf(io,"ERR credencial\n");return;}OPENSSL_cleanse(token,sizeof token);
  for(int i=0;i<128;i++)if(sessions[i].used&&CRYPTO_memcmp(sessions[i].hash,digest,32)==0){if(sessions[i].until<=time(NULL)){sessions[i].used=0;event("validacion",sessions[i].user,"vencida");fprintf(io,"ERR vencida\n");return;}if(!strcmp(op,"REVOKE")){sessions[i].used=0;event("revocacion",sessions[i].user,"ok");fprintf(io,"OK revocada\n");}else {event("validacion",sessions[i].user,"ok");fprintf(io,"OK usuario=%s rol=%s vence_unix=%lld\n",sessions[i].user,sessions[i].role,(long long)sessions[i].until);}return;}
  event(!strcmp(op,"REVOKE")?"revocacion":"validacion","desconocido","fallo");fprintf(io,"ERR credencial\n");return;
 }
 fprintf(io,"ERR operacion\n");
}
int main(void){umask(077);if(mkdir(DIR,0700)&&errno!=EEXIST){perror("mkdir");return 1;}struct stat st;if(lstat(DIR,&st)||!S_ISDIR(st.st_mode)||st.st_uid!=geteuid()||(st.st_mode&077)){fprintf(stderr,"Directorio inseguro\n");return 1;}int fd=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);if(fd<0){perror("socket");return 1;}struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,SOCK);if(bind(fd,(void*)&addr,sizeof addr)||chmod(SOCK,0600)||listen(fd,16)){perror("bind/listen: socket existente no se elimina automaticamente");return 1;}event("servicio","-","iniciado");for(;;){int c=accept4(fd,NULL,NULL,SOCK_CLOEXEC);if(c<0){if(errno==EINTR)continue;perror("accept");break;}FILE *io=fdopen(c,"r+");if(!io){close(c);continue;}respond(io);fclose(io);}close(fd);return 1;}
