///.../CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc -o btn -I../libev/ -L../libev/.libs/ -static -lev btn.c -lev -lm -s
#include <ev.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>


#define EVDEV "/dev/input/event0"
#define RECVH "10.0.1.9"
#define RECVP 4423

#define SENDDELAY (0.1)
#define MAXREP 30

///////////

#define STR(s) #s
#define XSTR(s) STR(s)
#define BTN_LOCKFILE "/tmp/btn.lock"


struct event
{
  // tt tt tt tt mm mm mm mm  ty ty co co va va va va
  // 58 df 1b 4c 4f 87 00 00  01 00 67 00 01 00 00 00

  unsigned int time1;
  unsigned int time2;
  unsigned short type;
  unsigned short code;
  unsigned int value;
};


struct reg
{
  int code;
  unsigned count[3];
};


static ev_io input_watcher;
static int input_fd;
static int send_s;
static struct sockaddr_in send_si;
static struct reg count[]=
{
  {0x6c},
  {0x6a},
  {0x69},
  {0x67},
  {0x1c},
  {-1}
};
static ev_timer send_watcher;
static int codec=0;
static struct ev_loop *loop;
static int timer_on=0;


static void send_cb(EV_P_ ev_timer *w, int revents)
{
  char msg[64];
  char val[20];
  int len,l,i,keep=0;
  static int send_count=0;

  if(codec!=0)
  {
    msg[0]='\0';
    for(len=i=0;count[i].code>-1;i++)
    {
      if((count[i].count[0]|count[i].count[1]|count[i].count[2])>0)
      {
        l=snprintf(val,sizeof(val),"%d:%d,%d,%d ",count[i].code,count[i].count[0],count[i].count[1],count[i].count[2]);
        if((count[i].count[1]|count[i].count[2])!=0) keep=1;
        if((len+l)<sizeof(msg))
        {
          len+=l;
          strcat(msg,val);
        }
      }
    }
    if(len>0)
    {
      if(++send_count>MAXREP) keep=0;
      strcat(msg,"\n");
      len++;
      sendto(send_s,(const void *)msg,(size_t)len,0,(const struct sockaddr *)&send_si,sizeof(send_si));
      for(i=0;count[i].code>-1;i++)
      {
        if((count[i].count[1]|count[i].count[2])!=0&&keep) count[i].count[2]++;
        else count[i].count[2]=0;
        count[i].count[0]=count[i].count[1]=0;
      }
    }
    if(!keep)
    {
      codec=0;
      timer_on=0;
      ev_timer_stop(loop,&send_watcher);
      send_count=0;
    }
  }
}


static void process(struct event *ev, int s)
{
  int i;
  //printf("ev[%d] %d,%d,%d\n",s,ev->type,ev->code,ev->value);
  for(i=0;count[i].code>ev->code;i++);
  if(ev->code==count[i].code)
  {
    count[i].count[(ev->value==0?0:1)]++;
    count[i].count[2]=0;
    codec=1;
    if(!timer_on)
    {
      send_watcher.repeat=SENDDELAY;
      ev_timer_again(loop,&send_watcher);
      timer_on=1;
    }
  }
}


static void input_cb(EV_P_ ev_io *w, int revents)
{
  struct event ev[2];
  int r;
  
  if(revents&EV_READ) while(-1!=(r=read(input_fd,&ev,sizeof(ev)))) process(&ev[0],(r/sizeof(struct event)));
}

int main(int argc, char **argv)
{
  loop = EV_DEFAULT;

  if(sizeof(int)!=4||sizeof(short)!=2) { printf("basic type size mismatch\n"); return(1); }

  if((send_s=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))!=-1)
  {
    memset((char *)&send_si,0,sizeof(send_si));
    send_si.sin_family=AF_INET;
    send_si.sin_port=htons(RECVP);
    inet_aton(RECVH,&send_si.sin_addr);
    timer_on=0;

    input_fd=open(EVDEV,O_RDONLY|O_NONBLOCK);
    if(input_fd>=0)
    {
      ev_io_init(&input_watcher, input_cb, input_fd, EV_READ);
      ev_io_start(EV_A_ &input_watcher);
      ev_init(&send_watcher,send_cb);
      ev_run (loop, 0);
      close(input_fd);
      ev_timer_stop(loop,&send_watcher);
    }
    else perror("open " EVDEV);
  }

  return 0;
}
