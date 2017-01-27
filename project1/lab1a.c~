#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/wait.h>

int d = 0;
int status;
void *writethread(void *arg);

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;
struct termios tattr;


void signal_handler(int signal)
{
  if (signal == SIGPIPE)
    exit(1);
}

void
reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode (void)
{
  //struct termios tattr;
  //  char *name;

  /* Make sure stdin is a terminal. */
  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (EXIT_FAILURE);
    }

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (reset_input_mode);

  /* Set the funny terminal modes. */
  tcgetattr (STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}


int
main (int argc, char *argv[])
{
  char c;
  int shell = 0;

  if (argc > 2)
    {
      printf("wrong number of arguments\n");
    }
  else if (argc == 2)
    {
      if (strcmp(argv[1], "--shell") == 0)
	shell = 1;
      else
	{
	  printf("No such argument\n");
	  exit(1);
	}
    }
    
  set_input_mode();    //set into noncanonacal mode with no echo

  if (shell == 1)     //if --shell arguemtn was used
    {


      /* create two pipes */
      int tochild[2];
      int toparent[2];
      pid_t child_id = -1;
      
      if (pipe(tochild) == -1){
	fprintf(stderr, "failed to create pipe to child\n");
	exit(1);
      }
      
      if (pipe(toparent) == -1){
	fprintf(stderr, "failed to create pipe to parent\n");
	exit(1);
      }

      /*fork into two processes*/
      child_id = fork();

      
      if (child_id == 0) //child
	{
	  close(tochild[1]);
	  close(toparent[0]);
	  dup2(toparent[1], STDOUT_FILENO);
	  dup2(tochild[0], STDIN_FILENO);
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

	  tattr.c_lflag &= ~(ISIG);
	  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
	  
	  pthread_t thread2;
	  if (pthread_create(&thread2, NULL, writethread,(void *) &toparent[0]) != 0 ){
	    fprintf(stderr, "Thread failed to create\n");
	    exit(1);
	  }
	  
	  while(1)
	    {
	      read (STDIN_FILENO, &buffer,1);
	      
	      if (buffer == '\003'){
		kill(child_id, 2);
		d = 2;
	      }
	      
	      if (buffer == '\004'){
		close(tochild[1]);
		close(toparent[1]);
		kill(child_id,1);
		d = 1;
	      }
	      
	      if (buffer == '\015' || buffer == '\012'){
		char b[2];
		b[0] = '\015';
		b[1] = '\012';
		write(STDOUT_FILENO, &b, 2);
		buffer = '\012';
	      } else {
		write(STDOUT_FILENO, &buffer, 1);
	      }
	      
	      write(tochild[1], &buffer, 1);
	    }
	}
      else if (child_id != 0)
	{
	  fprintf(stderr, "fork failed\n");
	  exit(1);
	}
      
  

  }
  else if (shell == 0)     //no -shell argument
    {
      
      while (1)
	{
	  read (STDIN_FILENO, &c, 1);
	  if (c == '\004')          /* C-d */
	    break;
	  else if (c == '\015' || c == '\012'){
	    char b[2];
	    b[0] = '\015';
	    b[1] = '\012';
	    write(STDOUT_FILENO, &b, 2);
	  }else
	    write(STDOUT_FILENO, &c, 1);
	}
    }
  
  return EXIT_SUCCESS;
}


/*thread function*/
void *writethread(void *arg)
{
  char buffer;
  
  while(1)
    {
      read(*((int *) arg), &buffer, 1);
      if (buffer == 0){
	wait(&status);
	printf("shell exit status: %d\n", WEXITSTATUS(status));
	exit(1);
      }
      else if (d == 1){
	wait(&status);
	printf("shell exit status: %d\n", WEXITSTATUS(status));
	exit(0);
      }
      else if (d == 2){
	wait(&status);
	printf("shell exit status: %d\n", WEXITSTATUS(status));
	exit(1);
      }
      write(STDOUT_FILENO, &buffer, 1);
    }
  return NULL;
}
