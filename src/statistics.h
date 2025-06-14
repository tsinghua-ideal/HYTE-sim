#ifndef STATISTICS_H
#define STATISTICS_H

extern long long totalCycle;
extern long long preCycle, calCycle, postCycle;
extern long long minCycle;

extern long long preDramAccess, preSramAccess;

extern long long computeDramAccess, computeSramAccess, computePE;
extern long long Abandwidth, Bbandwidth, Cbandwidth, computebandwidth;

extern long long postDramAccess, postSramAccess;

extern long long computeA, computeB, computeC;
extern long long totalSram, totalDram, totalPE;

// totalA/B/C correspond to all dram access (in the whole process)
extern long long totalA, totalB, totalC;
// preA, preB, postC correspond the the pre & post access
// (also there is computeA/B/C)
extern long long preA, preB, postC;

extern long long AccessByte;

void analyze_statistics();

#endif