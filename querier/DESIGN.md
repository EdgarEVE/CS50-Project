# CS50 TSE Querier

## Design Spec

**Authors:** Brandon Wang, Ryan Sun, Jan Zuk, Edgar Fraire
**Date:** 05/22/2026

In this document we reference the [Requirements Specification](REQUIREMENTS.md) and focus on the implementation-independent design decisions.
Here we cover the core subset of design topics relevant to the Querier:

- User interface
- Inputs and outputs
- Functional decomposition into modules
- Pseudo code (plain English-like language) for logic/algorithmic flow
- Major data structures
- Testing plan

## User interface

The querier's interface with the user has two parts.

First, a command line; it must always be given exactly two arguments.

```bash
$ querier pageDirectory indexFilename
```

For example, to query the index built from the `letters` site at depth 6:

```bash
$ ./querier ../data/letters ../data/letters.index
```

Second, after the index is loaded, the querier reads search queries from stdin, one query per line, until EOF.
When stdin is a terminal (a tty), the querier prints a `Query?` prompt before each line; when stdin is not a terminal (e.g., a test file), no prompt is printed, so test output stays clean.

## Inputs and outputs

_Input:_ the querier takes two file-related inputs and one stream input.
The `indexFilename` argument names a file produced by the Indexer, which is loaded into memory.
The `pageDirectory` argument names a directory produced by the Crawler; its page files are read (only their first line, the URL) when results are printed.
Search queries are read from stdin, one per line.

_Output:_ for each valid, non-empty query the querier prints, to stdout, the _clean query_ (the query with normalized spacing and lower-case letters) followed by the matching documents.
If no documents match, it prints `No documents match.`
Otherwise it prints the matching documents in decreasing order of score; for each it prints the score, the document ID, and the URL.
For a query with invalid syntax, it prints an error message describing the problem and moves on to the next query.
Empty queries (blank lines) produce no output.
Unrecoverable errors produce a message on stderr and a non-zero exit status.

## Functional decomposition into modules

We anticipate the following functions within `querier.c`:

1. _main_, which parses arguments, loads the index, and runs the query loop;
2. _parseArgs_, which validates the command-line arguments;
3. _tokenize_, which validates the query characters and splits the query line into an array of normalized words;
4. _validateQuery_, which checks that the token array obeys the grammar's structural rules;
5. _matchQuery_, which walks the token array per the BNF grammar and produces a `counters` of _(docID, score)_ results;
6. _andSequence_ helper logic and _orResult_ helper logic, realized through two set operations: _intersect_ (for AND) and _merge_ (for OR);
7. _rankAndPrint_, which prints the result set in decreasing score order.

And some helper modules that provide data structures and shared services:

1. _index_, the module (from `common`) providing the in-memory index and `index_load` / `index_find`;
2. _counters_, the data structure (from `libcs50`) used to represent each _(docID, count)_ set;
3. _pagedir_, the module (from `common`) used to read a URL from a page file;
4. _word_, the module (from `common`) providing `normalizeWord`;
5. _file_, the module (from `libcs50`) used to read lines and count lines.

## Pseudo code for logic/algorithmic flow

### Overall flow

    parse and validate arguments (pageDirectory is a Crawler directory; indexFilename is readable)
    load the index from indexFilename into an index_t
    loop reading lines from stdin until EOF:
        print the prompt if stdin is a tty
        if the line is blank (empty or only spaces), skip it
        tokenize the line: scan for disallowed characters, split on spaces, lower-case each word
        if tokenizing fails (a bad character was found), print an error and continue
        if the token array is empty, print nothing and continue
        validate the token array's structure
        if invalid, print the error and continue
        print the clean query
        call matchQuery to obtain a counters of (docID, score)
        call rankAndPrint on that counters
        free the counters and the token array
    delete the index and exit zero

### matchQuery

`matchQuery` walks the token array following the grammar
`query ::= <andsequence> [or <andsequence>]...` and
`andsequence ::= <word> [[and] <word>]...`.

