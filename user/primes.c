#include "kernel/types.h"
#include "user/user.h"

void prime(int port) {  //传入读端口
    int n; //用来存放每一轮管道中取出的最小的数
    int val;  //用来承接管道中读出的值
    int p[2];  //新的管道
    pipe(p);
    if(read(port,&n,sizeof(int))>0) {
        printf("prime %d\n",n); //输出第一个数
    }
    else {
        exit();
    }
    if (fork() == 0) {   //子进程
        close(p[1]);  //关闭写端
        prime(p[0]);  //传入读端
        exit();
    }  
     //父进程
    while(read(port,&val,sizeof(int))) {
        if(val % n != 0) { //和第一个数取模
            write(p[1],&val,sizeof(int)); //筛选出的数传入写端
        }
    } 
    close(p[1]);  //关闭写端
    wait();
    exit();
    return;
}

int main () {
    int p[2];
    pipe(p);
    int i;
    for (i = 2;i <= 35;i++) {
        write(p[1],&i,sizeof(int));  //将初始的2到35放入管道
    }
    close(p[1]);
    prime(p[0]);
    exit();
    return 0;
}
