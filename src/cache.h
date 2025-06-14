#ifndef CACHE_H
#define CACHE_H

extern int cachesize;

extern int cacheScheme;

void initializeCacheValid();

__attribute__((noinline)) void cacheAccessFiber(int jj, int fibersize);

#endif