It maintains two `counters` objects: `orResult`, the running union across completed
andsequences, and `andSeq`, the running intersection within the current andsequence.
To distinguish a _brand-new_ andsequence from one that has been _zeroed by a missing word_,
we use a boolean flag `firstWord` that is true only before the first word of the current
andsequence has been processed.

    create an empty counters orResult
    create an empty counters andSeq
    set firstWord = true
    for each token in the array:
        if the token is 'or':
            merge andSeq into orResult (union)
            reset andSeq to a new empty counters
            set firstWord = true
        else if the token is 'and':
            do nothing (the 'and' is structural; it is handled implicitly)
        else (the token is a word):
            look up the word's counters in the index (index_find)
            if firstWord is true:
                if the word is in the index, copy its counters into andSeq
                (otherwise andSeq stays empty)
                set firstWord = false
            else:
                intersect andSeq with the word's counters (take the min per docID);
                if the word is not in the index, this intersects with the empty set,
                correctly leaving andSeq empty
    merge the final andSeq into orResult (union)
    return orResult

Because intersection with a missing word zeroes the whole andsequence, and `firstWord`
is only reset at the start of a _new_ andsequence (after an `or`), a missing word can
never accidentally resurrect a dead andsequence.

### rankAndPrint

`rankAndPrint` prints the result counters in decreasing score order, using a
selection-sort-style repeated scan (the `counters` module offers no sorted iteration).

    let printedAny = false
    repeat:
        iterate over the counters to find the (docID, score) pair with the highest score
        if the highest score is 0, stop
        read the URL from the first line of pageDirectory/docID
        print the score, the docID, and the URL
        set that docID's counter to 0 (so it is not chosen again)
        set printedAny = true
    if printedAny is false, print "No documents match."

## Major data structures

The central data structure is the _index_, an in-memory inverted index mapping each
_word_ to a _counters_ object of _(docID, count)_ pairs.
The index is implemented (in the `common/index` module) as a _hashtable_ keyed by word
whose items are _counters_; it is loaded from the index file, and is sized from the
number of lines in that file, since the index file holds exactly one word per line.

During query processing we use two additional _counters_ objects: the
**and-sequence accumulator** (`andSeq`), holding the running intersection for the current
andsequence, and the **or-result accumulator** (`orResult`), holding the running union
across all andsequences processed so far.
These are fresh `counters` objects, separate from those owned by the index; copying a
word's counters into `andSeq` makes an independent copy so that intersecting or zeroing
the accumulator never corrupts the index.

For tokenizing, we use an _array of word pointers_ into the (normalized, in-place) query
line.
For ranking, we use a small helper struct holding the current best _(docID, score)_ pair
while scanning for the maximum.

### Set operations

The two set operations both use `counters_iterate`, `counters_get`, and `counters_set`,
so they require no change to the `counters` module.

- **Intersection (AND):** iterate over the _accumulator_ (`andSeq`); for each _docID_,
  look up that docID's count in the _word's_ counters and set the accumulator's count to
  `min(accumulatorCount, wordCount)`.
  If the word's counters does not contain the docID (or the word is absent from the index
  entirely), the looked-up count is 0, so the accumulator's count becomes 0.

- **Union (OR):** iterate over the _source_ (`andSeq`); for each _docID_, add its count to
  the corresponding count in the _destination_ (`orResult`), creating the docID in the
  destination if it is not already present.

## Testing plan

- **Arguments:** run the querier with 0, 1, and 3+ arguments; with a `pageDirectory` that
  does not exist; with a `pageDirectory` that is not a Crawler directory; and with an
  `indexFilename` that cannot be read. Each should produce an error on stderr and a
  non-zero exit status.

- **Query syntax:** feed queries that start or end with `and`/`or`; that place two
  operators adjacently; that contain non-letter, non-space characters; and blank lines.
  Confirm each produces the correct error message (or, for blank lines, no output) and
  that the querier continues to the next query.

- **Matching semantics:** test single-word queries; implicit and explicit `and`; `or`
  across andsequences; a word absent from the index inside an andsequence (the whole
  andsequence must yield no matches); and queries that match no documents.

- **Ranking:** confirm results print in decreasing score order, and that
  `No documents match.` is printed exactly when no documents qualify.

- **Comparison against known-good output:** run the querier on the shared crawler/indexer
  output and, where possible, compare against expected results; use the provided
  `fuzzquery` program to fire many random queries and confirm the querier never crashes.

- **Valgrind:** run the querier under valgrind over a variety of queries and confirm there
  are no memory leaks or memory errors when it exits normally.
