#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_SIZE_OF_CMDLINE 2048

int getArguments(char **, int, int *, int);
void runShell();
void catchSIGINT(int);
int runBackgroundProcess(char*, char*, char**, int, int);
int redirectIO(char*, char*, char*, int, int);

int main(){
  runShell();
}

void runShell(){

  struct sigaction SIGINT_action = {0};
  SIGINT_action.sa_handler = catchSIGINT;
  sigfillset(&SIGINT_action.sa_mask);
  //SIGINT_action.sa_flags = SA_RESTART;
  sigaction(SIGINT, &SIGINT_action, NULL);

  char *parsedCmds[MAX_SIZE_OF_CMDLINE];
  int numCmds;
  int childExitMethod = -5;
  int status;
  int fileDescriptor;
  int result;
  int inputFileCheck = 0;
  char *inputFile;
  int outputFileCheck = 0;
  char *outputFile;
  int childPIDS[100];
  int PIDindex = 0;
  int aPID;
  int i;
  char firstCommand[100];
  char checkComment;
  char doubleDollar[2] = "$$";

  while(1)
  {


    pid_t spawnpid = -5;
    int isRedirection = 0;
    numCmds = getArguments(parsedCmds, childExitMethod, childPIDS, PIDindex);
    //printf("here\n" );
    int background = 0;

    strcpy(firstCommand, parsedCmds[0]);





    for (i = 0; i < numCmds; i++)
    {
      if (strcmp(parsedCmds[i], "<") == 0){
        //input file at i+1
        isRedirection = 1;
        inputFile = parsedCmds[i+1];
        inputFileCheck = 1;
      //  printf("input: %s\n", inputFile);
      }
      else if (strcmp(parsedCmds[i], ">") == 0)
      {
        //output file at i+1
        isRedirection = 1;
        outputFile = parsedCmds[i+1];
        outputFileCheck = 1;
      //  printf("output: %s\n", outputFile);
      }
    }

    char * result = strstr(parsedCmds[numCmds-1], doubleDollar);
      if (strstr(parsedCmds[numCmds-1], doubleDollar) != NULL){
        char * token = strtok(parsedCmds[numCmds-1], doubleDollar);
        sprintf(parsedCmds[numCmds-1] , "%s%d", token, getpid());
      }


    if (numCmds != 0 )
    {

      if(strcmp(parsedCmds[numCmds-1], "&") == 0){
      //  printf("here\n");
        parsedCmds[numCmds-1] = '\0';
        background = 1;
        if(isRedirection == 0 ){
          //printf("background process with NO specified redirection files\n");
          childPIDS[PIDindex] = runBackgroundProcess("/dev/null", "/dev/null", parsedCmds, inputFileCheck, outputFileCheck);
          PIDindex++;
        }
        // if(isRedirection == 1){
        //   //printf("background process with specified redirection files\n");
        //   runBackgroundProcess(inputFile, outputFile, parsedCmds, inputFileCheck, outputFileCheck);
        //
        // }
      }

      else if (isRedirection == 1 && background == 0)
      {
        //printf("forground process with redirection\n");
        childExitMethod = redirectIO(inputFile, outputFile, parsedCmds[0], inputFileCheck, outputFileCheck);
      }

      else if (strcmp(parsedCmds[0], "exit") == 0)
      {
        printf("Exiting Shell...\n");
        exit(0);
      }

      else if (strcmp(parsedCmds[0], "cd") == 0)
      {
        if (numCmds == 1){chdir(getenv("HOME"));}
        else{ chdir(parsedCmds[1]); }
      }

      else if (strcmp(parsedCmds[0], "status") == 0)
      {
        if (WIFEXITED(childExitMethod) != 0){printf("exit value %d\n",   WEXITSTATUS(childExitMethod));}
        else if (WIFSIGNALED(childExitMethod) != 0){printf("terminated by signal %d\n", WTERMSIG(childExitMethod));}
      }

      else if (firstCommand[0] == '#')
      {
        //printf("comment\n");
      }

      else
      {
        spawnpid = fork();
        switch (spawnpid)
        {
          case -1:
            perror("Error Creating Child Process! UHH OH");
            exit(1);
          break;
            case 0:
              if (execvp(*parsedCmds, parsedCmds) < 0){perror("Error executing");exit(1);}
            default:
              waitpid(spawnpid, &childExitMethod, 0);
        }
      }
    }
  }
}

