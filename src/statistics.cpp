#include "headers.h"

long long totalCycle;
long long preCycle, calCycle, postCycle;
long long minCycle = 1LL << 60;

long long preDramAccess, preSramAccess;

long long computeDramAccess, computeSramAccess, computePE;
long long Abandwidth = 0, Bbandwidth = 0, Cbandwidth = 0, computebandwidth = 0;

long long postDramAccess, postSramAccess;

long long computeA, computeB, computeC;
long long totalSram, totalDram, totalPE;

// totalA/B/C correspond to all dram access (in the whole process)
long long totalA, totalB, totalC;
// preA, preB, postC correspond the the pre & post access
// (also there is computeA/B/C)
long long preA, preB, postC;

long long AccessByte = 0;

void analyze_statistics() {

  printf("total cycle = %lld\nload cycle = %lld, multiply cycle = %lld, "
         "merge and writeback cycle = %lld \n",
         totalCycle, preCycle, calCycle, postCycle);
  printf("total SRAM cycle = %lld, total DRAM cycle = %lld, total PE cycle = "
         "%lld\n",
         totalSram, totalDram, totalPE);
  printf("DRAM access A during multiply = %lld, "
         "DRAM access B during multiply = %lld, "
         "DRAM access C during multiply = %lld\n"
         "DRAM access A during load = %lld, "
         "DRAM access B during load = %lld, "
         "DRAM access C during merge and writeback = %lld\n",
         computeA / PEcnt, computeB / PEcnt, computeC / PEcnt, preA, preB,
         postC / PEcnt);
  totalA = computeA / PEcnt + preA;
  totalB = computeB / PEcnt + preB;
  totalC = computeC / PEcnt + postC / PEcnt;
  printf("total DRAM access A = %lld, total DRAM access B = %lld, "
         "total DRAM access C = %lld \n",
         totalA, totalB, totalC);
}