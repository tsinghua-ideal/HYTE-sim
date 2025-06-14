#include "parameters.h"

// smaple parameter p
double samplep = 0.01;
// sample parameter k
double samplek = 100;

int I, J, K;

int tti = 1, ttk = 1, ttj = 1;
// block size
int iii = 1000, jjj = 1000, kkk = 1000;

int BSRsize = 32;

InterOrder interorder;
DataFlow dataflow;
Format format;

string printInterOrder[10] = {"IJK", "IKJ", "JKI", "JIK", "KIJ", "KJI"};
string printDataFlow[10] = {"Inner", "Outer", "Gust"};
string printFormat[10] = {"RR", "RC", "CR", "CC", "BB"};

int Asize;
int Bsize;
int Csize;