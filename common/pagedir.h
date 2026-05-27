#ifndef PAGEDIR_H
#define PAGEDIR_H

#include "../libcs50/webpage.h"
#include <stdbool.h>


bool pagedir_init(const char* pageDirectory);
void pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID);

#endif