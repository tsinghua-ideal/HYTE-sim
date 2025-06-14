#include "headers.h"
#include "statistics.h"
#include "util.h"

// int cachesize = 131072;
int cachesize = 262144;

// cacheline : 16,4
// 4,2
// 8,3
// 32,5
int CACHEBLOCK = 16;
int CACHEBLOCKLOG = 4;
// int CACHEBLOCK = 4;
// int CACHEBLOCKLOG = 2;
// int CACHEBLOCK = 8;
// int CACHEBLOCKLOG = 3;
// int CACHEBLOCK = 32;
// int CACHEBLOCKLOG = 5;

#define SETASSOC 4
#define SETASSOCLOG 2

#define MAXSET 1000005

int SET = cachesize / (CACHEBLOCK * SETASSOC);
int SETLOG = getlog(SET);

#define BIAS 23

bool Valid[MAXSET][SETASSOC];
int Tag[MAXSET][SETASSOC];
int lrubit[MAXSET][SETASSOC];

// split into 4 parts.  witin 16: 0000, 0001, 0010,,,,  1111
short partialValid[MAXSET][SETASSOC];

// use to record the lru.
// higher is better (accessed recently)
int cachecycle = 0;

// cache Scheme 1: the original version, each fiber is a new cacheline; allocate
// another cacheline if exceed. cache Scheme 0: alternative 1 (worse); don't
// allocate another cacheline if exceed -> may bad in both long or short
// scenarios cache Scheme 4: the condensed address version.  need to load a
// whole cacheline each time request a short fiber

// start addr: start of the fiber. fiberid or condensed dram address
// exceed part: the part exceed the cacheline. cut: only cache one cacheline.
// split: split to consective addrs not full part: when less then one cacheline.
// whole: load the whole cacheline anyway. partial: only load a part (need more
// hardware change ) to support partial: need a extra metadata to track whether
// a fiber is valid. (not very expensive. only one bit per each fiber)
//                  start addr     exceed part      not full part
// cache Scheme 0       fiber       cut                 whole
// cache Scheme 1       fiber       split               whole
int cacheScheme;

long long getCacheAddr(int fiberid, int relative) {
  long long ret;

  ret = (((long long)fiberid) << CACHEBLOCKLOG);
  if (relative) {
    ret += (((long long)relative) << (CACHEBLOCKLOG + BIAS));
  }

  return ret;
}

// mapping: tag | set index | offset within cacheblock
unsigned int getSet(long long addr) { return (addr >> (CACHEBLOCKLOG)) % SET; }

unsigned int getTag(long long addr) {

  return (addr >> (CACHEBLOCKLOG + SETLOG));
}

int getLRU(int _set, int _index) { return lrubit[_set][_index]; }

void updateLRU(int _set, int _index) {
  cachecycle++;
  lrubit[_set][_index] = cachecycle;
}

void initLRU(int _set, int _index) {
  // play the same as updateLRU in LRU policy
  cachecycle++;
  lrubit[_set][_index] = cachecycle;
}

bool cacheHit(long long addr) {
  int _set = getSet(addr);
  int _tag = getTag(addr);

  for (int i = 0; i < SETASSOC; i++) {
    if (Valid[_set][i] && (Tag[_set][i] == _tag)) {
      // hit !!

      // update lru bit
      updateLRU(_set, i);
      return 1;
    }
  }

  // miss !
  return 0;
}

void cacheReplace(long long addr) {

  int replaceindex = -1;
  // higher means need to keep
  // -1 means invalid now
  int replacelru = 10000000;

  int _set = getSet(addr);
  int _tag = getTag(addr);

  for (int i = 0; i < SETASSOC; i++) {
    if (!Valid[_set][i]) {
      // if has invalid slot, use it
      replacelru = -1;
      replaceindex = i;
      // don't need to find others anymore
      break;
    } else {
      int tmplru = getLRU(_set, i);

      // change to this slot
      if (tmplru < replacelru) {
        replacelru = tmplru;
        replaceindex = i;
      }
    }
  }

  Valid[_set][replaceindex] = 1;
  Tag[_set][replaceindex] = _tag;

  initLRU(_set, replaceindex);
}

void cacheRead(long long addr) {
  // cache hit!
  if (cacheHit(addr)) {
    // sram read
    computeSramAccess += sramReadBandwidth(CACHEBLOCK);

  }
  // cache miss
  else {
    // dram load
    computeDramAccess += memoryBandwidthPE(CACHEBLOCK);
    // sram write
    computeSramAccess += sramWriteBandwidth(CACHEBLOCK);

    computeB += memoryBandwidthPE(CACHEBLOCK);
    AccessByte += CACHEBLOCK;

    // update cache status
    cacheReplace(addr);
  }
}

void initializeCacheValid() { memset(Valid, 0, sizeof(Valid)); }

__attribute__((noinline)) void cacheAccessFiber(int jj, int fibersize) {

  // fiber + cut + whole
  // only cache the part within a cacheline (x-cache)
  if (cacheScheme == 0) {
    // if the whole size exceed the cacheline, then the rest part miss
    long long tmpaddr = getCacheAddr(jj, 0);

    cacheRead(tmpaddr);

    // the exceed part will miss anyway
    if (fibersize > CACHEBLOCK) {
      // dram load
      computeDramAccess += memoryBandwidthPE(fibersize - CACHEBLOCK);
      // sram write
      computeSramAccess += sramWriteBandwidth(fibersize - CACHEBLOCK);

      computeB += memoryBandwidthPE(fibersize - CACHEBLOCK);
      AccessByte += fibersize - CACHEBLOCK;
    }
  }

  // fiber + split + whole
  // split to multiple consective cachelines when exceed cacheline size
  if (cacheScheme == 1) {
    // for each BLOCK segment of the B fiber
    for (int tmpcurr = 0; tmpcurr < fibersize; tmpcurr += CACHEBLOCK) {

      // the address alters in different cache schemes
      long long tmpaddr = getCacheAddr(jj, tmpcurr / CACHEBLOCK);

      // the read granularity alters in different cache schemes
      cacheRead(tmpaddr);
    }
  }
}