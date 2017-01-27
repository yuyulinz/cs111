#include <stdlib.h>
#include <unistd.h> //getopt
#include <getopt.h> //getopt_long
#include <stdio.h>  //printf
#include <sys/types.h> //open
#include <sys/stat.h> //open
#include <fcntl.h> //open
#include <errno.h> //perror
#include <signal.h> //signal

void signal_handler(int signum)
{
  fprintf(stderr, "Segmentation Fault detected. Signal number: %d\n", signum);
  _exit(3);
}

int main(int argc, char *argv[])
{

  struct option long_options[] =
    {
      {"input=", required_argument, NULL, 'i'},
      {"output=", required_argument, NULL, 'o'},
      {"segfault", no_argument, NULL, 's'},
      {"catch", no_argument, NULL, 'c'},
      {0,0,0,0}
    };

  int fault = 0;   //0 means dont seg fault, 1 means segfault

  int ret = 0;
  while (1) {
    ret = getopt_long(argc, argv, "", long_options, NULL);
    if (ret == -1) break;

    switch (ret) {
      case 'i':              //changes the input to a file
      {
	int fd = open(optarg, O_RDONLY);
	if (fd == -1)
	  {
	    perror("Failed to open input file");
	    _exit(1);
	  }
	dup2(fd,0);
	close(fd);
	break;
      }
      case 'o':              //changes the output to a file
      {
	int fd = open(optarg, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
	  {
	    perror("Failed to create output file");
	    _exit(2);
	  }
	dup2(fd,1);
	close(fd);
	break;
      }
      case 's':             //forces a segfault by storing into a NULL pointer
      {
	fault = 1;   //set varible to segfault
	break;
      }
      case 'c':
      {
	signal(SIGSEGV, signal_handler);
	break;
      }
      default:
      {
	printf("error");
	exit(1);
      }
    }
  } 

  if ( fault == 1) {
  	char *x = NULL;
	*x = 'a';
  }

  char buf;
  while (read(0,&buf,1) != 0)
    {
      write(1,&buf,1);
    }
  return 0;

}
