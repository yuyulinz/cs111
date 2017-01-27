#include <stdlib.h>
#include <stdio.h>
#include "SortedList.h"


void main(int argc, char *argv[])
{
  char key = rand();
  SortedList_t *dummy = malloc(sizeof(SortedList_t));
  dummy->prev = dummy;
  dummy->next = dummy;
  dummy->key = &key;

  printf("number of element: %d\n", SortedList_length(dummy));
  
  SortedListElement_t *newelement = malloc(sizeof(SortedListElement_t));
  char newkey = 'x';
  newelement->key = &newkey;
  SortedList_insert(dummy, newelement); 

  printf("number of element: %d\n", SortedList_length(dummy));


  SortedListElement_t *pointfind = SortedList_lookup(dummy,&newkey);

  printf("found key: %c \n", *(pointfind->key));
  
  free(dummy);
  free(newelement);


}
