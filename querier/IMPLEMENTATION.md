# CS50 TSE Querier

## Implementation Spec

**Authors:** Brandon Wang, Ryan Sun, Jan Zuk, Edgar Fraire
**Date:** 05/22/2026

In this document we reference the [Requirements Specification](REQUIREMENTS.md) and [Design Specification](DESIGN.md) and focus on the implementation-specific decisions.
Here we cover the core subset relevant to the Querier:

- Data structures
- Control flow: pseudo code for overall flow, and for each of the functions
- Detailed function prototypes and their parameters
- Error handling and recovery
- Testing plan

The querier targets the full specification: it supports `and`/`or` operators with
`and` taking precedence over `or`, and prints the matching documents ranked in
decreasing order of score.

## Data structures

The querier uses the following data structures.

The **index** (`index_t`, from the `common/index` module) is an inverted index mapping
each word to a `counters` object of _(docID, count)_ pairs.
It is loaded from `indexFilename` with `index_load`, which sizes the underlying hashtable
from the number of lines in the index file (one word per line).

A **token array** holds the words and operator literals of a single query.
It is implemented as an array of `char*`, each pointing to a word _within_ the query
string itself; the query string is normalized (lower-cased) and split in place by writing
string terminators over the spaces, so no per-word memory is allocated.
The array and a count of its entries are passed together to the query functions.

Two **`counters` accumulators** are used while interpreting a query: `andSeq`, the running
intersection within the current andsequence, and `orResult`, the running union across
completed andsequences.
Both are fresh `counters` objects, independent of those owned by the index.

A small **ranking struct** (a `twoCounts`-style helper holding a docID and a score, or a
pair of integers) is used by the iterate callbacks that find the maximum-scoring document
and that combine counters.

## Control flow

The Querier is implemented in one file `querier.c`.
Its functions are described below.

### main

    call parseArgs to validate the command line
    load the index from indexFilename with index_load
    loop:
        print the prompt if stdin is a tty
        read one line from stdin; on EOF, break
        process the line (tokenize, validate, match, rank-and-print)
        free the line and the token array
    delete the index
    exit zero

### parseArgs

Given the command-line arguments, validate them; return only if successful.

- verify exactly two arguments were given
- for `pageDirectory`, call `pagedir_validate()` to confirm it is a directory produced by
  the Crawler (a readable `.crawler` file and a readable file named `1` are present)
- for `indexFilename`, verify the file can be opened for reading
- on any failure, print an error message to stderr and exit non-zero

### tokenize

Given a query line, validate its characters and split it into a token array.
Pseudocode:

    scan the line: if any character is neither a letter nor a space,
      print "Error: bad character 'X' in query." and return failure
    normalize the line by lower-casing every letter in place
    walk the line, treating runs of spaces as delimiters:
      at the start of each word, record a pointer into the array;
      write a '\0' over the space(s) following each word
    store the number of words found
    return success (the caller checks for a zero-word query and prints nothing)

### validateQuery

Given the token array, confirm it obeys the grammar's structural rules.
Pseudocode:

    if the first token is 'and' or 'or', print the matching error and return false
    if the last token is 'and' or 'or', print the matching error and return false
    for each adjacent pair of tokens:
      if both are operators, print the matching error and return false
    return true

The specific error messages follow the examples in the Requirements Spec, e.g.
`Error: 'and' cannot be first`, `Error: 'or' cannot be last`,
`Error: 'and' and 'or' cannot be adjacent`.

### matchQuery

Given a validated token array and the index, produce a `counters` of _(docID, score)_.
This follows the grammar `query ::= <andsequence> [or <andsequence>]...`.
Pseudocode:

    create an empty counters orResult
    create an empty counters andSeq
    set firstWord = true
    for each token:
        if token is "or":
            merge andSeq into orResult
            delete andSeq; create a new empty andSeq
            set firstWord = true
        else if token is "and":
            continue (the literal 'and' is structural)
        else (a word):
            look up the word with index_find
            if firstWord:
                if the word was found, copy its counters into andSeq
                set firstWord = false
            else:
                intersect andSeq with the word's counters
                (a not-found word intersects with the empty set, zeroing andSeq)
    merge andSeq into orResult
    delete andSeq
    return orResult

### rankAndPrint

Given the result `counters` and the `pageDirectory`, print the matching documents.
Pseudocode:

    count the non-zero entries in the result counters
    if the count is 0, print "No documents match." and return
    repeat (count times):
        iterate to find the (docID, score) pair with the highest score
        read the URL from the first line of pageDirectory/docID
        print "score N doc M: URL"
        set that docID's count to 0
    print a separator line between queries

