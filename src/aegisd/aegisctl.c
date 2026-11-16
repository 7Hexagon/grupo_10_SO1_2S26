#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#ifndef SOCK
#define SOCK "/run/aegisd.sock"
#endif
static int read_line(char *p,size_t cap){if(!fgets(p,cap,stdin))return 0;size_t n=strcspn(p,"\n");if(p[n]!='\n')return 0;p[n]=0;return 1;}
int main(int argc,char **argv){
 if(argc<2){fprintf(stderr,"Uso: aegisctl iniciar-admin USUARIO | crear USUARIO ROL | autenticar USUARIO | estado | validar | revocar\nSecretos por stdin.\n");return 2;}
 char req[2048]={0},pw[130]={0},enc[257]={0},token[66]={0},target[66]={0};const char *op=argv[1];
 if((!strcmp(op,"iniciar-admin")&&argc==3)||(!strcmp(op,"crear")&&argc==4)||(!strcmp(op,"autenticar")&&argc==3)){
  if(!strcmp(op,"crear")&&!read_line(token,sizeof token))return 2;
  if(!read_line(pw,sizeof pw))return 2;
  size_t n=strlen(pw);const char *d="0123456789abcdef";for(size_t i=0;i<n;i++){enc[2*i]=d[(unsigned char)pw[i]>>4];enc[2*i+1]=d[(unsigned char)pw[i]&15];}enc[2*n]=0;OPENSSL_cleanse(pw,sizeof pw);
  if(!strcmp(op,"iniciar-admin"))snprintf(req,sizeof req,"BOOTSTRAP %s %s\n",argv[2],enc);else if(!strcmp(op,"crear"))snprintf(req,sizeof req,"USER %s %s %s %s\n",token,argv[2],argv[3],enc);else snprintf(req,sizeof req,"AUTH %s %s\n",argv[2],enc);OPENSSL_cleanse(enc,sizeof enc);
 }else if(argc==2&&(!strcmp(op,"estado")||!strcmp(op,"validar")||!strcmp(op,"revocar"))){if(!read_line(token,sizeof token))return 2;if(!strcmp(op,"revocar")){if(!read_line(target,sizeof target))return 2;snprintf(req,sizeof req,"REVOKE %s %s\n",token,target);}else snprintf(req,sizeof req,"%s %s\n",!strcmp(op,"estado")?"STATUS":"VALIDATE",token);}else return 2;
 OPENSSL_cleanse(token,sizeof token);OPENSSL_cleanse(target,sizeof target);
 int fd=socket(AF_UNIX,SOCK_STREAM,0);if(fd<0){perror("socket");return 1;}struct sockaddr_un a={.sun_family=AF_UNIX};strcpy(a.sun_path,SOCK);if(connect(fd,(void*)&a,sizeof a)){perror("aegisd");return 1;}FILE *f=fdopen(fd,"r+");if(!f)return 1;fputs(req,f);fflush(f);OPENSSL_cleanse(req,sizeof req);char response[512];if(!fgets(response,sizeof response,f)){fclose(f);return 1;}fputs(response,stdout);fclose(f);return strncmp(response,"OK ",3)!=0;
}
