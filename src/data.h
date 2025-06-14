#ifndef DATA_H
#define DATA_H

#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

static const int MAXN = 4000000;

// input matrix size and number of non-zero elements
extern int N, M, nzA, nzB;

// sparse matrices A, B and their transposes Ac, Bc
extern std::vector<int> A[MAXN], Ac[MAXN];
extern std::vector<int> B[MAXN], Bc[MAXN];

// store the offsets for A, Ac, B, Bc
extern int offsetarrayA[MAXN], offsetarrayAc[MAXN];
extern int offsetarrayB[MAXN], offsetarrayBc[MAXN];

// sample matrix
extern int SI, SK;
extern std::vector<int> SA[MAXN];
extern std::vector<double> SAc[MAXN];
extern std::vector<int> SBc[MAXN];
extern std::vector<double> SB[MAXN];
extern int SAindex[MAXN], SBcindex[MAXN];

// Read input matrices A and B from files
void readInputMatrices(const char *fileA, const char *fileB);

#endif // DATA_H
