#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
find(char *path, char *filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:   //如果是文件
    if(strcmp(fmtname(path) , filename)==0) {//如果该文件和filename同名，则输出路径
        printf("%s\n", path);
    }
    break;

  case T_DIR:   //如果是文件夹
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      
      if(strcmp(de.name,".")==0) continue;
      if(strcmp(de.name,"..")==0) continue;  //避免无限递归
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      find(buf, filename);  //递归搜索子文件夹
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        exit();
    }
    char name[DIRSIZ + 1];
    for (int i = 0; i < sizeof(argv[2]); i++)
    {
        name[i] = argv[2][i];
    }
    memset(name + strlen(argv[2]), ' ', DIRSIZ - strlen(argv[2]));
    find(argv[1], name);
    exit();
}