## Set operations

The AND and OR operations are implemented with `counters_iterate`; the `counters` module
itself is not modified.

**Intersection (AND)** — `andSeq` is intersected with a word's `counters`:

    iterate over andSeq; for each (docID, count):
        look up the word's count for that docID with counters_get (0 if absent)
        counters_set andSeq[docID] = min(count, wordCount)

**Union (OR)** — `andSeq` is merged into `orResult`:

    iterate over andSeq; for each (docID, count):
        look up orResult's current count for that docID with counters_get
        counters_set orResult[docID] = currentCount + count

Because `counters_set` creates a key that does not yet exist, the union naturally adds
docIDs that are new to `orResult`.

**Copying a word's counters into `andSeq`** — when the first word of an andsequence is
found in the index, its counters must be _copied_, never aliased, so that intersecting or
zeroing `andSeq` cannot corrupt the index:

    iterate over the word's counters; for each (docID, count):
        counters_set andSeq[docID] = count

## Function prototypes

Detailed descriptions of each function's parameters and return values are provided as
paragraph comments above each function's declaration in `querier.c`, and are not repeated
here.

```c
int main(const int argc, char* argv[]);
static void parseArgs(const int argc, char* argv[],
                      char** pageDirectory, char** indexFilename);
static bool tokenize(char* line, char** words, int* numWords);
static bool validateQuery(char** words, const int numWords);
static counters_t* matchQuery(index_t* index, char** words, const int numWords);
static void rankAndPrint(counters_t* result, const char* pageDirectory);

static counters_t* intersectCounters(counters_t* andSeq, counters_t* word);
static void mergeCounters(counters_t* orResult, counters_t* andSeq);
static counters_t* copyCounters(counters_t* source);

static void prompt(void);
```

The iterate callbacks needed by the set operations and by ranking (for example, a
callback that takes the minimum, one that takes the sum, one that finds the maximum, and
one that counts non-zero entries) are also `static` functions in `querier.c`; their
signatures match the `counters_iterate` itemfunc contract.

## Error handling and recovery

All command-line arguments are validated by `parseArgs` before the index is loaded;
problems print a message to stderr and exit non-zero.

A query with a disallowed character, or with an invalid operator structure, is _not_ an
unrecoverable error: the querier prints a descriptive error message, abandons that query,
and continues reading the next line.
Blank lines and zero-word queries produce no output and are simply skipped.

Out-of-memory conditions are handled by variants of the `mem_assert` functions, which
print a message to stderr and exit non-zero; such failures are expected to be rare.

All functions check their pointer parameters for `NULL` and take a safe action (returning
`NULL`, returning `false`, or doing nothing) when given bad input.

Per the Requirements Spec, the querier may assume the index file and the page files
follow the documented formats, so it does not perform extensive validation when reading
them.

## Testing plan

### Unit testing

The individual functions are exercised through the system tests below; the set operations
(intersection, union) are verified indirectly by checking query scores against
hand-computed expected values on small indexes such as `letters`.

### Integration / system testing

We write a script `testing.sh`, invoked by `make test` (run as `bash -v testing.sh`),
that covers:

1. **Argument errors:** the querier is run with zero, one, and three arguments; with a
   `pageDirectory` that does not exist; with a `pageDirectory` lacking a `.crawler` file;
   and with an unreadable `indexFilename`. Each must print an error and exit non-zero.

2. **Query-syntax errors:** a file of queries is piped to the querier containing queries
   that begin or end with an operator, that place two operators adjacently, that contain
   non-letter characters, and that are blank. The querier must print the correct error
   message (or nothing, for blank lines) and continue.

3. **Matching and scoring:** queries are run against a known index (e.g. `letters` and
   `toscrape` from the shared output) covering single words, implicit and explicit `and`,
   `or` across andsequences, `and`-over-`or` precedence, a word absent from the index
   inside an andsequence, and queries that match no documents. Output is checked against
   hand-computed or known-good expected results.

4. **Ranking:** results are confirmed to print in decreasing score order, and
   `No documents match.` is confirmed to appear exactly when no document qualifies.

5. **Fuzz testing:** the provided `fuzzquery` program is used to generate many random
   queries from a known index; these are piped to the querier to confirm it never
   crashes or leaks on unexpected input. `fuzzquery.c` is committed to the repository
   since the testing script depends on it.

6. **Valgrind:** the querier is run under valgrind over a representative set of queries
   to confirm there are no memory leaks or errors when it exits normally.

The output of all tests is saved with `make test &> testing.out`.
