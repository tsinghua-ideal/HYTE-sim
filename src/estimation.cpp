#include "headers.h"
#include "util.h"

int SIcnt, SKcnt, SAnnz, SBnnz;

const int pmod = 1000000007; // A large prime number

double ha1, hb1;
double ha2, hb2;

void initsample() {
  ha1 = getRandomCoefficient(), hb1 = getRandomCoefficient();
  ha2 = getRandomCoefficient(), hb2 = getRandomCoefficient();
}

void sampleA() {
  // work on the sample
  for (int i = 0; i <= I; i++) {
    // Be the sampled row in probability p
    if (sampleP()) {
      for (int j = 0; j < A[i].size(); j++) {
        SA[SIcnt].push_back(A[i][j]);
        // directly push back the h1(x)!
        SAc[A[i][j]].push_back(hash1(i, ha1, hb1, pmod));
        SAnnz++;
      }
      SAindex[SIcnt] = i;
      SIcnt++;
    }
  }
}

void sampleB() {
  // work on the sample
  for (int k = 0; k <= K; k++) {
    // Be the sampled row in probability p
    if (sampleP()) {
      for (int j = 0; j < Bc[k].size(); j++) {
        SBc[SKcnt].push_back(Bc[k][j]);
        SB[Bc[k][j]].push_back(hash2(k, ha2, hb2, pmod));
        SBnnz++;
      }
      SBcindex[SKcnt] = k;
      SKcnt++;
    }
  }
}

struct pair_hash {
  template <typename T1, typename T2>
  std::size_t operator()(const std::pair<T1, T2> &p) const {
    auto h1 = std::hash<T1>{}(p.first);
    auto h2 = std::hash<T2>{}(p.second);
    return h1 ^ (h2 << 1);
  }
};

unordered_set<double> hashset;
priority_queue<double> hashqu;

unordered_set<pair<double, int>, pair_hash> hashsetr;
priority_queue<pair<double, int>> hashqur;

unordered_set<double> Fset;
unordered_set<pair<double, int>, pair_hash> Fsetr;

unordered_set<double> hashsetTJ[256];
priority_queue<double> hashquTJ[256];

// this is get through the sample K
// maxh means the sampled kth largest answer
// this is further used in the compute of estnnzCTK
// to only track the numbers among the maxh and to predict the size
double maxh;

int OutputK(int lastk, int nowk) {

  for (int tmpk = lastk; tmpk < nowk; tmpk++) {
    if (!hashqu.empty()) {
      // printf("@@@ %d %lf\n", tmpk, hashqu.top());
      hashqu.pop();
      //  printf("&& %d %d\n", hashqu.size(), hashqu.empty());
      if (hashqu.empty())
        break;
    } else {
      break;
    }
  }

  fflush(stdout);
  // if(!hashqu.empty()){
  double tmph = -hashqu.top();
  double ans = nowk / tmph;
  maxh = tmph;
  // printf("!!! %d %lf   %lf\n", nowk, ans, tmph);
  return ((int)ans);
  //}
}

int cTJ;
int csizeJ;

double tkans[128];

int OutputKTJ(int lastk, int nowk, double maxh) {

  int ret = 0;

  double ans = 0;

  for (int tmptj = 0; tmptj < cTJ; tmptj++) {

    bool flag = 1;
    int cntk;

    for (int tmpk = lastk; tmpk < nowk; tmpk++) {
      if (!hashquTJ[tmptj].empty()) {
        tkans[tmptj] = tmpk / (-hashquTJ[tmptj].top());
        hashquTJ[tmptj].pop();
      } else {
        flag = 0;
        cntk = tmpk;
        // printf("%d %d\n", tmptj, tmpk);
        // fflush(stdout);
        break;
      }
    }

    if (flag == 1) {
      ans += nowk / maxh;
      // printf("111 nowk:%d  ans:%lf\n", nowk, ans);
    } else {
      // ans += tkans[tmptj];
      // don‘t have tmpk nnz
      ans += cntk / maxh;
      // printf("222 nowk:%d  ans:%lf  cntk:%d  maxh:%lf\n", nowk, ans, cntk,
      // maxh);
    }
  }
  // printf("!!! nowk:%d  ans:%lf\n", nowk, ans);
  fflush(stdout);
  return ((int)ans);
}

