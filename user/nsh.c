#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
 
void execPipe(char*argv[],int argc);

#define MAXARGS 10
#define MAXWORD 30
#define MAXLINE 100
 
int getcmd(char *buf, int nbuf)  //功能：从脚本读入一行命令，取自sh.c
{
    fprintf(2, "@ "); //使用fprintf(2，…)将错误和调试消息打印到文件描述符2，即标准错误
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}
char whitespace[] = " \t\r\n\v";  //可忽略的字符
char args[MAXARGS][MAXWORD];  //参数表

void setargs(char *cmd, char* argv[],int* argc) //功能：让argv的每一个元素都指向args的每一行
{
    for(int i=0;i<MAXARGS;i++){
        argv[i]=&args[i][0];
    }
    int i = 0; // 表示第i个单词
    int j = 0;  //当前命令的扫描头
    for (; cmd[j] != '\n' && cmd[j] != '\0'; j++)
    {
        // 每一轮循环都是找到输入的命令中的一个word，比如 echo hi ,就是先找到echo，再找到hi
        // 让argv[i]分别指向他们的开头，并且将echo，hi后面的空格设为\0
        // 跳过之前的空格
        while (strchr(whitespace,cmd[j])){
            j++;
        }
        argv[i++]=cmd+j;   //将当前单词的首地址给参数表
        // 只要不是空格，就j++,找到下一个空格
        while (strchr(whitespace,cmd[j])==0){ //找到当前单词的末尾
            j++;
        }
        cmd[j]='\0';  //在当前单词的末尾加上结束符
    }
    argv[i]=0;  //参数表结束
    *argc=i;
}
 
void runcmd(char*argv[],int argc) //功能：递归地执行命令
{
    for(int i = 1;i < argc;i++){
        if(!strcmp(argv[i],"|")){
            execPipe(argv,argc);// 如果遇到 | 则用管道将两侧的输出输入端连接
        }
    }
    // 此时一行命令已经被拆分为由‘|‘分割的若干子命令
    for(int i = 1;i < argc;i++){
        if(!strcmp(argv[i],">")){  // 如果遇到 > ，说明需要执行输出重定向
            close(1); //关闭标准输出端
            // 把输出重定向到后面给出的文件名对应的文件里
            open(argv[i+1],O_CREATE|O_WRONLY);  //如果文件不存在则创建，如果存在则打开
            argv[i] = 0;  //遇到>说明前面的命令和参数已经读入完毕，所以参数表末尾设置标志位
        }
        if(!strcmp(argv[i],"<")){
            // 如果遇到< ,需要执行输入重定向，关闭stdin
            close(0);
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
        }
    }
    exec(argv[0], argv);
}
 
void execPipe(char*argv[],int argc){  //功能：在|左右两个进程之间创建管道
    int i=0;  //子命令中的参数个数
    // 首先找到命令中的"|",然后把他换成'\0'
    // 从前到后，找到第一个|
    for(;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            argv[i]=0;
            break;
        }
    }
    // 将|两侧的内容分为两个子命令
    int fd[2];  //用于连接|两侧的输出端和输入端
    pipe(fd);
    if(fork()==0){
        // 子进程 执行|左边的命令 把自己的标准输出关闭
        close(1);  //关闭子进程的标准输出
        dup(fd[1]);  //将输出重定向到fd[1]
        close(fd[0]);
        close(fd[1]);
        runcmd(argv,i);  //递归执行子命令
    }else{
        // 父进程 执行|右边的命令 把自己的标准输入关闭
        close(0);  //关闭父进程的标准输入
        dup(fd[0]);  //将输入重定向到fd[0]
        close(fd[0]);
        close(fd[1]);
        runcmd(argv+i+1,argc-i-1);  //递归执行子命令
    }
}

int main()
{
    char buf[MAXLINE];
    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0)  //从脚本读入一行命令
    {
 
        if (fork() == 0)
        {
            char* argv[MAXARGS];
            int argc=-1;
            setargs(buf, argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
    exit(0);
}