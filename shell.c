#define CMD_MAX_LEN 1024
#define MAX_CMD_NUM 16
#define MAX_ARG_NUM 16
#define MAX_PATH_LEN 256
#define MAX_PROMPT_LEN 256
#define MAX_PROC_NUM 16
#define STDIN_FD 0
#define STDOUT_FD 1
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>
char cmd[CMD_MAX_LEN];
char *cmdsplit[MAX_CMD_NUM];
char *currentcmd[MAX_ARG_NUM+1];//last must be NULL
char prompt[MAX_PROMPT_LEN];
char current_working_directory[MAX_PATH_LEN];

char *stdin_filename;
char *stdout_filename;

int pipefd[MAX_PROC_NUM][2];

int background_job=0;


static void sigint_handler(int signo);
int parse_cmd();
void execute_cmd();


//parse command from cmd to cmdsplit
//return cmd num
int parse_cmd() {
  int len=strlen(cmd); //check if  there is & at the end
  for (int i=len-1;i>=0;--i) {
    if (cmd[i]=='&') {
      background_job=1;
      cmd[i]='\0';
      break;
    }
  }
  //split cmd use '|'
  char* pch=strtok(cmd,"|\n");
  int i=0;
  while (pch!=NULL && i<MAX_CMD_NUM) {
    cmdsplit[i++]=pch;
    pch=strtok(NULL,"|\n");
  }
  return i;
}

int generate_prompt(char dst[]) { //make a prompt string to dst
  if (dst==NULL) return -1;
  if (getcwd(current_working_directory,MAX_PATH_LEN)) {
    strncpy(dst,current_working_directory,MAX_PROMPT_LEN);
    strncat(dst," >",2);
  } else {
    perror("getcwd error prompt");
    return -1;
  }
  return 0;
}

void execute_cmd() {
  int cnt=parse_cmd();
  stdin_filename=NULL;
  stdout_filename=NULL;
  for (int i=0;i<cnt;++i) {
    bzero(currentcmd,sizeof(currentcmd));
    char* pch=strtok(cmdsplit[i]," \n");
    if (pch==NULL) return;
    int j=0;
    while (pch!=NULL && j<MAX_ARG_NUM) {
      if (i==0) {
        if (!strcmp("<",pch)) {
          pch=strtok(NULL," \n");
          stdin_filename=pch;
          pch=strtok(NULL," \n");
          continue;
        }
      }
      if (i==cnt-1) {
        if (!strcmp(">",pch)) {
          pch=strtok(NULL," \n");
          stdout_filename=pch;
          pch=strtok(NULL," \n");
          continue;
        }
      }
      currentcmd[j++]=pch;
      pch=strtok(NULL," \n");
    }
    if (!strcmp(cmd,"exit")) {
      printf("\n");
      exit(0);
    }
    if (!strcmp(currentcmd[0],"cd")) {
      if (j==2) {
        if (currentcmd[1][0]=='/') {//absoulte path
          strncpy(current_working_directory,currentcmd[1],MAX_PATH_LEN);
        } else {//relative path
          if (getcwd(current_working_directory,MAX_PATH_LEN)) {
            strncat(current_working_directory,"/",1);
            strncat(current_working_directory,currentcmd[1],MAX_PATH_LEN-strlen(current_working_directory));
          } else {
            perror("getcwd error cd");
          }
        }
        if (chdir(current_working_directory)<0) { //change cwd
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
    if (i<cnt-1)
      if (pipe(pipefd[i])<0) {
        perror("pipe error");
        return;
      }
    pid_t childpid=fork();
    if (childpid<0){
      perror("fork error");
      continue;
    }
    if (childpid==0) { //child process
      /*printf("child process pid=%d\n",getpid());*/
      if(i==0 && stdin_filename) {
        int rediroutfd=open(stdin_filename,O_RDONLY);
        if (rediroutfd<0) {
          perror("Redirect STDIN ERROR");
          exit(1);
        }
        dup2(rediroutfd,STDIN_FD); //redirect
      }
      if (i==cnt-1 && stdout_filename) {
        int rediroutfd=creat(stdout_filename,0644);
        if (rediroutfd<0) {
          perror("Redirect STDOUT ERROR");
          exit(1);
        }
        dup2(rediroutfd,STDOUT_FD); //redirect
      }
      if (i>0) {
        close(STDIN_FD);
        close(pipefd[i-1][1]);
        dup2(pipefd[i-1][0],STDIN_FD);
      }
      if (i<cnt-1) {
        close(STDOUT_FD);
        close(pipefd[i][0]);
        dup2(pipefd[i][1],STDOUT_FD);
      }
      if (execvp(currentcmd[0],currentcmd)<0) {
        perror(currentcmd[0]);
        exit(0);
      }
    } else {
      if (!background_job) {
        int tmp=wait(NULL);
        while (tmp!=childpid)
          tmp=wait(NULL);
      }
      if (i<cnt-1)
        close(pipefd[i][1]);
      if (i>0)
      close(pipefd[i-1][0]);
    }
  }
}


static void sigint_handler(int signo) { //disable Ctrl+c
  return;
}

int main() {
  //disable Ctrl+c
  signal(SIGINT,&sigint_handler);
  while (1) {
    if (generate_prompt(prompt)<0) {
      printf("Error in prompt generate!\n");
      exit(1);
    }
    fputs(prompt,stdout);
    if (fgets(cmd,CMD_MAX_LEN,stdin)==NULL) break;
    /*cmd[strlen(cmd)-1]='\0';*/
    background_job=0;
    execute_cmd();
  }
  printf("exit\n");
  return 0;
}