double combineSF() {

  if (hashqu.empty()) {
    for (const auto &element : Fset) {
      hashqu.push(element);
      hashset.insert(element);
    }
    for (const auto &element : Fsetr) {
      hashqur.push(element);
      hashsetr.insert(element);
    }
    return hashqu.top();
  }

  // combine Fset into hashset and hashqu
  double topnow = hashqu.top();

  for (const auto &element : Fset) {
    if (element >= topnow) {
      // not small enough!
      continue;
    }

    if (hashset.find(element) == hashset.end()) {
      // not in the set now ! update the set and queue
      double tmppop = hashqu.top();
      hashqu.pop();
      if (topnow != 1.0) {
        hashset.erase(topnow);
      }

      while (hashqur.top().first == tmppop) {
        pair<double, int> tmppopr = hashqur.top();
        hashqur.pop();
        hashsetr.erase(tmppopr);
      }

      hashqu.push(element);
      hashset.insert(element);

      // update the top
      topnow = hashqu.top();
    } else {
      // already in the set, continue
      continue;
    }
  }

  for (const auto &element : Fsetr) {
    if (element.first > topnow) {
      continue;
    }

    hashqur.push(element);
    hashsetr.insert(element);
  }

  return topnow;
}

long long estEffMAC, estnnzC, nnzCTk[33];

int timeEstMAC, timeEstnnzC, timeEstnnzCTK;

vector<double> vectorTK[8195];

// use the previously calculated hashsetr  and hashqur to calculate this
// hashsetr contains all sampled pairs < maxh
void newestnnzCTK() {

  int maxkkk = 4096;
  int maxTK = (J + 4095) / 4096;

  double prevf = -1;
  int prevtk = -1;

  while (!hashqur.empty()) {
    pair<double, int> tmppopr = hashqur.top();
    hashqur.pop();

    // must be a number < 4096
    int tkthis = tmppopr.second / maxTK;

    // if didn't occur previously, push into the vector
    if ((tmppopr.first != prevf) || (tkthis != prevtk)) {
      // store negative numbers for ascending order
      // (default priority queue is descending order)
      vectorTK[tkthis + 4096].push_back(-tmppopr.first);
    }

    prevf = tmppopr.first;
    prevtk = tkthis;
  }

  for (int i = 4096; i < 4096 * 2; i++) {
    nnzCTk[12] += vectorTK[i].size();
  }

  for (int k = 11; k >= 0; k--) {
    maxkkk /= 2;

    // for the else, merge from i<<1 and i<<1+1
    for (int i = maxkkk; i < maxkkk * 2; i++) {
      vectorTK[i] = mergevector(vectorTK[i * 2], vectorTK[i * 2 + 1]);
      nnzCTk[k] += vectorTK[i].size();
    }

    //   printf("nnzctk: k:%d %d\n", k, nnzCTk[k]);
  }
}

inline int Addmod(int a1, int a2, int mod) {
  int ret = a1 + a2;
  if (ret >= mod)
    ret -= mod;
  if (ret < 0)
    ret += mod;
  return ret;
}

