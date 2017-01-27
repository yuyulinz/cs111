#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <mcrypt.h>
#include <sys/stat.h>
#include <fcntl.h>

pid_t child_id = -1;
int exitcode = 0;

//encryption
MCRYPT tde;
int encryptbit = 0;


void *writethread(void *arg);

void error(char *msg)
{
  perror(msg);
  exit(1);
}

void signal_handler(int signal)
{
  if (signal == SIGPIPE)
    {
      kill(child_id, 1);
      exit(2);
    }
}

int main(int argc, char *argv[])
{

  //parse arguments
  struct option long_options[] =
    {
      {"port=", required_argument, NULL, 'p'},
      {"encrypt", no_argument, NULL, 'e'},
      {0,0,0,0}
    };

  int ret = 0;
  int port = -1;

  //encryption
  int i;
  char *key;
  char password[20];
  char *IV;
  int keysize = 16;
  int keyfd;
  
  while (1) {
    ret = getopt_long(argc, argv, "", long_options, NULL);
    if (ret == -1) break;

    switch (ret) {
    case 'p':               //initialize port
      port = atoi(optarg);
      break;
    case 'e':
      encryptbit = 1;

      //create key
      key = calloc(1, keysize);
      keyfd = open("my.key", O_RDWR);
      memset(password, '\0', 20);
      if(read(keyfd, &password, 16) < 1)
	exit(1);
      close(keyfd);

      //strcpy(password, "A_large_key");///////////this should be from a file
      memmove( key, password, strlen(password));

      //open encrypt
      tde = mcrypt_module_open("twofish", NULL, "cfb", NULL);
      if (tde == MCRYPT_FAILED) exit(1);

      //set IV
      IV = malloc(mcrypt_enc_get_iv_size(tde));

      for( i = 0; i < mcrypt_enc_get_iv_size(tde); i++)
	{
	  IV[i] = 'x';
	}

      //initialize the encrypt
      i = mcrypt_generic_init(tde, key, keysize, IV);
      if (i<0)
	{
	  mcrypt_perror(i);
	  exit(1);
	}
      break;
      
    default:
      {
	printf("getopt error");
	exit(1);
      }
    }
  }

  if(port == -1)
    {
      printf("Error: must specify port\n");
      exit(1);
    }
  
  

  //create socket, listen for client and connect
  int sockfd, newsockfd, portno, clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)error("ERROR opening socket");
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  portno = port;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");
  listen(sockfd,5);
  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0)
    error("ERROR on accept");

  //forking into two processes
  
  // create two pipes
  int tochild[2];
  int toparent[2];

  if (pipe(tochild) == -1){
    fprintf(stderr, "failed to create pipe to child\n");
    exit(1);
  }

  if (pipe(toparent) == -1){
    fprintf(stderr, "failed to create pipe to parent\n");
    exit(1);
  }

  //fork into two

  child_id = fork();

  if (child_id == 0) //child
    {
      close(tochild[1]);
      close(toparent[0]);
      dup2(toparent[1], STDOUT_FILENO);
      dup2(tochild[0], STDIN_FILENO);
      dup2(toparent[1], STDERR_FILENO);
      close(tochild[0]);
      close(toparent[1]);

      char *execvp_argv[2];
      char execvp_filename[] = "/bin/bash";
      execvp_argv[0] = execvp_filename;
      execvp_argv[1] = NULL;
      if (execvp(execvp_filename, execvp_argv) == -1) {
	fprintf(stderr, "execvp() failed\n");
	exit(1);
      }
    }
  else if (child_id > 0) //parent
    {
      signal(SIGPIPE, signal_handler);
      char buffer;
      close(tochild[0]);
      close(toparent[1]);
      dup2(newsockfd, STDOUT_FILENO);
      dup2(newsockfd, STDIN_FILENO);
      dup2(newsockfd, STDERR_FILENO);
      close(newsockfd);
      // tattr.c_lflag &= ~(ISIG);
      //tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);

      pthread_t thread2;
      if (pthread_create(&thread2, NULL, writethread,(void *) &toparent[0]) != 0 ){
	fprintf(stderr, "Thread failed to create\n");
	exit(1);
      }

      int n;
      while(1)
	{
	  n = read (STDIN_FILENO, &buffer,1);
	  if (encryptbit == 1)
	    mdecrypt_generic(tde, &buffer,1);
	  if (buffer == '\004' || n < 1){
	    close(tochild[1]);
	    close(toparent[0]);
	    kill(child_id,1);
	    exitcode = 1;
	    }
	  write(tochild[1], &buffer, 1);
	}
    }
  else if (child_id != 0)
    {
      fprintf(stderr, "fork failed\n");
      exit(1);
    }

  return 0;
}

void *writethread(void *arg)
{
  char buffer[1000];
  int n;
  while(1)
    {
      n = read(*((int *) arg), buffer, 999);
      if (encryptbit == 1)
	mcrypt_generic(tde, &buffer, n);
      buffer[n] = '\0';
      if (exitcode == 1){
	exit(1);
      }
      if (n < 1)
	{
	  kill(child_id,1);
	  exit(2);
	}
      write(STDOUT_FILENO, buffer, n);
    }
  return NULL;
}
