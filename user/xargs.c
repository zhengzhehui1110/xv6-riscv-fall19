#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    
    int j;
    
    char readbuf[512];  //每次读取标准输出端的缓冲区
    int length;  //每次读取标准输出端得到的字符个数

    int i;
    char *buf[MAXARG];
    buf[0] = argv[1];  //参数1：filename
    int m = 0; //每次执行filename命令时参数的个数
    char singlepara[MAXARG][MAXARG];  //每次执行filename命令时的参数
    memset(singlepara,'\0',sizeof(singlepara));
    char *p = singlepara[0];

    for(i = 2;i < argc;i++) {
        buf[i-1] = argv[i];
    }
    m = argc-1;
    p = singlepara[m];

    while ( (length = read(0,readbuf,sizeof(readbuf))) > 0)
    {
        for(j = 0;j < length;j++) {
            if(readbuf[j] == '\n') {  //检测到回车，执行一次filename命令
                *p = '\0';  //字符串结束符
                buf[m] = singlepara[m];
                m++; //参数数量+1
                buf[m] = 0;  //参数接收完毕的标志位
                m = argc - 1; //回到第一次读取标准输出端的位置
                if(fork()==0) {
                    exec(argv[1],buf);  //执行一次命令
                }
                wait();
                p = singlepara[m];
            }
            else if(readbuf[j]==' ') { //检测到空格，增加一个参数
                *p = '\0';  //字符串结束符
                buf[m] = singlepara[m];
                m++;
                p = singlepara[m];  //准备填写参数表的下一行
                //printf("para count is %d\n",m);
            }
            else {  //检测到任意字符，填入参数表
                *p = readbuf[j];
                p++;
                //printf("now para is:(%s)\n",singlepara[m]);
            }
        }
    }
    
    exit();
    return 0;
}