void getParameterSample() {

  ///////////////////// get estEffMAC /////////////ƒ//////////
  estEffMAC = 0;

  auto time0 = std::chrono::high_resolution_clock::now();

  for (int j = 0; j <= J; j++) {

    int tmpsizea = SAc[j].size();
    int tmpsizeb = SB[j].size();

    if (tmpsizea) {
      sort(SAc[j].begin(), SAc[j].end());
    }
    if (tmpsizeb) {
      sort(SB[j].begin(), SB[j].end());
    }

    if (tmpsizea && tmpsizeb) {
      estEffMAC += tmpsizea * tmpsizeb;
    }
  }

  estEffMAC *= (1.0 / (samplep * samplep));

  printf("estEffMAC: %lld\n", estEffMAC);
  fflush(stdout);

  ///////////////////////// get estnnzC ///////////////////////

  auto time1 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::micro> elapsed = time1 - time0;

  timeEstMAC = (int)(elapsed.count());

  printf("time of sampling and estimating estEffMAC: %d\n", timeEstMAC);

  hashset.clear();
  Fset.clear();
  while (!hashqu.empty())
    hashqu.pop();
  hashsetr.clear();
  Fsetr.clear();
  while (!hashqur.empty())
    hashqur.pop();

  int Fsetsize = 0;

  double pnow = 1.0;

  for (int j = 0; j < J; j++) {

    int tmpsizea = SAc[j].size();
    int tmpsizeb = SB[j].size();

    if (tmpsizea && tmpsizeb) {
      int sbar = 0;

      for (int t = 0; t < tmpsizeb; t++) {
        // first find sbar s.t. h(sbar)
        while (h(SAc[j][sbar], SB[j][t]) >
               h(SAc[j][Addmod(sbar, -1, tmpsizea)], SB[j][t])) {
          sbar = Addmod(sbar, 1, tmpsizea);
        }
        int stmp = sbar;
        while (h(SAc[j][stmp], SB[j][t]) < pnow) {

          double tmph = h(SAc[j][stmp], SB[j][t]);

          // insert to Fsetr anyway (also insert the redundant)
          Fsetr.insert(make_pair(tmph, j));

          // only insert the element not occured.
          if (Fset.find(tmph) == Fset.end()) {
            Fset.insert(tmph);
            Fsetsize++;

            // flush the elements in F to S
            if (Fsetsize == samplek) {
              pnow = combineSF();
              Fset.clear();
              Fsetr.clear();
              Fsetsize = 0;
            }
          }
          stmp = Addmod(stmp, 1, tmpsizea);

          // break if come back to the first min place
          if (stmp == sbar) {
            break;
          }
        }
      }
    }
  }

  double ansp = combineSF();
  // printf("SF!!  %lf %lf\n", ansp, samplek/ansp);

  auto time20 = std::chrono::high_resolution_clock::now();

  elapsed = time20 - time1;

  timeEstnnzC = (int)(elapsed.count());

  printf("time of estimating nnzC: %d\n", timeEstnnzC);

  fflush(stdout);

  auto timetk0 = std::chrono::high_resolution_clock::now();

  newestnnzCTK();

  auto timetk1 = std::chrono::high_resolution_clock::now();

  elapsed = timetk1 - timetk0;

  printf("time of estimating nnzCtk: %d\n", (int)(elapsed.count()));

  fflush(stdout);

  hashset.clear();
  Fset.clear();
  while (!hashqu.empty())
    hashqu.pop();
  hashsetr.clear();
  Fsetr.clear();
  while (!hashqur.empty())
    hashqur.pop();

  int tmpnnzc = 0;
  for (int i = 0; i < SIcnt; i++) {
    for (int k = 0; k < SKcnt; k++) {

      bool tmpflag = 0;

      int tmpj = 0, tmpsza = SA[i].size();
      int tmpk = 0, tmpszb = SBc[k].size();
      for (tmpj = 0; (tmpj < tmpsza); tmpj++) {
        int nowj = SA[i][tmpj];
        while (tmpk < tmpszb && SBc[k][tmpk] < nowj) {
          tmpk++;
        }
        if (tmpk < tmpszb && SBc[k][tmpk] == nowj) {
          tmpflag = 1;
          double tmph = h(i, k, ha1, hb1, ha2, hb2, 1.0);
          // only insert when not in the set
          if (hashset.find(tmph) == hashset.end()) {
            hashset.insert(tmph);
            tmpflag = 1;
            tmpnnzc++;
            break;
          }
          // hashqu.push();
        }
      }
      if (tmpflag) {
        // samplennzC ++;
      }
    }
  }

  // printf("tmpnnzc: %d\n", tmpnnzc);

  // first convert the set to queue
  for (const auto &element : hashset) {
    hashqu.push(-element);
  }

  // printf("&&&&&&&&&&&&&&&&&&&&&&&&  %d\n", hashqu.size());

  int hashsize = hashqu.size();
  if (hashsize < samplek) {
    samplek = hashsize;
  }

  estnnzC = OutputK(0, samplek);
  fflush(stdout);

  auto time21 = std::chrono::high_resolution_clock::now();

  elapsed = time21 - time20;

  timeEstnnzC = (int)(elapsed.count());

  // printf("timeEstnnzC:%d\n", timeEstnnzC);

  ///////////////////////// get nnzCTK ///////////////////////

  // caculating nnzCTK in the ratio version
  // calculate TK=128 and scacle other TKs
  // use TJ in the code to represent TK

  cTJ = 128;
  csizeJ = J / cTJ;

  for (int i = 0; i < SIcnt; i++) {
    for (int k = 0; k < SKcnt; k++) {

      // bool tmpflag = 0;

      int tmpj = 0, tmpsza = SA[i].size();
      int tmpk = 0, tmpszb = SBc[k].size();
      for (tmpj = 0; (tmpj < tmpsza); tmpj++) {
        int nowj = SA[i][tmpj];

        // calculate which TJ this nowj is in
        int nowTJ = nowj / csizeJ;

        while (tmpk < tmpszb && SBc[k][tmpk] < nowj) {
          tmpk++;
        }
        if (tmpk < tmpszb && SBc[k][tmpk] == nowj) {
          // tmpflag = 1;
          double tmph = h(i, k, ha1, hb1, ha2, hb2, 1.0);

          // only insert when tmph <= maxh (to ensure the complexity)
          if (tmph <= maxh) {

            // only insert when not in the set
            if (hashsetTJ[nowTJ].find(tmph) == hashsetTJ[nowTJ].end()) {
              hashsetTJ[nowTJ].insert(tmph);
              // tmpflag = 1;
            }
          }
        }
      }
    }
  }

  for (int tmptj = 0; tmptj < cTJ; tmptj++) {
    for (const auto &element : hashsetTJ[tmptj]) {
      hashquTJ[tmptj].push(-element);
    }
  }

  nnzCTk[7] = OutputKTJ(0, samplek, maxh);
  nnzCTk[0] = estnnzC;

  auto time3 = std::chrono::high_resolution_clock::now();

  elapsed = time3 - time21;

  timeEstnnzCTK = (int)(elapsed.count());

  // printf("timeEstnnzCTK:%d\n", timeEstnnzCTK);

  // compute the rest nnzCTk through 0 and 7

  double nnzCTkratio = pow(((double)nnzCTk[7]) / ((double)estnnzC), 1.0 / 7.0);

  estnnzC *= (1.0 / (samplep * samplep));
  nnzCTk[0] *= (1.0 / (samplep * samplep));

  for (int i = 1; i <= 20; i++) {
    nnzCTk[i] = (long long)(nnzCTk[i - 1] * nnzCTkratio);
    //  printf("%d %lld\n", i, nnzCTk[i]);
  }

  printf("nnzC = %lld, nnzCTk[7] = %lld\n", nnzCTk[0], nnzCTk[7]);
}