int runBackgroundProcess(char* inputFile, char* outputFile, char** parsedCmds, int inputFileCheck, int outputFileCheck){
  //printf("%s %s %s %d %d \n", inputFile, outputFile, parsedCmd, inputFileCheck, outputFileCheck);
  int spawnpid = -5;
  int childExitMethod = -5;
  int result, fileDescriptor1, fileDescriptor2;

  // printf("%s\n",parsedCmd);


  spawnpid = fork();
  switch (spawnpid)
  {
    case -1:
      perror("Error Creating Child Process! UHH OH");
      exit(1);
    break;
      case 0:

        if (inputFileCheck == 1)
        {
          fileDescriptor1 = open(inputFile, O_RDONLY);
          if (fileDescriptor1 == -1){ perror("Could not open input file"); printf("%s\n", inputFile); exit(1);}
          result = dup2(fileDescriptor1, 0);
          if (result == -1) { perror("source dup2()"); exit(2); }
          fcntl(fileDescriptor1, F_SETFD, FD_CLOEXEC);
        }
        if(outputFileCheck == 1)
        {
          fileDescriptor2 = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fileDescriptor2 == -1){ perror("Could not open outputFile file"); exit(1);}
          result = dup2(fileDescriptor2, 1);
          if (result == -1) { perror("source dup2()"); exit(2); }
          fcntl(fileDescriptor2, F_SETFD, FD_CLOEXEC);
        }
        if (execvp(*parsedCmds, parsedCmds) < 0){perror("Error executing");exit(1);}
      default:
        printf("background pid is %d\n",spawnpid);
        waitpid(spawnpid, &childExitMethod, WNOHANG);
  }
  return spawnpid;
}

int getArguments(char **parsedCmds, int childExitMethod, int *childPIDS, int PIDindex){
  char *rawInputText = NULL;
  int numCharInInput = -5;

  size_t bufferSize = 0;

  while(1)
  {
    int i;
    int aPID;
    for (i = 0; i < PIDindex; i++)
    {
      aPID = waitpid(childPIDS[i], &childExitMethod, WNOHANG);
      if (aPID > 0)
      {
        printf("background pid %d is done: ", aPID);
        childPIDS[i] = '\0';
        if (WIFEXITED(childExitMethod) != 0){printf("exit value %d\n",   WEXITSTATUS(childExitMethod));}
        else if (WIFSIGNALED(childExitMethod) != 0){printf("terminated by signal %d\n", WTERMSIG(childExitMethod));}
      }
    }
    printf(":");

    numCharInInput = getline(&rawInputText, &bufferSize, stdin);

    if (numCharInInput == -1)
    {
      clearerr(stdin);
    }
    else
    {
      break;
    }
  }

  rawInputText[strcspn(rawInputText, "\n")] = '\0';
  // printf("input text RAW :|%s|\n", rawInputText);
  //prase raw input into array of strings
  const char delim[2] = " ";
  int i = 1;
  char *token = strtok(rawInputText, delim);
  parsedCmds[0] = token;
  while(token != NULL)
  {
    //printf("%s\n", token);
    token = strtok(NULL, delim);
    parsedCmds[i] = token;
    i++;
  }
  //printf("parsed %s\n", parsedCmds[0]);
  //that array of strings needs to be passed to runshell where
  // the commands will be executed
  return(i-1);
}

void catchSIGINT(int signo){
  char* message = "\n";

  write(STDOUT_FILENO, message, 28);
}

int redirectIO(char * inputFile, char * outputFile, char *parsedCmd, int inputFileCheck, int outputFileCheck){

  int spawnpid = -5;
  int childExitMethod = -5;
  int result, fileDescriptor1, fileDescriptor2;

  // printf("%s\n",parsedCmd);


  spawnpid = fork();
  switch (spawnpid)
  {
    case -1:
      perror("Error Creating Child Process! UHH OH");
      exit(1);
    break;
      case 0:
        if (inputFileCheck == 1)
        {
          fileDescriptor1 = open(inputFile, O_RDONLY);
          if (fileDescriptor1 == -1){ perror("Could not open input file"); exit(1);}
          result = dup2(fileDescriptor1, 0);
          if (result == -1) { perror("source dup2()"); exit(2); }
          fcntl(fileDescriptor1, F_SETFD, FD_CLOEXEC);
        }
        if(outputFileCheck == 1)
        {
          fileDescriptor2 = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fileDescriptor2 == -1){ perror("Could not open outputFile file"); exit(1);}
          result = dup2(fileDescriptor2, 1);
          if (result == -1) { perror("source dup2()"); exit(2); }
          fcntl(fileDescriptor2, F_SETFD, FD_CLOEXEC);
        }
        if (execlp(parsedCmd, parsedCmd, NULL) < 0){perror("Error executing");exit(1);}
      default:
        waitpid(spawnpid, &childExitMethod, 0);
  }
  return childExitMethod;

}
