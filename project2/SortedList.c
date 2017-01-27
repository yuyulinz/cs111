#define _GNU_SOURCE

#include <stdlib.h>
#include "SortedList.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>

int opt_yield = 0;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
  SortedListElement_t *previous = list;
  SortedListElement_t *next = list->next;

  while(next != list)
    {
      if (strcmp(element->key, next->key) <= 0)
	break;
      previous = next;
      next = next->next;
    }

  if(opt_yield & INSERT_YIELD){
    pthread_yield();
  }

  element->prev = previous;
  element->next = next;
  previous->next = element;
  next->prev = element;
}


int SortedList_delete (SortedListElement_t *element)
{
  
  SortedListElement_t *previous = element->prev;
  SortedListElement_t *next = element->next;

  if(opt_yield & DELETE_YIELD){
    pthread_yield();
  }
  
  if(previous == NULL || next == NULL)
    return 1;

  previous->next = next;
  next->prev = previous;

  element->prev = NULL;
  element->next = NULL;
  return 0;

}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
  SortedListElement_t *next = list->next;

  if(opt_yield & SEARCH_YIELD){
    pthread_yield();
  }
  
  while(next != list)
    {  
      if(next->key == key)
	return next;

      next = next->next;
    }

  return NULL;
}


int SortedList_length(SortedList_t *list)
{
  int count = 0;
  SortedListElement_t *next = list->next;

  if (opt_yield & SEARCH_YIELD){
    pthread_yield();

  }
  while(next != list)
    {
      count++;
      next = next->next;

      if( next == NULL)
	return -1;
    }
  return count;  
}