map<int, bool> estC[MAXN];

int startA[MAXN];
int endA[MAXN];
int endB[MAXN];

void getParameter() {

  // get parameters in force

  // calculate estEffMAC and nnzC
  estEffMAC = 0;
  estnnzC = 0;

  for (int i = 0; i < I; i++)
    endA[i] = A[i].size();

  for (int j = 0; j < J; j++)
    endB[j] = B[j].size();

  for (int i = 0; i < I; i++) {
    int lenA = endA[i];
    for (int j = 0; j < lenA; j++) {
      int tmpx = A[i][j];

      int lenB = endB[tmpx];
      estEffMAC += lenB;

      for (int k = 0; k < lenB; k++) {

        int tmpk = B[tmpx][k];
        if (!estC[i][tmpk]) {
          estC[i][tmpk] = 1;
          estnnzC++;
        }
      }
    }
  }

  printf("MAC: %lld   nnzC: %lld\n", estEffMAC, estnnzC);
  nnzCTk[0] = estnnzC;

  // calculate nnzCTk

  // force version. compute the nnzCTk for Tiling 2/4/6/8,,,512
  int Ttcnt = 1, Tsize = J;
  for (int jj = 1; jj <= 10; jj++) {
    Ttcnt *= 2;
    Tsize /= 2;

    for (int j = 0; j < J; j++) {
      startA[j] = 0;
      estC[j].clear();
    }

    int boundnow = Tsize;
    // calculate Tcnt blocks
    for (int tcnt = 0; tcnt < Ttcnt; tcnt++) {

      for (int i = 0; i < I; i++) {
        for (int j = startA[i];; j++) {
          if (j >= endA[i]) {
            startA[i] = j;
            break;
          }
          int tmpx = A[i][j];
          if (tmpx >= boundnow) {
            startA[i] = j;
            break;
          }
          for (int k = 0; k < endB[tmpx]; k++) {
            int tmpk = B[tmpx][k];
            if (!estC[i][tmpk]) {
              estC[i][tmpk] = 1;
              nnzCTk[jj]++;
            }
          }
        }
      }

      for (int j = 0; j < J; j++) {
        estC[j].clear();
      }
      boundnow += Tsize;
    }

    printf("!!!  TK: %d  nnzCTK: %lld\n", jj, nnzCTk[jj]);
    fflush(stdout);
  }
}

