/* 
 * counters.c - CS50 'counters' module
 *
 * see counters.h for more information.
 *
 * Edgar Fraire May 2026
 */
#include <stdio.h>
#include <stdbool.h>
#include "mem.h"

/**************** local types ****************/
typedef struct countersnode{
	int key;
	int count;
	struct countersnode* next;
}countersnode_t;


/**************** global types ****************/
typedef struct counters {
	struct countersnode* head;
} counters_t;  // opaque to users of the module

/**************** functions ****************/
static countersnode_t*
countersnode_grab(counters_t* ctrs, int key);
static countersnode_t*
countersnode_new(int key);
/**************** FUNCTION ****************/

/**************** counters_new ****************/
/* see counters.h for description */

counters_t* counters_new(void){
	counters_t* counters = mem_malloc(sizeof(counters_t));

  if (counters == NULL) {
    return NULL;              // error allocating counter set
  } else {
    // initialize contents of counter set structure
    counters->head = NULL;
    return counters;
  }
}

/**************** counters_add ****************/
/* see counters.h for description */
int counters_add(counters_t* ctrs, const int key){
	if (ctrs != NULL && key >= 0){
		//Look through the set and if key exist, increment and return
		countersnode_t* node = countersnode_grab(ctrs,key);
		if (node != NULL){
			node->count += 1;
			return node->count;
		}
		//No key present, make a new node
		countersnode_t* new = countersnode_new(key);
		if (new != NULL) {
      // add it to the head of the list
      new->next = ctrs->head;
      ctrs->head = new;
			return 1; //A new counter is always 1
    }
	}
	return 0;
}

//Make a new countersnode for inserting
static countersnode_t*
countersnode_new(int key){
	countersnode_t* node = mem_malloc(sizeof(countersnode_t));

  if (node == NULL) {
    // error allocating memory for node; return error
    return NULL;
  } else {
    node->key = key;
		node->count = 1;
    node->next = NULL;
    return node;
  }
}

//search through the data structure for a node that matches the key
static countersnode_t*
countersnode_grab(counters_t* ctrs, int key){
	for (countersnode_t* node = ctrs->head; node != NULL; node = node->next){
		if (node->key == key){
		  return node;
		}
	}
	return NULL;
}
/**************** counters_get ****************/
/* see counters.h for description */
int counters_get(counters_t* ctrs, const int key){
	if (ctrs != NULL && key >= 0){
		countersnode_t* node = countersnode_grab(ctrs,key);
		if (node != NULL){
			return node->count;
		}
	}
	return 0;
}

/**************** counters_set ****************/
/* see counters.h for description */

bool counters_set(counters_t* ctrs, const int key, const int count){
	if (ctrs != NULL && key >= 0 && count >= 0){
		countersnode_t* node = countersnode_grab(ctrs,key);
		if (node != NULL){
			node->count = count;
			return true;
		}
		//can't find node with specific key, make a new node
		countersnode_t* new = countersnode_new(key);
		if (new != NULL) {
			new->count = count;
      // add it to the head of the list
      new->next = ctrs->head;
      ctrs->head = new;
			return true;
    }
	}
	return false;
}

/**************** counters_print ****************/
/* see counters.h for description */

void counters_print(counters_t* ctrs, FILE* fp){
	if (fp != NULL){
		if (ctrs != NULL){
			fputc('{',fp);
			for (countersnode_t* node = ctrs->head; node != NULL; node = node->next){
				fprintf(fp,"%d=%d",node->key,node->count);
				if (node->next != NULL){
					fputc(',',fp);

				}
			}
			fputc('}',fp);
		}else{
			fputs("(null)",fp);
		}
		
		
	}
	
}

/**************** counters_iterate ****************/
/* see counters.h for description */

void counters_iterate(counters_t* ctrs, void* arg, void (*itemfunc)(void* arg, const int key, const int count)){
	if (ctrs != NULL && itemfunc != NULL){
		for (countersnode_t* node = ctrs->head; node != NULL; node = node->next){
			(*itemfunc)(arg, node->key,node->count); 
		}
	}
}

/**************** counters_delete ****************/
/* see counters.h for description */

void counters_delete(counters_t* ctrs)
{
  if (ctrs != NULL) {
    for (countersnode_t* node = ctrs->head; node != NULL; ) {
      countersnode_t* next = node->next;     // remember what comes next
      mem_free(node);                   // free the node
      node = next;                      // and move on to next
    }
    mem_free(ctrs);
  }
}
