#include "data.h"
#include <fstream>
#include <sstream>
#include <string>

int N, M, nzA, nzB;

std::vector<int> A[MAXN], Ac[MAXN], B[MAXN], Bc[MAXN];

int offsetarrayA[MAXN], offsetarrayAc[MAXN];
int offsetarrayB[MAXN], offsetarrayBc[MAXN];

int SI, SK;
std::vector<int> SA[MAXN];
std::vector<double> SAc[MAXN];
std::vector<int> SBc[MAXN];
std::vector<double> SB[MAXN];
int SAindex[MAXN], SBcindex[MAXN];

extern double ha1, hb1;
extern double ha2, hb2;
