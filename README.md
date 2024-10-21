# fuzzy-native

Fuzzy string matching library package for Node. Implemented natively in C++ for speed with support for multithreading.

The scoring algorithm is heavily tuned for file paths, but should work for general strings. It also supports the same algorithm that was implemented in the `fuzzaldrin` library — the one that powers the command palette and other fuzzy-finders in Pulsar.

## API

```ts
export type MatcherOptions = {
  // Default: false
  caseSensitive?: boolean,

  // Default: infinite
  maxResults?: number,

  // Maximum gap to allow between consecutive letters in a match.
  // Provide a smaller `maxGap` to speed up query results.
  // Default: unlimited
  maxGap?: number;

  // Default: 1
  numThreads?: number,

  // Default: false
  recordMatchIndexes?: boolean,

  // can be either "fuzzaldrin" or anything else to use the file-based option
  algorithm?: string
}

export type MatchResult = {
  id: number,
  value: string,

  // A number in the range (0-1]. Higher scores are more relevant.
  // 0 denotes "no match" and will never be returned.
  score: number,

  // Matching character index in `value` for each character in `query`. This
  // can be costly, so this is only returned if `recordMatchIndexes` was set in
  // `options`.
  matchIndexes?: Array<number>,
}

export class Matcher {
  constructor(candidates: Array<string>) {}

  // Returns all matching candidates (subject to `options`).
  // Will be ordered by score, descending.
  match(query: string, options?: MatcherOptions): Array<MatchResult>;

  addCandidates(ids: Array<number>, candidates: Array<string>): void;
  removeCandidates(ids: Array<number>): void;
  setCandidates(ids: Array<number>, candidates: Array<string>): void;
}
```

See also the [spec](spec/fuzzy-native-spec.js) for basic usage.

## Scoring algorithm

### Default

The _default scoring_ algorithm is mostly borrowed from @wincent's excellent [command-t](https://github.com/wincent/command-t) vim plugin; most of the code is from [his implementation in  match.c](https://github.com/wincent/command-t/blob/master/ruby/command-t/match.c).

Read [the source code](src/score_match.cpp) for a quick overview of how it works (the function `recursive_match`).

Note that [score_match.cpp](src/score_match.cpp) and [score_match.h](src/score_match.h) have no dependencies besides the C/C++ stdlib and can easily be reused for other purposes.

There are a few notable optimizations:

- Before running the recursive matcher, we first do a backwards scan through the haystack to see if the needle exists at all. At the same time, we compute the right-most match for each character in the needle to prune the search space.
- For each candidate string, we pre-compute and store a bitmask of its letters in `MatcherBase`. We then compare this the "letter bitmask" of the query to quickly prune out non-matches.

### `fuzzaldrin`

Ported from Atom's [`fuzzaldrin` library](https://github.com/pulsar-edit/fuzzaldrin). It's easier to read the original version; see [scorer.coffee](https://github.com/atom/fuzzaldrin/blob/master/src/scorer.coffee) from Atom's archived repository.
