#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>

int nthreads = 1;
int niterations = 1;
int opt_yield = 0;  
char choose_lock = 'n';
pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
int m_spinlock = 0;

//main add function
void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    pthread_yield();
  *pointer = sum;
}


//thread function
void *threadfunction(void *arg)
{
  int i;
  long long previous;
  long long next;

  //add 1
  for( i = 0; i < niterations; i++)
    {
      //choose which lock to use
      switch(choose_lock)
	{
	case 'n':
	  add((long long*) arg, 1);
	  break;
	case 'm':
	  pthread_mutex_lock(&m_mutex);
	  add((long long*) arg, 1);
	  pthread_mutex_unlock(&m_mutex);
	  break;
	case 's':
	  while(__sync_lock_test_and_set(&m_spinlock,1));
	  add((long long*) arg, 1);
	  __sync_lock_release(&m_spinlock);
	  break;
	case 'c':
	  do{
	    previous = *((long long*) arg);
	    next = previous + 1;
	    if(opt_yield)
	      pthread_yield();
	  }while(__sync_val_compare_and_swap((long long*) arg, previous, next) != previous);
	  break;
	default:
	  printf("Error choosing lock\n");
	  exit(1);
	} 
    }

  //subtract 1
  for( i = 0; i < niterations; i++)
    {
      //choose which lock to use
      switch(choose_lock)
	{
	case 'n':
	  add((long long*) arg, -1);
	  break;
	case 'm':
	  pthread_mutex_lock(&m_mutex);
	  add((long long*) arg, -1);
	  pthread_mutex_unlock(&m_mutex);
	  break;
	case 's':
	  while(__sync_lock_test_and_set(&m_spinlock,1));
	  add((long long*) arg, -1);
	  __sync_lock_release(&m_spinlock);
	  break;
	case 'c':
	  do{
	    previous = *((long long*) arg);
	    next = previous - 1;
	    if(opt_yield)
	      pthread_yield();
	  }while(__sync_val_compare_and_swap((long long*) arg, previous, next) != previous);
	  break;
	default:
	  printf("Error choosing lock\n");
	  exit(1);
	} 
    }
  return NULL;
}


int main (int argc, char *argv[])
{
  //struct for parsing args
  struct option long_options[] =
    {
      {"threads=", optional_argument, NULL, 't'},
      {"iterations=", optional_argument, NULL, 'i'},
      {"yield=", no_argument, NULL, 'y'},
      {"sync=", required_argument, NULL, 's'},
      {0,0,0,0}
    };

  //parse args
  char *c;
  int ret = 0;
  while (1) {
    ret = getopt_long(argc, argv, "", long_options, NULL);
    if (ret == -1) break;
    switch (ret) {
    case 't':
      nthreads = atoi(optarg);
      break;
    case 'i':
      niterations = atoi(optarg);
      break;
    case 'y':
      opt_yield = 1;
      break;
    case's':
      c = optarg;
      choose_lock = *c;
      break;
    default:
      {
	printf("getopt error");
	exit(1);
      }
    }
  }
  //nthreads and niterations should not be zero
  if (nthreads == 0 || niterations == 0)
    {
      printf("Error: thread and iteration must be number\n");
      exit(1);
    }


  
  //print number of total ops, set count, begin timer
  long long totalops = nthreads*niterations*2; 
  printf("%d threads x %d iterations x (add + subtract) = %lld operations\n",nthreads,niterations,totalops);

  long long count = 0;

  struct timespec starttime;
  clock_gettime(CLOCK_MONOTONIC, &starttime);

  
  //create threads
  int i;
  pthread_t threads[nthreads];
  for(i = 0; i < nthreads; i++)
    {
	 if (pthread_create(&threads[i], NULL, threadfunction,(void *) &count) != 0 )
	   {
	     fprintf(stderr, "Thread failed to create\n");
	     exit(1);
	   }
    }
  
  //wait for threads 
  for(i = 0; i < nthreads; i++)
    {
      if (pthread_join(threads[i],NULL) != 0 )
	{
	  fprintf(stderr, "Thread failed to join\n");
	  exit(1);
	}
    }
  
  //end timer
  struct timespec endtime;
  clock_gettime(CLOCK_MONOTONIC, &endtime);

  //check error counting
  if (count != 0)
    {
      fprintf(stderr,"ERROR: final count = %lld\n", count);
    }

  //calculate time elapsed
  long long timetotal = (endtime.tv_sec - starttime.tv_sec) * 1000000000; 
  timetotal += endtime.tv_nsec;
  timetotal -= starttime.tv_nsec;
  printf("elasped time: %lld\n",timetotal);
  
  //calculate average time
  long long ave = timetotal/totalops;
  printf("per operation: %lldns\n",ave);

  return 0;
}
