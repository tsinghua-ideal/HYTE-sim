#include "config.h"

double HBMbandwidth;
int PEcnt;
int mergecnt;
double HBMbandwidthperPE;
int sramBank;
int sramReadPort;
int sramWritePort;

// DRAM bandwidth
double memoryBandwidthWhole(long long ss) {
  return (ss + HBMbandwidth - 1) / HBMbandwidth;
}

// bandwidth per PE
double memoryBandwidthPE(long long ss) { return ss / HBMbandwidthperPE; }

/*
Return the single port cycle. (Otherwise will lead to fractional)
Just / banknumber at last

2 read port and 1 write port
*/
// SRAM read bandwidth
long long sramReadBandwidth(long long ss) { return ss / 2; }

long long sramWriteBandwidth(long long ss) { return ss; }

bool ISCACHE;