#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include "SortedList.h"

//main list
SortedList_t *pl;

//number of lists
int nlists = 1;

//thread and iterations total
int nthreads = 1;
int niterations = 1;

//lock type
char choose_lock = 'n';
pthread_mutex_t *m_mutex;
int *m_spinlock;

//thread function
void *threadfunction(void *arg)
{
  
  //current element
  SortedListElement_t *m_element = (SortedListElement_t *) arg;

  
  //insert element
  int i;
  for(i = 0; i < niterations; i++)
    {
      int listnum = ((*((m_element+i)->key)+128) % nlists);    
      switch(choose_lock)
	{
	case 'n':
	  SortedList_insert(pl+listnum, (m_element+i));
	  break;
	case 'm':
	  pthread_mutex_lock(m_mutex+listnum);
	  SortedList_insert(pl+listnum, (m_element+i));
	  pthread_mutex_unlock(m_mutex+listnum);
	  break;
	case 's':
	  while(__sync_lock_test_and_set(m_spinlock+listnum,1));
	  SortedList_insert(pl+listnum, (m_element+i));
	  __sync_lock_release(m_spinlock+listnum);
	  break;
	default:
	  printf("Error choosing lock\n");
	  exit(1);
	}    
    }
  
  //get element
  int length = 0;
  for(i=0;i<nlists;i++)
    {
      switch(choose_lock)
	{
	case 'n':
	  length += SortedList_length(pl+i);
	break;
	case 'm':
	  pthread_mutex_lock(m_mutex+i);
	  length += SortedList_length(pl+i);
	  pthread_mutex_unlock(m_mutex+i);
	  break;
	case 's':
	  while(__sync_lock_test_and_set(m_spinlock+i,1));
	  length += SortedList_length(pl+i);
	  __sync_lock_release(m_spinlock+i);
	  break;
	default:
	  printf("Error choosing lock\n");
	  exit(1);
	}
    }
  
  //delete element
  for(i = 0; i < niterations; i++)
    {
      int listnum = ((*((m_element+i)->key)+128) % nlists);
      switch(choose_lock)
	{
	case 'n':
	  if (SortedList_delete(SortedList_lookup(pl+listnum, (m_element+i)->key)) != 0)
	    printf("error deleteing element!!");
	  break;
	case 'm':
	  pthread_mutex_lock(m_mutex+listnum);
	  if (SortedList_delete(SortedList_lookup(pl+listnum, (m_element+i)->key)) != 0)
	    printf("error deleteing element!!");
	  pthread_mutex_unlock(m_mutex+listnum);
	  break;
	case 's':
	  while(__sync_lock_test_and_set(m_spinlock+listnum,1));
	  if (SortedList_delete(SortedList_lookup(pl+listnum, (m_element+i)->key)) != 0)
	    printf("error deleteing element!!");
	  __sync_lock_release(m_spinlock+listnum);
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
      {"yield=", required_argument, NULL, 'y'},
      {"sync=", required_argument, NULL, 's'},
      {"lists=", required_argument, NULL, 'l'},
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
      c = optarg;
      while(*c != '\0')
	{
	  if(*c == 'i')
	    opt_yield = opt_yield | 0x01;
	  else if(*c == 'd')
	    opt_yield = opt_yield | 0x02;
	  else if(*c == 's')
	    opt_yield = opt_yield | 0x04;
	  else
	    printf("Error: yield option must be i,s, or d\n");
	  c = c+1;
	}
      break;
    case's':
      c = optarg;
      choose_lock = *c;
      break;
    case 'l':
      nlists = atoi(optarg);
      break;
    default:
      {
 	printf("getopt error");
	exit(1);
      }
    }
  }
  //nthreads and niterations should not be zero
  if (nthreads == 0 || niterations == 0 || nlists == 0)
    {
      printf("Error: thread, iteration, and lists must be number\n");
      exit(1);
    }


  
  //print number of total ops, set count, begin timer
  long long totalops = nthreads*niterations; 
  printf("%d threads x %d iterations x (add + subtract) = %lld operations\n",nthreads,niterations,(totalops*2));

  //initialize the locks
  m_mutex = malloc(nlists*sizeof(pthread_mutex_t));
  m_spinlock = malloc(nlists*sizeof(int));
  
  //initialize the list and locks
  pl = malloc(nlists*sizeof(SortedListElement_t));
  int i;
  for(i = 0; i < nlists; i++)
    {
      pthread_mutex_init((m_mutex+i),NULL);
      *(m_spinlock+i) = 0;
      (pl+i)->next = pl+i;
      (pl+i)->prev = pl+i;    
      (pl+i)->key = NULL;
    }
  
  //create the elements and keys
  SortedListElement_t *elements = malloc(totalops*sizeof(SortedListElement_t));
  char *ch = malloc(totalops*sizeof(char));

  srand(time(NULL));
  //declare the keys
  int k;
  for(k = 0; k < totalops; k++)
    {  
      *(ch+k) = (char) rand();
      (elements+k)->next = NULL;
      (elements+k)->prev = NULL;
      (elements+k)->key = (ch+k);
    }
  
  //start time
  struct timespec starttime;
  clock_gettime(CLOCK_MONOTONIC, &starttime);

  
  //create threads
  pthread_t threads[nthreads];
  for(i = 0; i < nthreads; i++)
    {
      //pass in the elements heree
      if (pthread_create(&threads[i], NULL, threadfunction,(void *) (elements+(i*niterations))) != 0 )
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
  
  //check error counting, check for the corrent behavior here
  if (SortedList_length(pl) != 0)
    fprintf(stderr, "ERROR: list size = %d\n", SortedList_length(pl));

  //free up allocated mem
  free(pl);
  free(ch);
  
  //calculate time elapsed
  long long timetotal = (endtime.tv_sec - starttime.tv_sec) * 1000000000; 
  timetotal += endtime.tv_nsec;
  timetotal -= starttime.tv_nsec;
  printf("elasped time: %lld\n",timetotal);
  
  //calculate average time
  long long ave = timetotal/(totalops*2);
  printf("per operation: %lldns\n",ave);
  
  return 0;
}
