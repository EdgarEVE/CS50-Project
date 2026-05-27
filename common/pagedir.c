#include "pagedir.h"
#include "../libcs50/webpage.h"
#include "../libcs50/mem.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool pagedir_init(const char* pageDirectory){
  if (pageDirectory == NULL) {
    return false;
  }

  const char* name = "/.crawler";
  char *pathName = mem_malloc(strlen(pageDirectory) + strlen(name) + 1);
  
  if (pathName == NULL){
    return false;
  }
  
  strcpy(pathName, pageDirectory);
  strcat(pathName, name);

  FILE* fp;
  if ((fp = fopen(pathName,"w")) == NULL){
    free(pathName);
    return false;
  }

  fclose(fp);
  free(pathName);
  return true;
}

void pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID){
  //Check Parameters
  if (page == NULL){
    mem_assert(NULL,"Webpage is NULL, no saving occurred");
  }
  if (pageDirectory == NULL){
    mem_assert(NULL,"Page directory is NULL, no saving occurred");
  }
  if (docID <= 0){
    mem_assert(NULL,"DocID is negative or zero, no saving occurred");
  }

  //Allocate safe amount of space and make the new path from pageDirectory and ID
  char* pathName = mem_malloc(strlen(pageDirectory) + 20);
  sprintf(pathName, "%s/%d", pageDirectory, docID);
  
  //Check if possible to write into file
  FILE* fp;
  if ((fp = fopen(pathName, "w")) == NULL){
    mem_free(pathName);
    mem_assert(NULL,"Fatal issue with opening file");
  }
  //Don't need pathName, free
  mem_free(pathName);

  //Write the critical information needed in each
  fprintf(fp, "%s\n", webpage_getURL(page));
  fprintf(fp, "%d\n", webpage_getDepth(page));
  fprintf(fp, "%s\n", webpage_getHTML(page));
  fclose(fp);
}