#define _GNU_SOURCE

#include <stdlib.h>
#include "SortedList.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>

int opt_yield = 0;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
  int listnum;
  //if(list->master == 1)
  //  {
      listnum = (*(element->key) + 128) % list->nlists;
      
      SortedList_t *temp = (list->lists)+listnum;
      //  }
      //else if(list->master == 0)
      //{
      SortedListElement_t *previous = temp;
      SortedListElement_t *next = temp->next;
      
      while(next != temp)
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
      //}
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

  //if(list->master == 1)
  // {
      int listnum = (*(key) + 128) % list->nlists;
      SortedList_t *temp = (list->lists)+listnum;
      // }
      //else if(list->master == 0)
      //{
      SortedListElement_t *next = temp->next;
      
      if(opt_yield & SEARCH_YIELD){
	pthread_yield();
      }
      
      while(next != temp)
	{  
	  if(next->key == key)
	    return next;
	  
	  next = next->next;
	}
      
      return NULL;
      //}
}

int SortedList_length(SortedList_t *list)
{
  int count = 0;
  int i;
  for (i = 0; i < list->nlists; i++)
    {
      
      SortedListElement_t *next = ((list->lists)+i)->next;
      
      if (opt_yield & SEARCH_YIELD){
	pthread_yield();	
      }
      while(next != ((list->lists)+i))
	{
	  count++;
	  next = next->next;
	  
	  if( next == NULL)
	    return -1;
	}
    }
  return count;  
}