double getvarianceBJ(int ttj) { return 1; }

double getvarianceBK(int ttk) { return 1; }

long long getnnzC(int jj) { return nnzCTk[getlog(jj)]; }

long long estSRAM, estDRAM, estPE, esttotal;
long long estpreSram, estpreDram;
long long estpostSram, estpostDram;
long long estcomputeSram, estcomputeDram;
long long esttotalA, esttotalB, esttotalC;
long long estcomputeA, estcomputeB, estcomputeC;
long long estpreA, estpreB, estpostC;

long long estmin = 1LL << 60;
long long estsquaremin = 1LL << 60;

int estiii, estjjj, estkkk;
int esttti, estttj, estttk;

long long gustest(int estsum) {

  estSRAM = estDRAM = estPE = esttotal = 0;
  estpreSram = estpreDram = 0;
  estpostSram = estpostDram = 0;
  estcomputeSram = estcomputeDram = 0;
  esttotalA = esttotalB = esttotalC = 0;
  estpreA = estpreB = estpostC = 0;
  estcomputeA = estcomputeB = estcomputeC = 0;

  ////////////////////////// version1: estimate without nnzCTk

  /////// pre load stage //////////////////////////////////

  long long avgTileSize = min((nzB * 3) / (ttj * ttk) + (2 * jjj), Bsize);

  if (!ISCACHE) {
    // pre load B

    // If the inter-tile change is I this time, then B can be reused and don't
    // need to reload again. If the inter-tile change is J or K, then need to
    // reload min(this tile size, Bsize) at each round. i.e., when interorder ==
    // **I, load TJ*TK*(avg tile size) else, load TI*TJ*TK*(avg tile size) (will
    // have TI times of redundant)

    // have inter-reuse B. need less load
    if ((interorder == JKI) || (interorder == KJI)) {
      estpreB += (long long)ttj * ttk * memoryBandwidthWhole(avgTileSize);
      estpreDram += (long long)ttj * ttk * memoryBandwidthWhole(avgTileSize);
      estpreSram += (long long)ttj * ttk * sramWriteBandwidth(avgTileSize);
    } // don't have inter-reuse B
    else {
      estpreB += (long long)(tti)*ttj * ttk * memoryBandwidthWhole(avgTileSize);
      estpreDram +=
          (long long)(tti)*ttj * ttk * memoryBandwidthWhole(avgTileSize);
      estpreSram +=
          (long long)(tti)*ttj * ttk * sramWriteBandwidth(avgTileSize);
    }
  }

  estpreSram /= sramBank;
  esttotal += max(estpreSram, estpreDram);

  ////// calculate stage////////////////////////////////////

  estPE = estEffMAC;

  // B access
  // sram access for B: all
  estcomputeSram +=
      (long long)tti * sramReadBandwidth(estEffMAC * 3LL + 2LL * J * ttk);

  long long cacheTileSize;
  double esthitrateB;

  if (!estsum) {
    cacheTileSize = min((nzB * 3) / (ttj * ttk) + (jjj), Bsize);

    // this is the upperbound of hitrate. hitrate will be lower when tiles are
    // not uniform.
    esthitrateB =
        min((double)cacheTileSize / ((nzB * 3) / (ttj * ttk) + (jjj)), 1.0);

  } else {
    esthitrateB = ((double)estsum) / ((nzB * 3) / (ttj * ttk) + jjj);
    esthitrateB = min(esthitrateB, 1.0);

    // printf("%d %lf\n", estsum, esthitrateB);
  }
  estcomputeDram +=
      (long long)tti *
      memoryBandwidthPE((estEffMAC * 3LL + 2LL * J * ttk) * (1 - esthitrateB));
  estcomputeB +=
      (long long)tti *
      memoryBandwidthPE((estEffMAC * 3LL + 2LL * J * ttk) * (1 - esthitrateB));

  double esthitrateC = 0;

  int usennzTK = 1;
  if (!usennzTK) {
    estcomputeSram +=
        (long long)(ttj)*sramWriteBandwidth(estnnzC * 3LL + 2LL * I * (ttk));

    estcomputeDram += (long long)(ttj)*memoryBandwidthPE(
        (estnnzC * 3LL + 2LL * I * (ttk)) * (1 - esthitrateC));
    estcomputeC += (long long)(ttj)*memoryBandwidthPE(
        (estnnzC * 3LL + 2LL * I * (ttk)) * (1 - esthitrateC));

    if (jjj != J) {
      estpostC += (long long)(ttj)*memoryBandwidthPE(
          (estnnzC * 3LL + 2LL * I * (ttk)) * (1 - esthitrateC));
      estpostDram += (long long)(ttj)*memoryBandwidthPE(
          (estnnzC * 3LL + 2LL * I * (ttk)) * (1 - esthitrateC));
    }
  } else {

    estcomputeSram += sramWriteBandwidth(getnnzC(ttj) * 3LL + 2LL * I * (ttk));
    estcomputeDram += memoryBandwidthPE((getnnzC(ttj) * 3LL + 2LL * I * (ttk)) *
                                        (1 - esthitrateC));
    estcomputeC += memoryBandwidthPE(((getnnzC(ttj) * 3LL + 2LL * I * (ttk))) *
                                     (1 - esthitrateC));

    if (jjj != J) {

      estpostC += memoryBandwidthPE(((getnnzC(ttj) * 3LL + 2LL * I * (ttk))) *
                                    (1 - esthitrateC));
      estpostDram += memoryBandwidthPE(
          ((getnnzC(ttj) * 3LL + 2LL * I * (ttk))) * (1 - esthitrateC));
    }
  }

  // A access
  estcomputeDram +=
      (long long)(ttk)*memoryBandwidthPE((nzA * 3LL + 2LL * I * (ttj)));
  estcomputeSram +=
      (long long)(ttk)*memoryBandwidthPE((nzA * 3LL + 2LL * I * (ttj)));
  estcomputeA +=
      (long long)(ttk)*memoryBandwidthPE((nzA * 3LL + 2LL * I * (ttj)));

  esttotal += max(estPE / PEcnt,
                  max(estcomputeDram / PEcnt, estcomputeSram / sramBank));

  //////// post stage/////////////////////////////////////
  // The inter-tile C merge
  if (dataflow == Gust) {
    if (jjj != J) {
      estpostC += memoryBandwidthPE(estnnzC * 3LL + 2LL * I);
      estpostDram += memoryBandwidthPE(estnnzC * 3LL + 2LL * I);
    }
  }

  esttotal += max(estpostDram / PEcnt, estpostSram / sramBank);

  int estsquareiii, estsquarejjj, estsquarekkk;
  int estsquaretti, estsquarettj, estsquarettk;

  if (jjj == kkk) {
    if (esttotal < estsquaremin) {
      estsquaremin = esttotal;
      estsquareiii = iii;
      estsquarejjj = jjj;
      estsquarekkk = kkk;
      estsquaretti = tti;
      estsquarettj = ttj;
      estsquarettk = ttk;
    }
  }

  if (esttotal < estmin) {

    estmin = esttotal;

    estiii = iii;
    estjjj = jjj;
    estkkk = kkk;
    esttti = tti;
    estttj = ttj;
    estttk = ttk;
  }

  return esttotal;
}

