// #include "kernel/types.h"
// #include "user/user.h"
// #include "kernel/fcntl.h"

// #define MAXARGS 10

// #define EXEC  1
// #define REDIR 2
// #define PIPE  3

// struct cmd {
//   int type;
// };

// struct execcmd {
//   int type;
//   char *argv[MAXARGS];
//   char *eargv[MAXARGS];
// };

// struct redircmd {
//   int type;
//   struct cmd *cmd;
//   char *file;
//   char *efile;
//   int mode;
//   int fd;
// };

// struct pipecmd {
//   int type;
//   struct cmd *left;
//   struct cmd *right;
// };








// int main(void)
// {
//   static char buf[100];
//   int fd;

//   // Ensure that three file descriptors are open.
//   while((fd = open("console", O_RDWR)) >= 0){
//     if(fd >= 3){
//       close(fd);
//       break;
//     }
//   }

//   // Read and run input commands.
//   while(getcmd(buf, sizeof(buf)) >= 0){  //读入一行命令
//     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){ //如果该命令是cd
//       // Chdir must be called by the parent, not the child.
//       buf[strlen(buf)-1] = 0;  // chop \n
//       if(chdir(buf+3) < 0)  //判断cd后面要打开的目录名能不能打开
//         fprintf(2, "cannot cd %s\n", buf+3);
//       continue;
//     }
//     if(fork1() == 0)  //创建一个子进程
//       runcmd(parsecmd(buf));
//     wait(0);
//   }
//   exit(0);
// }

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
 
void execPipe(char*argv[],int argc);
//*****************START  from sh.c *******************
 
 
#define MAXARGS 10
#define MAXWORD 30
#define MAXLINE 100
 
int getcmd(char *buf, int nbuf)  //从脚本读入一行命令
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}
char whitespace[] = " \t\r\n\v";
char args[MAXARGS][MAXWORD];  //参数表
 
//*****************END  from sh.c ******************
void setargs(char *cmd, char* argv[],int* argc)
{
    // 让argv的每一个元素都指向args的每一行
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
 
// void runcmd(char *cmd)
void runcmd(char*argv[],int argc)
{
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"|")){
            // 如果遇到 | 则用管道将两侧的输出输入端连接
            execPipe(argv,argc);
        }
    }
    // 此时一行命令已经被拆分为由‘|‘分割的若干子命令
    for(int i=1;i<argc;i++){
        // 如果遇到 > ，说明需要执行输出重定向，首先需要关闭stdout
        if(!strcmp(argv[i],">")){
            close(1);
            // 此时需要把输出重定向到后面给出的文件名对应的文件里
            // 当然如果>是最后一个，那就会error，不过暂时先不考虑
            open(argv[i+1],O_CREATE|O_WRONLY);  //如果文件不存在则创建，如果存在则打开
            argv[i]=0;
            // break;
        }
        if(!strcmp(argv[i],"<")){
            // 如果遇到< ,需要执行输入重定向，关闭stdin
            close(0);
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
            // break;
        }
    }
    exec(argv[0], argv);  //重定向完成后，可以开始执行命令了
}
 
void execPipe(char*argv[],int argc){
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