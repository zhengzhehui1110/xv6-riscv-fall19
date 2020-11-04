#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int id = getpid(); //父进程id
    int fd[2];
    pipe(fd);
    int ch[2];
    pipe(ch);
    char buf[] = "ping"; //传入的字符串ping
    write(fd[1], buf,strlen(buf)); 
    int pid = fork();  //子进程id
    char n[50] = {0};  //子进程中接收的字符串
    if(pid==0) {  //子进程
        
        read(fd[0],n,sizeof(n));  //接收字符串ping
        printf("%d: received %s\n",getpid(),&n);
        write(ch[1],"pong",4);  //传入字符串pong
        exit();
    }
    char ans[50] = {0};
    read(ch[0],ans,sizeof(ans));  //接收字符串pong
    
    printf("%d: received %s\n",id,&ans);
    exit();
    return 0;
}