long long tilecnt[512][512];

void postEstAdjust() {
  // the prior-stage est:
  // estiii, estjjj, estkkk
  // esttti, estttj, estttk
  // now we are going to fine-tune it
  // (pre-stage finetune; run-time finetune(dynamic))

  // first: get the accurate hitrate of the current tiling
  // second: get the accurate hitrate of the finer tiling on J and K
  // if a road is better, than continue; else stop finer

  printf("\n\npostAdjust: iii jjj kkk: %d %d %d\n", estiii, estjjj, estkkk);
  fflush(stdout);

  memset(tilecnt, 0, sizeof(tilecnt));

  int iii2 = (estiii + 1) / 2, jjj2 = (estjjj + 1) / 2, kkk2 = (estkkk + 1) / 2;
  int tti2 = esttti * 2, ttj2 = estttj * 2, ttk2 = estttk * 2;

  for (int k = 0; k < SKcnt; k++) {
    int tmpszb = SBc[k].size();
    for (int j = 0; j < tmpszb; j++) {
      int tmpj = SBc[k][j];
      int tmpk = SBcindex[k];

      // to calculate how many nnzs in each tile
      tilecnt[tmpj / jjj2][tmpk / kkk2]++;
    }
  }

  // calculate 4 versions

  // J+K type0
  int outtile = 0;
  long long estsum = 0;
  for (int tj = 0; tj < ttj; tj++) {
    for (int tk = 0; tk < ttk; tk++) {
      long long sizejk =
          ((tilecnt[tj * 2][tk * 2] + tilecnt[tj * 2][tk * 2 + 1] +
            tilecnt[tj * 2 + 1][tk * 2] + tilecnt[tj * 2 + 1][tk * 2 + 1]) /
           samplep) *
              3LL +
          jjj;

      if (sizejk < Bsize) {
        estsum += sizejk;
      } else {
        estsum += Bsize;
      }
    }
  }
  iii = estiii;
  jjj = estjjj;
  kkk = estkkk;
  tti = esttti;
  ttj = estttj;
  ttk = estttk;

  long long mintime = gustest(estsum / (ttj * ttk));
  int mintype = 0;

  // J/2+K type1

  outtile = 0;
  estsum = 0;
  for (int tj = 0; tj < ttj2; tj++) {
    for (int tk = 0; tk < ttk; tk++) {
      long long sizejk =
          ((tilecnt[tj][tk * 2] + tilecnt[tj][tk * 2 + 1]) / samplep) * 3LL +
          jjj2;

      if (sizejk < Bsize) {
        estsum += sizejk;
      } else {
        estsum += Bsize;
      }
    }
  }
  iii = estiii;
  jjj = jjj2;
  kkk = estkkk;
  tti = esttti;
  ttj = ttj2;
  ttk = estttk;

  long long tmptime = gustest(estsum / (ttj2 * ttk));
  if (tmptime < mintime) {
    mintime = tmptime;
    mintype = 1;
  }

  // J+K/2 type2

  outtile = 0;
  estsum = 0;
  for (int tj = 0; tj < ttj; tj++) {
    for (int tk = 0; tk < ttk2; tk++) {
      long long sizejk =
          ((tilecnt[tj * 2][tk] + tilecnt[tj * 2 + 1][tk]) / samplep) * 3LL +
          jjj;

      if (sizejk < Bsize) {
        estsum += sizejk;
      } else {
        estsum += Bsize;
      }
    }
  }
  iii = estiii;
  jjj = estjjj;
  kkk = kkk2;
  tti = esttti;
  ttj = estttj;
  ttk = ttk2;

  tmptime = gustest(estsum / (ttj * ttk2));
  if (tmptime < mintime) {
    mintime = tmptime;
    mintype = 2;
  }

  // J/2+K/2  type3

  outtile = 0;
  estsum = 0;
  for (int tj = 0; tj < ttj2; tj++) {
    for (int tk = 0; tk < ttk2; tk++) {
      long long sizejk = (tilecnt[tj][tk] / samplep) * 3LL + jjj2;

      if (sizejk < Bsize) {
        estsum += sizejk;
      } else {
        estsum += Bsize;
      }
    }
  }
  iii = estiii;
  jjj = jjj2;
  kkk = kkk2;
  tti = esttti;
  ttj = ttj2;
  ttk = ttk2;

  tmptime = gustest(estsum / (ttj2 * ttk2));
  if (tmptime < mintime) {
    mintime = tmptime;
    mintype = 3;
  }

  if (mintype == 0) {
    // don't devide anymore is the best choice
    return;
  }

  if (mintype == 1) {
    estjjj = (estjjj + 1) / 2;
    ttj *= 2;
    postEstAdjust();

    return;
  }

  if (mintype == 2) {
    estkkk = (estkkk + 1) / 2;
    ttk *= 2;
    postEstAdjust();
    return;
  }

  if (mintype == 3) {
    estjjj = (estjjj + 1) / 2;
    estkkk = (estkkk + 1) / 2;
    ttj *= 2;
    ttk *= 2;
    postEstAdjust();
    return;
  }
}
