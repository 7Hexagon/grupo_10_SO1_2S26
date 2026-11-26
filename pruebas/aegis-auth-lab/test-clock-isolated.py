import os,secrets,socket,subprocess,tempfile,time,sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'src' / 'aegis-auth-lab'))
from aegisd_client import AegisClient
if len(sys.argv) != 3: raise SystemExit('uso: test-clock-isolated.py FUENTE_V3 FUENTE_CORREGIDA')
FAKE=r'''#define _GNU_SOURCE
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
time_t time(time_t *p) { struct timeval v; gettimeofday(&v,0); long offset=0; const char *f=getenv("FAKE_WALL_OFFSET"); FILE *x=f?fopen(f,"r"):0; if(x){if(fscanf(x,"%ld",&offset)!=1)offset=0;fclose(x);} time_t t=v.tv_sec+offset; if(p)*p=t; return t; }
'''
with tempfile.TemporaryDirectory(prefix='aegis-clock-test-') as d:
 root=Path(d); (root/'fake.c').write_text(FAKE); subprocess.run(['gcc','-shared','-fPIC',str(root/'fake.c'),'-o',str(root/'fake.so')],check=True)
 offset=root/'offset';offset.write_text('0');pw=secrets.token_hex(12)
 for label,source in [('old',sys.argv[1]),('new',sys.argv[2])]:
  sock=str(root/(label+'.sock'));data=str(root/(label+'-data'));log=str(root/(label+'-log'));exe=str(root/(label+'-bin'));offset.write_text('0')
  subprocess.run(['gcc','-O2',source,'-o',exe,'-lcrypto',f'-DDIR="{data}"',f'-DSOCK="{sock}"',f'-DLOG="{log}"','-DTTL=3'],check=True)
  env=os.environ.copy();env.update({'LD_PRELOAD':str(root/'fake.so'),'FAKE_WALL_OFFSET':str(offset)})
  proc=subprocess.Popen([exe],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
  try:
   for _ in range(100):
    if os.path.exists(sock):break
    if proc.poll() is not None:raise RuntimeError('daemon failed')
    time.sleep(.05)
   with socket.socket(socket.AF_UNIX,socket.SOCK_STREAM) as s:
    s.connect(sock);s.sendall(('BOOTSTRAP test_admin '+pw.encode().hex()+'\n').encode());assert s.recv(256)==b'OK usuario_creado\n'
   c=AegisClient(sock);res=c.authenticate('test_admin',pw);assert res.startswith('OK token=');token=res.split()[1].split('=',1)[1]
   assert c.validate(token).startswith('OK ')
   offset.write_text('-3600');time.sleep(4)
   result=c.validate(token)
   if label=='old':assert result.startswith('OK ');print('OLD_WALL_CLOCK_BACKWARD_EXTENDS_TOKEN=CONFIRMED')
   else:assert result=='ERR vencida';print('NEW_BACKWARD_CLOCK_TOKEN_EXPIRES=PASS')
  finally:
   proc.terminate();proc.wait(timeout=3)
