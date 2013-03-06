#define CMD_MAX_LEN 1000
#define MAX_CMD_NUM 10
#define MAX_ARG_NUM 10
#define MAX_PATH_LEN 300
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
char cmd[CMD_MAX_LEN];
char *cmdsplit[MAX_CMD_NUM];
char *currentcmd[MAX_ARG_NUM+1];//last must be NULL
char current_working_directory[MAX_PATH_LEN];
/*char *envp[]={"PATH=/usr/bin:/usr/local/bin",NULL};*/
static void sigint_handler(int signo);
int parse_cmd();
void execute_cmd();


//parse command from cmd to cmdsplit
//return cmd num
int parse_cmd() {
  char* pch=strtok(cmd,"|\n");
  int i=0;
  while (pch!=NULL && i<MAX_CMD_NUM) {
    cmdsplit[i++]=pch;
    pch=strtok(NULL,"|\n");
  }
  return i;
}


void execute_cmd() {
  int cnt=parse_cmd();
  for (int i=0;i<cnt;++i) {
    bzero(currentcmd,sizeof(currentcmd));
    char* pch=strtok(cmdsplit[i]," \n");
    if (pch==NULL) return;
    int j=0;
    while (pch!=NULL && j<MAX_ARG_NUM) {
      currentcmd[j++]=pch;
      pch=strtok(NULL," \n");
    }
    if (!strcmp(cmd,"exit")) {
      printf("\n");
      exit(0);
    }
    if (!strcmp(currentcmd[0],"cd")) {
      if (j==2) {
        if (currentcmd[1][0]=='/') {
          strncpy(current_working_directory,currentcmd[1],MAX_PATH_LEN);
        } else {
          if (getcwd(current_working_directory,MAX_PATH_LEN)) {
            strncat(current_working_directory,"/",1);
            strncat(current_working_directory,currentcmd[1],MAX_PATH_LEN-strlen(current_working_directory));
          } else {
            perror("getcwd error cd");
          }
        }
        if (chdir(current_working_directory)<0) {
          perror("cd");
        }
      } else if (j==1) { //if only "cd" then just go to home
        struct passwd *pwd;
        pwd=getpwuid(getuid());
        strncpy(current_working_directory,"/home/",MAX_PATH_LEN);
        strncat(current_working_directory,pwd->pw_name,MAX_PATH_LEN);
        if (chdir(current_working_directory)<0) {
          perror("cd");
        }
      } else {
        printf("Too Many Arguments\n");
      }
      continue;
    }
    pid_t p=fork();
    if (p<0){
      perror("fork error");
      continue;
    }
    if (p==0) { //child process
      /*printf("child process pid=%d\n",getpid());*/
      if (execvp(currentcmd[0],currentcmd)<0) {
        perror(currentcmd[0]);
        exit(0);
      }
    } else {
      wait(NULL);
      /*printf("parent Process,pid=%d\n",getpid());*/
    }
  }
}


static void sigint_handler(int signo) {
  return;
}

int main() {
  //disable Ctrl+c
  signal(SIGINT,&sigint_handler);
  while (1) {
    printf("shell>");
    if (fgets(cmd,CMD_MAX_LEN,stdin)==NULL) break;
    /*cmd[strlen(cmd)-1]='\0';*/
    execute_cmd();
  }
  printf("\n");
  return 0;
}
