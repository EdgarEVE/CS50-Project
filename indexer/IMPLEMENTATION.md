# CS50 TSE Indexer

## Implementation Spec

**Authors:** Brandon Wang, Ryan Sun, Jan Zuk, Edgar Fraire
**Date:** 05/22/2026

In this document we reference the [Requirements Specification](REQUIREMENTS.md) and [Design Specification](DESIGN.md) and focus on the implementation-specific decisions.
Here we cover the core subset relevant to the Indexer:

- Data structures
- Control flow: pseudo code for overall flow, and for each of the functions
- Detailed function prototypes and their parameters
- Error handling and recovery
- Testing plan

This spec describes the `indexer` and the `common` modules it relies on; the index tester (`indextest`) is described only as part of the testing plan.

## Data structures

The major data structure used in this subsystem is `index_t`, which provides an _inverted index_ mapping each word to a set of _(docID, count)_ pairs, where _count_ is the number of occurrences of that word in the document with that docID.
The `index_t` is implemented as a `hashtable` keyed by word, where each item is a `counters` object keyed by docID.

The number of slots in the hashtable cannot be known in advance.
When _building_ an index, we use a fixed size of 900 slots, defined as a constant (`#define NUMSLOTS 900`); this is a reasonable size for the word counts produced by the CS50 test sites.
When _loading_ an index from a file, the number of words is known, because the index file holds exactly one word per line; we count the lines in the file and use that count (a small multiple may also be used to keep the load factor low) to size the hashtable.

## Control flow

The Indexer is implemented in one file `indexer.c`, with three functions.

### main

The `main` function calls `parseArgs`, then `indexBuild`, then saves and deletes the index, then exits zero.
Pseudocode:

    parse the command line via parseArgs
    call indexBuild to build the index from pageDirectory
    call index_save to write the index to indexFilename
    call index_delete to free the index
    exit zero

### parseArgs

Given arguments from the command line, extract them into the function parameters; return only if successful.

- validate that exactly two arguments were provided
- for `pageDirectory`, call `pagedir_validate()` to confirm it is a directory produced by the Crawler (it must contain a readable `.crawler` file and a readable file named `1`)
- for `indexFilename`, verify the file can be opened for writing
- if any test fails, print an error message to stderr and exit non-zero

### indexBuild

Build an in-memory index from the webpage files in `pageDirectory`.
Pseudocode:

    create a new index_t with NUMSLOTS slots
    for docID = 1, 2, 3, ... (incrementing):
        load the webpage from pageDirectory/docID using pagedir_load
        if the load fails (no such file), stop the loop
        call indexPage to add the webpage's words to the index
        delete the webpage
    return the index

### indexPage

Scan a single webpage and add each of its words to the index.
Pseudocode:

    while there is another word in the webpage:
        if the word has at least 3 characters:
            normalize the word
            call index_add(index, word, docID)
        free the word

The logic of creating a new counter at 1 versus incrementing an existing one is handled within `index_add` (see below).

## Other modules

### pagedir

We extend the re-usable module `common/pagedir.c` (begun in Lab 4) with functions used by the Indexer.

Pseudocode for `pagedir_validate`:

    construct the pathname for the .crawler file in pageDirectory
    open that file for reading; if it cannot be opened, return false
    close that file
    construct the pathname for the file named 1 in pageDirectory
    open that file for reading; if it cannot be opened, return false
    close that file and return true

Pseudocode for `pagedir_load`:

    construct the pathname pageDirectory/docID
    open that file for reading; if it cannot be opened, return NULL
    read the URL from the first line
    read the depth from the second line
    read the rest of the file as the HTML contents
    create a webpage_t from the URL, depth, and HTML, and return it
    (return NULL on any failure)

### index

We add a `common/index.c` module that implements the `index_t` data structure and the functions to create, populate, query, save, load, and free an index.

`index_new` creates an empty index with a given number of slots, backed by a new hashtable.
It returns the new `index_t`, or `NULL` on failure.

`index_add` adds one occurrence of a word for a given docID.
It looks up the word in the hashtable: if no counters object exists for that word, it creates a new (empty) counters object and inserts it under that word.
It then increments the count for `docID` in that counters object (creating it at 1 if this is the first occurrence).

`index_find` returns the counters object associated with a given word, or `NULL` if the word is not in the index.
It is used to look up a word's _(docID, count)_ set.
This function is provided for use by the querier; it is not exercised by the indexer or indextest.

