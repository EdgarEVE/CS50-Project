/* 
 * set.c - CS50 'set' module
 *
 * see set.h for more information.
 *
 * Edgar Fraire May 2026
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mem.h"
#include "set.h"
#include <stdbool.h>

/**************** local types ****************/
typedef struct setnode {
  char* key;
  void* item;
  struct setnode* next;
} setnode_t;

/**************** global types ****************/
typedef struct set {
  struct setnode* head;
} set_t;

static setnode_t* setnode_new(const char* key,void* item);


set_t* set_new(void){
  set_t* set = mem_malloc(sizeof(set_t));

  if (set == NULL) {
    return NULL;
  }else {
    set->head = NULL;
    return set;
  }
}



/**************** set_insert ****************/
/* see set.h for description */

bool set_insert(set_t* set, const char* key, void* item){
  setnode_t* currentNode;
  setnode_t* newNode;
  if (set != NULL && key != NULL && item != NULL) {
    //check if set is empty
    if (set->head != NULL) {
      currentNode = set->head;
      //Check if the key already exist
      //Check head element, iterate through the rest
      if (strcmp(currentNode->key, key) == 0) {
        return false;
      }
      //Go through each element and check if any key matches
      while (currentNode->next != NULL) {
        currentNode = currentNode->next;
        if (strcmp(currentNode->key, key) == 0) {
          return false;
        }
      }
      //It looked through the whole set and found no node with a similar key 
      //We are on the last node with the next pointer being null, make a new node
      newNode = setnode_new(key,item);
      if (newNode == NULL) {
        return false;
      }
      //Making sure the values are initialized
      newNode->next = NULL;
      currentNode->next = newNode;
      return true;   
    }else{
      set->head = setnode_new(key,item);
      if (set->head == NULL) {
        return false;     // allocation failed
      }
      return true;
    }
  }
  return false;
}

/**************** set_find ****************/
/* see set.h for description */

void* set_find(set_t* set, const char* key){
  //Safety check of the set and key, return NULL if they are NULL
  if (set == NULL || key == NULL) {
    return NULL;
  }
  setnode_t* currentNode = set->head;
  //Iterate through the set
  while (currentNode != NULL) {
    if (strcmp(currentNode->key, key) == 0) {
      //Found the right key so return the pointer to the item
      return currentNode->item;
    }else {
      //Keep iterating
      currentNode = currentNode->next;
    }
  }
  //Can't find node, return NULL
  return NULL;
}

/**************** set_print ****************/
/* see set.h for description */

void set_print(set_t* set, FILE* fp, void (*itemprint)(FILE* fp, const char* key, void* item) ){
  if (fp != NULL) {
    if (set != NULL) {
      fputc('{', fp);
      for (setnode_t* node = set->head; node != NULL; node = node->next) {
        // print this node
        if (itemprint != NULL) { // print the node's item 
          (*itemprint)(fp,node->key, node->item); 
          if (node->next != NULL){
            fputc(',', fp); 
          } 
        }
      }
      fputc('}', fp);
    } else {
      fputs("(null)", fp);
    }
  }
}

/**************** set_iterate ****************/
/* see set.h for description */

void set_iterate(set_t* set, void* arg, void (*itemfunc)(void* arg, const char* key, void* item)){
  if (set != NULL && itemfunc != NULL) {
    // call itemfunc with arg, on each item
    for (setnode_t* node = set->head; node != NULL; node = node->next) {
      (*itemfunc)(arg, node->key, node->item); 
    }
  }
}

/**************** set_delete ****************/
/* see set.h for description */

void set_delete(set_t* set, void (*itemdelete)(void* item)){
  if (set != NULL) {
    for (setnode_t* node = set->head; node != NULL; ) {
      if (itemdelete != NULL) {         // if possible...
        (*itemdelete)(node->item);      // delete node's item
      }
      setnode_t* next = node->next;     // remember what comes next
      char* key = node->key;
      mem_free(key);
      mem_free(node);                   // free the node
      node = next;                      // and move on to next
    }
    mem_free(set);
  }
}

//Create a setnode
static setnode_t*  // not visible outside this file
setnode_new(const char* key, void* item)
{
  setnode_t* node = mem_malloc(sizeof(setnode_t));
  if (node != NULL){
    //Size of string key
    size_t len = strlen(key) + 1;
    char *setKey = mem_malloc(len);

    if (setKey != NULL) {
      strcpy(setKey,key);
      node->key = setKey;
      node->item = item;
      node->next = NULL;
      return node;
    }
  }
  return NULL;
}
