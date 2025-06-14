/*
Keep all hardware configurations.
*/

#ifndef CONFIG_H
#define CONFIG_H

// global bandwdith & SRAM configuration
extern double HBMbandwidth;
extern int PEcnt, mergecnt;
extern double HBMbandwidthperPE;
extern int sramBank, sramReadPort, sramWritePort;

// DRAM ↔ SRAM bandwidth calculations
double memoryBandwidthWhole(long long ss);
double memoryBandwidthPE(long long ss);

// SRAM cycle
long long sramReadBandwidth(long long ss);
long long sramWriteBandwidth(long long ss);

extern bool ISCACHE;

#endif