`index_save` writes an `index_t` to a file in the format defined by the Requirements Spec.
It iterates over the hashtable, and for each word it prints the word followed by each _(docID, count)_ pair from that word's counters, all separated by spaces.
It prints a newline only after all of a word's pairs have been written, so each line of the file holds exactly one word followed by all of its pairs.
It returns `true` on success and `false` if the file cannot be opened for writing.

`index_load` reads an index file back into a new `index_t`.
Pseudocode:

    open the index file for reading; if it cannot be opened, return NULL
    count the lines in the file (one per word) and use that count to size the hashtable
    create a new index of that size
    for each line of the file:
        read the word
        repeatedly read a (docID, count) pair with fscanf using the format "%d %d",
          checking its return value, until no more pairs remain on the line
        set the count for that word and docID in the index
    close the file and return the index

`index_delete` frees the index.
It iterates over the hashtable, freeing each word's counters object, then frees the hashtable and the `index_t` struct itself.

### word

We add a `common/word.c` module, shared with the querier, providing `normalizeWord`.
`normalizeWord` converts every letter of a word to lower-case **in place** and returns the same pointer (for convenience); it does not allocate new memory.
Normalizing words ensures that occurrences differing only in case are counted as the same word.

### libcs50

We leverage the modules of `libcs50`, most notably `hashtable`, `counters`, `webpage`, and `file`.
The `webpage` module represents pages as `webpage_t` objects and provides `webpage_getNextWord`, used by `indexPage` to step through the words of a page.
The `file` module provides helpers such as line counting and line reading, used by `pagedir_load` and `index_load`.

## Function prototypes

Detailed descriptions of each function's parameters and return values are provided as paragraph comments above each function's declaration in the corresponding `.h`/`.c` file, and are not repeated here.

### indexer

```c
int main(const int argc, char* argv[]);
static void parseArgs(const int argc, char* argv[],
                      char** pageDirectory, char** indexFilename);
static index_t* indexBuild(const char* pageDirectory);
static void indexPage(index_t* index, webpage_t* page, const int docID);
```

### index

```c
index_t* index_new(const int num_slots);
void index_add(index_t* index, const char* word, const int docID);
counters_t* index_find(index_t* index, const char* word);
bool index_save(index_t* index, const char* indexFilename);
index_t* index_load(const char* oldIndexFilename);
void index_delete(index_t* index);
```

### pagedir

```c
bool pagedir_init(const char* pageDirectory);
void pagedir_save(const webpage_t* page, const char* pageDirectory, const int docID);
bool pagedir_validate(const char* pageDirectory);
webpage_t* pagedir_load(const char* pageDirectory, const int docID);
```

### word

```c
char* normalizeWord(char* word);
```

## Error handling and recovery

All command-line parameters are rigorously checked before any data structures are allocated or work begins; problems result in a message printed to stderr and a non-zero exit status.

Out-of-memory errors are handled by variants of the `mem_assert` functions, which print a message to stderr and exit with a non-zero status.
We anticipate out-of-memory errors to be rare and thus allow the program to crash (cleanly) in this way.

All functions use defensive-programming tactics: they check their pointer parameters for `NULL` and take a safe action (returning `NULL`, returning `false`, or stopping) when given bad input.

Page files in the `pageDirectory` and index files are guaranteed by the specs to follow the documented format, so the indexer does not perform extensive error checking when reading them.

## Testing plan

### Unit testing

The `indextest` program serves as a unit test for the `index` module.
It loads an index file into an `index_t` (via `index_load`) and writes it back out to a new file (via `index_save`).
Comparing the two files confirms that `index_save` and `index_load` are correct inverses of one another.

### Integration / system testing

We write a script `testing.sh`, invoked by `make test` (run as `bash -v testing.sh`), that:

1. Runs `indexer` with invalid arguments, testing each failure mode:
   - no arguments;
   - one argument;
   - three or more arguments;
   - a `pageDirectory` that does not exist;
   - a `pageDirectory` that is not a Crawler directory (no `.crawler` file);
   - an `indexFilename` in a non-existent directory;
   - an `indexFilename` in a read-only directory;
   - an `indexFilename` that names an existing, read-only file.
2. Runs `indexer` on a variety of valid pageDirectories (e.g. `letters`, `toscrape`, `wikipedia`), producing an index file for each.
3. Runs `indextest` on each resulting index file to produce a second index file.
4. Compares the two index files with `indexcmp` (not `diff`, since lines and the pairs within a line may appear in any order) to confirm the indexes are equivalent.
5. Runs `valgrind` on both `indexer` and `indextest` over a moderate-sized case (e.g. `toscrape` at a small depth) to confirm there are no memory leaks or errors.

We save the output of these tests with `make test &> testing.out`.
