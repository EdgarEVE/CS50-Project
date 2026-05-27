/* 
 * hashtable.c - CS50 'hashtable' module
 *
 * see hashtable.h for more information.
 *
 * Edgar Fraire May 2026
 */

#include <stdio.h>
#include <stdbool.h>
#include "mem.h"
#include "set.h"
#include "hash.h"


/**************** global types ****************/
typedef struct hashtable {
	int num_slots;
	struct set** arraySet;
} hashtable_t;  // opaque to users of the module

/**************** functions ****************/

/**************** hashtable_new ****************/
/* see hashtable.h for description */

hashtable_t* hashtable_new(const int num_slots){
	if (num_slots > 0){
		hashtable_t* newHashtable = mem_malloc(sizeof(hashtable_t));
		if (newHashtable != NULL){
      //Set slots and allocate the array
			newHashtable->num_slots = num_slots;
			newHashtable->arraySet = mem_malloc(num_slots * sizeof(set_t*));
			if (newHashtable->arraySet != NULL){
				//Allocate each individual set and return NULL if one fails;
				for(int i = 0; i < num_slots; i++){
					newHashtable->arraySet[i] = set_new();
					if (newHashtable->arraySet[i] == NULL){
						return NULL;
					}
				}
				//Every set allocated correctly, return the final hashtable
				return newHashtable;
			}else{
				return NULL;
			}
		}
	}
	return NULL;
}

/**************** hashtable_insert ****************/
/* see hashtable.h for description */

bool hashtable_insert(hashtable_t* ht, const char* key, void* item){
  if (ht != NULL && key != NULL && item != NULL){
    //Get the hash associated with the key string
    unsigned long hash = hash_jenkins(key,ht->num_slots);
    //Use the pointer associated with the hash
    return set_insert(ht->arraySet[hash],key,item);
  }
  return false;
}

/**************** hashtable_find ****************/
/* see hashtable.h for description */

void* hashtable_find(hashtable_t* ht, const char* key){
  if (ht != NULL && key != NULL){
    unsigned long hash = hash_jenkins(key,ht->num_slots);
    return set_find(ht->arraySet[hash],key);
  }
  return NULL;
}

/**************** hashtable_print ****************/
/* see hashtable.h for description */

void hashtable_print(hashtable_t* ht, FILE* fp, void (*itemprint)(FILE* fp, const char* key, void* item)){
  if (fp != NULL)
  {
    if (ht != NULL){
      for(int i = 0; i < ht->num_slots; i++){
        if (itemprint != NULL)
        {
          set_print(ht->arraySet[i],fp,itemprint);
        }
        fputc('\n',fp);
      }
    }else {
      fputs("(null)",fp);
    }
  }
}

/**************** hashtable_iterate ****************/
/* see hashtable.h for description */

void hashtable_iterate(hashtable_t* ht, void* arg, void (*itemfunc)(void* arg, const char* key, void* item)){
  if (ht != NULL && itemfunc != NULL){
    for (int i = 0; i < ht->num_slots; i++){
      set_iterate(ht->arraySet[i],arg,itemfunc);
    }
  }
}

/**************** hashtable_delete ****************/
/* see hashtable.h for description */

void hashtable_delete(hashtable_t* ht, void (*itemdelete)(void* item)){
  if (ht != NULL){
    for (int i = 0; i < ht->num_slots; i++){
      set_delete(ht->arraySet[i],itemdelete);
    }
    mem_free(ht->arraySet);
    mem_free(ht);
  }
}