#include "../libcs50/bag.h"
#include "../libcs50/hashtable.h"
#include "../libcs50/webpage.h"
#include "../libcs50/mem.h"
#include "../common/pagedir.h"
#include <stdio.h>
#include <string.h>



static void parseArgs(const int argc, char* argv[],
                      char** seedURLp, char** pageDirectoryp, int* maxDepth);
static void crawl(char* seedURL, char* pageDirectory, const int maxDepth);
static void pageScan(webpage_t* page, bag_t* pagesToCrawl, hashtable_t* pagesSeen);
static void logr(const char *word, const int depth, const char *url);


int main(int argc, char *argv[])
{
  char* seedURL;
  char* pageDirectory;
  int maxDepth;

  parseArgs(argc,argv,&seedURL,&pageDirectory,&maxDepth);
  printf("Parse Check:\n%s\n%s\n%d\n",seedURL,pageDirectory,maxDepth);
  crawl(seedURL,pageDirectory,maxDepth);

  return 0;
}
static
void parseArgs(int argc, char *argv[],char** seedURLp, char** pageDirectoryp, int* maxDepthp)
{
  char* progName = argv[0];
  if(argc != 4){
    fprintf(stderr,"usage: %s seedURL pageDirectory maxDepth\n",progName);
    exit(1);
  }

  //Save URL and pageDirectory
  *seedURLp = normalizeURL(argv[1]);
  *pageDirectoryp = argv[2];

  //CheckURL
  if (*seedURLp == NULL)
  {
    fprintf(stderr,"%s: %s is a invalid seedURL argument\n",progName, argv[1]);
    exit(1);
  }
  
  //Check depth
  if(sscanf(argv[3], "%d", maxDepthp) != 1 || (*maxDepthp < 0) || (*maxDepthp > 30)){
    fprintf(stderr,"%s: %s is an invalid depth argument\n",progName, argv[3]);
    exit(1);
  }

  if (!pagedir_init(*pageDirectoryp)) {
    fprintf(stderr, "%s: cannot initialize pageDirectory '%s'\n", progName, *pageDirectoryp);
    exit(1);
}
}

static
void crawl(char* seedURL, char* pageDirectory, const int maxDepth)
{
  hashtable_t* discoveredSites = hashtable_new(20);
  bag_t* sitesToVisit = bag_new();
  webpage_t* seedPage = webpage_new(seedURL,0,NULL);

  hashtable_insert(discoveredSites,seedURL,"");
  bag_insert(sitesToVisit,seedPage);

  webpage_t* currentWebpage;
  
  int docID = 1;
  while ((currentWebpage = bag_extract(sitesToVisit)) != NULL)
  {
    if (webpage_fetch(currentWebpage)){
      pagedir_save(currentWebpage,pageDirectory,docID);
      logr("Fetched",webpage_getDepth(currentWebpage),webpage_getURL(currentWebpage));
      docID++;
      if (webpage_getDepth(currentWebpage) != maxDepth)
      {
        logr("Scanning",webpage_getDepth(currentWebpage),webpage_getURL(currentWebpage));
        pageScan(currentWebpage,sitesToVisit,discoveredSites);
      }
    }
    webpage_delete(currentWebpage);
  }
  hashtable_delete(discoveredSites,NULL);
  bag_delete(sitesToVisit,webpage_delete);
}

static 
void pageScan(webpage_t* page, bag_t* pagesToCrawl, hashtable_t* pagesSeen){
  char* URLOnPage;
  int pos = 0;

  while ((URLOnPage = webpage_getNextURL(page,&pos))){
    logr("Found",webpage_getDepth(page),URLOnPage);
    if (isInternalURL(URLOnPage)){
      if(hashtable_insert(pagesSeen,URLOnPage,"")){
        
        logr("Added",webpage_getDepth(page),URLOnPage);
        webpage_t* urlWebpage = webpage_new(URLOnPage,webpage_getDepth(page)+1,NULL);
        if (urlWebpage != NULL){
          bag_insert(pagesToCrawl,urlWebpage);
        }else {
          mem_free(URLOnPage);
        }
      }else{
        logr("IgnDubl",webpage_getDepth(page),URLOnPage);
        mem_free(URLOnPage);
      }
    }else{
      logr("IgnExtrn",webpage_getDepth(page),URLOnPage);
      mem_free(URLOnPage);
    }
  }
}
// log one word (1-9 chars) about a given url                                   
static void logr(const char *word, const int depth, const char *url){
  printf("%2d %*s%9s: %s\n", depth, depth, "", word, url);
}