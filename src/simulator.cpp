#include "cache.h"
#include "dynamic.h"
#include "estimation.h"
#include "headers.h"
#include "statistics.h"

// store all the buffered C now
set<int> bufferedC[MAXN];
// record length of buffered C
// equals bufferedC[i].size()
int bufferedClen[MAXN];

int BLOCKSIZE = 16;

int beginA[MAXN];
int beginB[MAXN];

int beginAc[MAXN];
int beginBc[MAXN];

int begin[MAXN];

/*
The current fiber size of each array
stored according to the dataflow order
(not the storage format order)

update currsize each time the block of the array changes
(when inter-iterate)
*/
int currsizeA[MAXN];
int currsizeAc[MAXN];
int currsizeB[MAXN];
int currsizeBc[MAXN];
int currsizeC[MAXN];

/*
The currently buffered size of each array
(part of cuursizeA/B/C)

update bufferedsize each time
*/
int bufferedsizeA[MAXN];
int bufferedsizeB[MAXN];
int bufferedsizeC[MAXN];

int tmpC[MAXN];

// start of current block
int TI, TJ, TK;

bool ISDYNAMICJ = 0;
// dynamic j, play same as jjj
int dynj;

bool ISDYNAMICK = 0;
// dynamic k, play same as kkk
int dynk;

bool ISDYNAMICI = 0;
int dyni;

int PartialConfig;

bool check_outer_loop() {
  if ((interorder == KIJ) || (interorder == KJI)) {
    return TK < K;
  } else if ((interorder == JIK) || (interorder == JKI)) {
    return TJ < J;
  } else if ((interorder == IJK) || (interorder == IKJ)) {
    return TI < I;
  }
  return 0;
}

bool check_inner_loop() {

  if ((interorder == IJK) || (interorder == JIK)) {
    return (TK < K);
  } else if ((interorder == IKJ) || (interorder == KIJ)) {
    return (TJ < J);
  } else if ((interorder == JKI) || (interorder == KJI)) {
    return (TI < I);
  }
  return 0;
}

bool check_mid_loop() {

  if ((interorder == IKJ) || (interorder == JKI)) {
    return (TK < K);
  } else if ((interorder == IJK) || (interorder == KJI)) {
    return (TJ < J);
  } else if ((interorder == JIK) || (interorder == KIJ)) {
    return (TI < I);
  }
  return 0;
}

// STAR: call this
// when: 1) start time 2) each time update I/J
// can over called by call each time
/*
update currsizeA/B/C here!
(each time the block iterate)
currsize is consistent to dataflow order
*/
void updateBlockA() {

  // Row-majored
  if (dataflow == Inner || dataflow == Gust) {

    for (int ti = TI; ti < TI + iii; ti++) {
      if (ti > I)
        break;

      int startj = beginA[ti], tmpj = beginA[ti],
          maxj = offsetarrayA[ti + 1] - offsetarrayA[ti];

      // jjj -> ((ISDYNAMICJ)?dynj:jjj)

      // wrong!  this function is use to calculate the new currsizeA
      // can't simply change to dynj
      // solution: add a update fuction after pre-load B

      // check: will pre_load use this currsizeA and currsizeB ? -> cause cycle
      while (tmpj < maxj && A[ti][tmpj] < ((ISDYNAMICJ) ? dynj : jjj) + TJ) {
        tmpj++;
      }

      currsizeA[ti] = tmpj - startj;
    }
  }

  // Col-majored
  if (dataflow == Outer) {

    for (int tj = TJ; tj < TJ + ((ISDYNAMICJ) ? dynj : jjj); tj++) {
      if (tj > J)
        break;

      int starti = beginAc[tj], tmpi = beginAc[tj],
          maxi = offsetarrayAc[tj + 1] = offsetarrayAc[tj];

      while (tmpi < maxi && Ac[tj][tmpi] < iii + TI) {
        tmpi++;
      }

      currsizeAc[tj] = tmpi - starti;
    }
  }
}

void updateBlockB() {

  // Row-majored
  if (dataflow == Gust || dataflow == Outer) {

    for (int tj = TJ; tj < TJ + ((ISDYNAMICJ) ? dynj : jjj); tj++) {
      if (tj > J)
        break;

      int startk = beginB[tj], tmpk = beginB[tj],
          maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

      while (tmpk < maxk && B[tj][tmpk] < ((ISDYNAMICK) ? dynk : kkk) + TK) {
        tmpk++;
      }

      currsizeB[tj] = tmpk - startk;
    }
  }

  // Col-majored
  if (dataflow == Inner) {

    for (int tk = TK; tk < TK + ((ISDYNAMICK) ? dynk : kkk); tk++) {
      if (tk > K)
        break;

      int startj = beginBc[tk], tmpj = beginBc[tk],
          maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

      while (tmpj < maxj && Bc[tk][tmpj] < ((ISDYNAMICJ) ? dynj : jjj) + TJ) {
        tmpj++;
      }

      currsizeBc[tk] = tmpj - startj;
    }
  }
}

void updateBlockC() {

  // Don't have determined beginC and currsizeC
}

// beginA only related to TJ -> call every time update TJ
void forcebeginA() {
  for (int i = 0; i < I; i++) {

    int startj = 0, tmpj = 0, maxj = offsetarrayA[i + 1] - offsetarrayA[i];

    // here is TJ because TJ have added jjj before call the func
    while (tmpj < maxj && A[i][tmpj] < TJ) {
      tmpj++;
    }

    beginA[i] = tmpj;
  }

  for (int tj = 0; tj < J; tj++) {
    int starti = 0, tmpi = 0, maxi = offsetarrayAc[tj + 1] - offsetarrayAc[tj];

    while (tmpi < maxi && Ac[tj][tmpi] < TI) {
      tmpi++;
    }
    beginAc[tj] = tmpi;
  }
}

void forcebeginB() {

  for (int tj = 0; tj < J; tj++) {

    int startk = 0, tmpk = 0, maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

    while (tmpk < maxk && B[tj][tmpk] < TK) {
      tmpk++;
    }

    beginB[tj] = tmpk;
  }

  for (int tk = 0; tk < K; tk++) {

    int startj = 0, tmpj = 0, maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

    while (tmpj < maxj && Bc[tk][tmpj] < TJ) {
      tmpj++;
    }

    beginBc[tk] = tmpj;
  }
}

// each time after update TJ
void updateBeginA() {
  for (int ti = TI; ti < TI + iii; ti++) {
    if (ti > I)
      break;

    int startj = beginA[ti], tmpj = beginA[ti],
        maxj = offsetarrayA[ti + 1] - offsetarrayA[ti];

    // here is TJ because TJ have added jjj before call the func
    while (tmpj < maxj && A[ti][tmpj] < TJ) {
      tmpj++;
    }

    beginA[ti] = tmpj;
  }
}

void ALLupdateBeginAc() {
  for (int tj = 0; tj < J; tj++) {
    if (tj > J)
      break;

    int starti = beginAc[tj], tmpi = beginAc[tj],
        maxi = offsetarrayAc[tj + 1] - offsetarrayAc[tj];

    while (tmpi < maxi && Ac[tj][tmpi] < TI) {
      tmpi++;
    }

    beginAc[tj] = tmpi;
  }
}

void AllupdateBeginA() {
  for (int ti = 0; ti < I; ti++) {
    if (ti > I)
      break;

    int startj = beginA[ti], tmpj = beginA[ti],
        maxj = offsetarrayA[ti + 1] - offsetarrayA[ti];
    while (tmpj < maxj && A[ti][tmpj] < TJ) {
      tmpj++;
    }

    beginA[ti] = tmpj;
    // printf("%d %d %d %d   ", startj, tmpj, maxj, beginA[ti]);
  }
}

// each time update TI
void updateBeginAc() {
  for (int tj = TJ; tj < TJ + ((ISDYNAMICJ) ? dynj : jjj); tj++) {
    if (tj > J)
      break;

    int starti = beginAc[tj], tmpi = beginAc[tj],
        maxi = offsetarrayAc[tj + 1] - offsetarrayAc[tj];

    while (tmpi < maxi && Ac[tj][tmpi] < TI) {
      tmpi++;
    }

    beginAc[tj] = tmpi;
  }
}

void AllupdateBeginB() {

  // update beginB
  for (int tj = 0; tj < J; tj++) {
    if (tj > J)
      break;

    int startk = beginB[tj], tmpk = beginB[tj],
        maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

    while (tmpk < maxk && B[tj][tmpk] < TK) {
      tmpk++;
    }

    beginB[tj] = tmpk;
  }
}

void AllupdateBeginBc() {

  // update beginBc
  for (int tk = 0; tk < K; tk++) {
    if (tk > K)
      break;

    int startj = beginBc[tk], tmpj = beginBc[tk],
        maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

    while (tmpj < maxj && Bc[tk][tmpj] < TJ) {
      tmpj++;
    }

    beginBc[tk] = tmpj;
  }
}

// each time update Tk
void updateBeginB() {

  // update beginB
  for (int tj = TJ; tj < TJ + ((ISDYNAMICJ) ? dynj : jjj); tj++) {
    if (tj > J)
      break;

    int startk = beginB[tj], tmpk = beginB[tj],
        maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

    while (tmpk < maxk && B[tj][tmpk] < TK) {
      tmpk++;
    }

    beginB[tj] = tmpk;
  }
}

// eachtime update TJ
void updateBeginBc() {
  // update beginBc
  for (int tk = TK; tk < TK + ((ISDYNAMICK) ? dynk : kkk); tk++) {
    if (tk > K)
      break;

    int startj = beginBc[tk], tmpj = beginBc[tk],
        maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

    while (tmpj < maxj && Bc[tk][tmpj] < TJ) {
      tmpj++;
    }

    beginBc[tk] = tmpj;
  }
}

void updateBeginC() {}

void reinitialize_beginA() {
  for (int ti = 0; ti < I; ti++) {
    beginA[ti] = 0;
  }
}
void reinitialize_beginAc() {
  for (int tj = 0; tj < J; tj++) {
    beginAc[tj] = 0;
  }
}
void reinitialize_beginB() {
  for (int tj = 0; tj < J; tj++) {
    beginB[tj] = 0;
  }
}
void reinitialize_beginBc() {
  for (int tk = 0; tk < K; tk++) {
    beginBc[tk] = 0;
  }
}
void reinitialize_beginC() {
  /* for(int ti = 0; ti < I; ti ++){
       beginC[ti] = 0;
   }*/
}

// return 1 if I is before J in the interorder
// return 0 if J -> I
bool isIJ() {
  if (interorder == IJK || interorder == IKJ || interorder == KIJ)
    return 1;
  return 0;
}

// return 1 if J is before K in the interorder
// return 0 if K -> J
bool isJK() {
  if (interorder == JKI || interorder == JIK || interorder == IJK)
    return 1;
  return 0;
}

void updateTI() {
  TI += iii;

  if (isIJ()) {
    ALLupdateBeginAc();
  } else {
    updateBeginAc();
  }
}

void updateTJ() {
  TJ += ((ISDYNAMICJ) ? dynj : jjj);

  if (isIJ()) {
    updateBeginA();
  } else {
    AllupdateBeginA();
  }

  if (isJK()) {
    AllupdateBeginBc();
  } else {
    updateBeginBc();
  }
}

void updateTK() {
  TK += ((ISDYNAMICK) ? dynk : kkk);

  if (isJK()) {
    updateBeginB();
  } else {
    AllupdateBeginB();
  }
}

bool iterate_inner_loop() {
  if ((interorder == IJK) || (interorder == JIK)) {
    // adddyn
    // dynamicupdatek();
    if (TK + ((ISDYNAMICK) ? dynk : kkk) < K) {
      updateTK();
      if (ISDYNAMICK) {
        dynamicupdatek();
      }
      return 1;
    } else {
      TK += ((ISDYNAMICK) ? dynk : kkk);
      if (ISDYNAMICK) {
        dynamicupdatek();
      }
      return 0;
    }
  } else if ((interorder == IKJ) || (interorder == KIJ)) {
    // dynamicupdatej();
    if (TJ + ((ISDYNAMICJ) ? dynj : jjj) < J) {
      updateTJ();
      if (ISDYNAMICJ) {
        dynamicupdatej();
      }
      return 1;
    } else {
      TJ += ((ISDYNAMICJ) ? dynj : jjj);
      if (ISDYNAMICJ) {
        dynamicupdatej();
      }
      return 0;
    }
  } else if ((interorder == JKI) || (interorder == KJI)) {
    //  printf("####  %d %d %d\n", TI, iii, I);
    // dynamicupdatei();
    if (TI + iii < I) {
      updateTI();
      if (ISDYNAMICI) {
        dynamicupdatei();
      }
      return 1;

    } else {
      TI += iii;
      if (ISDYNAMICI) {
        dynamicupdatei();
      }
      return 0;
    }
  }

  return 0;
}

bool iterate_mid_loop() {
  if ((interorder == IKJ) || (interorder == JKI)) {
    // dynamicupdatek();
    if (TK + ((ISDYNAMICK) ? dynk : kkk) < K) {
      updateTK();
      if (ISDYNAMICK) {
        dynamicupdatek();
      }
      return 1;
    } else {
      TK += ((ISDYNAMICK) ? dynk : kkk);
      if (ISDYNAMICK) {
        dynamicupdatek();
      }
      return 0;
    }
  } else if ((interorder == IJK) || (interorder == KJI)) {
    // dynamicupdatej();
    if (TJ + ((ISDYNAMICJ) ? dynj : jjj) < J) {
      updateTJ();
      if (ISDYNAMICJ) {
        dynamicupdatej();
      }
      return 1;
    } else {
      TJ += ((ISDYNAMICJ) ? dynj : jjj);
      if (ISDYNAMICJ) {
        dynamicupdatej();
      }
      return 0;
    }
  } else if ((interorder == JIK) || (interorder == KIJ)) {
    // dynamicupdatei();
    if (TI + iii < I) {
      updateTI();
      if (ISDYNAMICI) {
        dynamicupdatei();
      }
      return 1;
    } else {
      TI += iii;
      if (ISDYNAMICI) {
        dynamicupdatei();
      }
      return 0;
    }
  }

  return 0;
}

bool iterate_outer_loop() {
  if ((interorder == KIJ) || (interorder == KJI)) {
    // dynamicupdatek();
    if (TK + ((ISDYNAMICK) ? dynk : kkk) < K) {
      updateTK();
      if (ISDYNAMICK) {
        dynamicupdatek();
      }
      return 1;
    } else {
      TK += ((ISDYNAMICK) ? dynk : kkk);
      if (ISDYNAMICK) {
        dynamicupdatek();
      }
      return 0;
    }
  } else if ((interorder == JIK) || (interorder == JKI)) {
    // dynamicupdatej();
    if (TJ + ((ISDYNAMICJ) ? dynj : jjj) < J) {
      updateTJ();
      if (ISDYNAMICJ) {
        dynamicupdatej();
      }
      return 1;
    } else {
      TJ += ((ISDYNAMICJ) ? dynj : jjj);
      if (ISDYNAMICJ) {
        dynamicupdatej();
      }
      return 0;
    }
  } else if ((interorder == IJK) || (interorder == IKJ)) {
    // dynamicupdatei();
    if (TI + iii < I) {
      updateTI();
      if (ISDYNAMICI) {
        dynamicupdatei();
      }
      return 1;
    } else {
      TI += iii;
      if (ISDYNAMICI) {
        dynamicupdatei();
      }
      return 0;
    }
  }

  return 0;
}

void reverse_I() {
  TI = 0;

  // reinitialize A
  // if((format == CR) || (format == CC)){
  if (isIJ()) {
    for (int tmpj = 0; tmpj < J; tmpj++) {
      beginAc[tmpj] = 0;
    }
  } else {
    for (int tmpj = TJ; tmpj < TJ + jjj; tmpj++) {
      beginAc[tmpj] = 0;
    }
  }
  //}
}

void reverse_J() {

  TJ = 0;
  // reinitialize A
  // if((format == RR) || (format == RC)){

  if (isIJ()) {
    for (int tmpi = TI; tmpi < TI + iii; tmpi++) {
      beginA[tmpi] = 0;
    }
  } else {
    for (int tmpi = 0; tmpi < I; tmpi++) {
      beginA[tmpi] = 0;
    }
  }
  // }

  // reinitialize Bc
  // if((format == RC) || (format == CC)){

  if (isJK()) {
    for (int tmpk = 0; tmpk < K; tmpk++) {
      beginBc[tmpk] = 0;
    }
  } else {
    for (int tmpk = TK; tmpk < TK + kkk; tmpk++) {
      beginBc[tmpk] = 0;
    }
  }

  //}
}

void reverse_K() {
  TK = 0;
  // reinitialize B
  // if((format == RR) || (format == CR)){

  if (isJK()) {

    for (int tmpj = TJ; tmpj < TJ + jjj; tmpj++) {
      beginB[tmpj] = 0;
    }
  } else {
    for (int tmpj = 0; tmpj < J; tmpj++) {
      beginB[tmpj] = 0;
    }
  }

  // }
}

void reverse_inner() {
  if ((interorder == IJK) || (interorder == JIK)) {
    reverse_K();
  } else if ((interorder == IKJ) || (interorder == KIJ)) {
    reverse_J();
  } else if ((interorder == JKI) || (interorder == KJI)) {
    reverse_I();
  }
}

void reverse_mid() {
  if ((interorder == IKJ) || (interorder == JKI)) {
    reverse_K();
  } else if ((interorder == IJK) || (interorder == KJI)) {
    reverse_J();
  } else if ((interorder == JIK) || (interorder == KIJ)) {
    reverse_I();
  }
}

void reinitialize_inner() {
  if ((interorder == IJK) || (interorder == JIK)) {
    reverse_K();
  } else if ((interorder == IKJ) || (interorder == KIJ)) {
    reverse_J();
  } else if ((interorder == JKI) || (interorder == KJI)) {
    reverse_I();
  }
}

void reinitialize_mid() {
  if ((interorder == IKJ) || (interorder == JKI)) {
    reverse_K();
  } else if ((interorder == IJK) || (interorder == KJI)) {
    reverse_J();
  } else if ((interorder == JIK) || (interorder == KIJ)) {
    reverse_I();
  }
}

void reinitialize_outer() {
  if ((interorder == KIJ) || (interorder == KJI)) {
    reverse_K();
  } else if ((interorder == JIK) || (interorder == JKI)) {
    reverse_J();
  } else if ((interorder == IJK) || (interorder == IKJ)) {
    reverse_I();
  }
}

// then will take length+1 place for the next fiberlet pointer
const int fiberletlength = 4;

/*
The allocated buffer to an array (A/B/Csize)
(accordind to "partial" parameter and dataflow)
and the currently used part (A/B/Csizenow)
*/
int Asizenow;
int Bsizenow;
int Csizenow;

bool fulltagA, fulltagB, fulltagC;
int fullA, fullB, fullC;

bool checkAndLoadReuseA() {
  if ((interorder == IJK || interorder == JIK)) {
    // load inter-reuse A
    // the TK == 0 should get a ratio of inter-level cache hit (how many in the
    // buffer)

    // need to reaccess if the buffer can't hold the full A
    // int restDram = 0;
    // int restSram = 0;

    if (TK == 0) {

      Asizenow = 0;
      fulltagA = 0;
      if (format == RR || format == RC) {

        // on-chip fiber start

        for (int ti = TI; ti < TI + iii; ti++) {
          if (ti > I)
            break;
          Asizenow++;

          int startj = beginA[ti], tmpj = beginA[ti],
              maxj = offsetarrayA[ti + 1] - offsetarrayA[ti];

          while (tmpj < maxj && A[ti][tmpj] < jjj + TJ) {
            tmpj++;
          }

          int tmpsize = (tmpj - startj);

          // overflow the buffer size
          // need to load again in rest tiles
          if (Asizenow + tmpsize * 3 >= Asize) {
            // outside the buffer, need reload every following tiles
            if (!fulltagA) {
              fulltagA = 1;
              fullA = ti;
            }
          } else {
            // inside the buffer, don't need to reload in following tiles
            Asizenow += tmpsize * 3;
            preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
            preA += memoryBandwidthWhole(tmpsize * 3 + 2);
            preSramAccess += sramWriteBandwidth(tmpsize * 3 + 2);
            AccessByte += tmpsize * 3 + 2;
          }
        }
      }

      // haven't consider inconsistent format now
      if (format == CR || format == CC) {
      }

      return 1;
    } else {
      return 1;
    }
  } else {

    // if interorder not IJK or JIK, can not buffer A whatever the buffersize
    // so just set the fullA to TI-1 (the first place)
    fulltagA = 1;
    fullA = TI - 1;
  }

  return 0;

  // need: the reusable inter loop + not the first tile (need to load at the
  // first time)
  if ((interorder == IJK || interorder == JIK) && (TK != 0))
    return 1;
  return 0;
}

bool checkReuseB() {
  if ((interorder == JKI || interorder == KJI) && (TI != 0))
    return 1;
  return 0;
}

bool checkReuseC() {
  if ((interorder == IKJ || interorder == KIJ) && (TJ != 0))
    return 1;
  return 0;
}

void pre_load_A() {

  if (dataflow == Outer) {
    fulltagA = 1;
    fullA = TJ;

    return;
  }

  // if reuse A, don't need to load again
  if (checkAndLoadReuseA()) {
    return;
  }

  // we suppose a consistent format now, don't need the following
  return;

  // When need load A:
  // 2 scenario: 1) When A storage format mismatch with dataflow. (Otherwise
  // don't need to buffer A in 3 dataflow) 2) inter reuse A (load A at the first
  // loop, then don't need to load again)
  // -> the second scenario is not free!! need to alloc buffer for it; and can
  // only reuse the partition inside the buffer

  // mismatch of Gust

  Asizenow = 0;
  fulltagA = 0;

  if ((dataflow == Gust) && ((format == CC) || (format == CR))) {
    // implicit transform while loading

    // estimated fiberlet fragment waste
    Asize += ((fiberletlength + 1) / 2) * iii;

    // on-chip fiber current
    // This is why very bad before!!!
    Asize += iii;

    // Initialize
    for (int ti = TI; ti < TI + iii; ti++) {
      bufferedsizeA[ti] = 0;
    }

    for (int tj = TJ; tj < TJ + jjj; tj++) {
      if (tj > J)
        break;

      int starti = beginAc[tj], tmpi = beginAc[tj],
          maxi = offsetarrayAc[tj + 1] - offsetarrayAc[tj];

      while (tmpi < maxi) {
        int tmpindex = Ac[tj][tmpi];
        if (tmpindex >= TI + iii) {
          break;
        }
        tmpi++;

        bufferedsizeA[tmpindex]++;
      }

      int tmpsize = (tmpi - starti);

      if (Asizenow + tmpsize * 3 >= Asize) {
        if (!fulltagA) {
          fulltagA = 1;
          fullA = tj;
        }
        // cache the csc size of each col block
        // currsizeAc[tj] = tmpsize;
      } else {
        // currsizeAc[tj] = tmpsize;
        preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
        preA += memoryBandwidthWhole(tmpsize * 3 + 2);
        AccessByte += tmpsize * 2 + 2;

        // for each element need:
        // 1) get pos: one read (current position)
        // 2) add to chain: 1 data write; 1/block chain index write
        preSramAccess += sramWriteBandwidth(tmpsize);
        preSramAccess +=
            sramReadBandwidth(tmpsize + tmpsize / fiberletlength) * 3;
      }
    }
  }

  // mismatch of IP

  if ((dataflow == Inner) && ((format == CC) || (format == CR))) {
    // implicit transform while loading

    // estimated fiberlet fragment waste
    Asize += ((fiberletlength + 1) / 2) * iii;

    // on-chip fiber current
    // This is why very bad before!!!
    Asize += iii;

    for (int ti = TI; ti < TI + iii; ti++) {
      bufferedsizeA[ti] = 0;
    }

    for (int tj = TJ; tj < TJ + jjj; tj++) {
      if (tj > J)
        break;

      int starti = beginAc[tj], tmpi = beginAc[tj],
          maxi = offsetarrayAc[tj + 1] - offsetarrayAc[tj];

      while (tmpi < maxi) {
        int tmpindex = Ac[tj][tmpi];
        if (tmpindex >= TI + iii) {
          break;
        }
        tmpi++;

        bufferedsizeA[tmpindex]++;
      }

      int tmpsize = (tmpi - starti);

      if (Asizenow + tmpsize * 3 >= Asize) {
        if (!fulltagA) {
          fulltagA = 1;
          fullA = tj;
        }
        // cache the csc size of each col block
        // currsizeAc[tj] = tmpsize;
      } else {
        // currsizeAc[tj] = tmpsize;
        preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
        preA += memoryBandwidthWhole(tmpsize * 3 + 2);
        AccessByte += tmpsize * 3 + 2;

        // for each element need:
        // 1) get pos: one read (current position)
        // 2) add to chain: 1 data write; 1/block chain index write
        preSramAccess += sramWriteBandwidth(tmpsize);
        preSramAccess +=
            sramReadBandwidth(tmpsize + tmpsize / fiberletlength) * 3;
      }
    }
  }

  // mismatch of OP (different)

  if ((dataflow == Outer) && ((format == RC) || (format == RR))) {
    // implicit transform while loading

    // estimated fiberlet fragment waste
    Asize += ((fiberletlength + 1) / 2) * jjj;

    // on-chip fiber current

    Asize += jjj;

    // Initialize
    for (int tj = TJ; tj < TJ + jjj; tj++) {
      bufferedsizeA[tj] = 0;
    }

    for (int ti = TI; ti < TI + iii; ti++) {
      if (ti > I)
        break;

      int startj = beginA[ti], tmpj = beginA[ti],
          maxj = offsetarrayA[ti + 1] - offsetarrayA[ti];

      while (tmpj < maxj) {
        int tmpindex = A[ti][tmpj];
        if (tmpindex >= TJ + jjj) {
          break;
        }
        tmpj++;

        bufferedsizeA[tmpindex]++;
      }

      int tmpsize = (tmpj - startj);

      if (Asizenow + tmpsize * 3 >= Asize) {
        if (!fulltagA) {
          fulltagA = 1;
          fullA = ti;
        }
        // cache the csc size of each col block
        // currsizeAc[tj] = tmpsize;
      } else {
        // currsizeAc[tj] = tmpsize;
        preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
        preA += memoryBandwidthWhole(tmpsize * 3 + 2);
        AccessByte += tmpsize * 3 + 2;

        // for each element need:
        // 1) get pos: one read (current position)
        // 2) add to chain: 1 data write; 1/block chain index write
        preSramAccess += sramWriteBandwidth(tmpsize);
        preSramAccess +=
            sramReadBandwidth(tmpsize + tmpsize / fiberletlength) * 3;
      }
    }
  }
}

void pre_load_B() {

  // if reuse B, don't need to load again
  if (checkReuseB()) {
    return;
  }

  if (dataflow == Outer) {
    fulltagB = 1;
    fullB = TJ;

    return;
  }

  // When need load B:
  // 2 scenarios: 1) IP & Gust (need to buffer B)  2) When B storage format
  // mismatch with dataflow

  // need to consider with inter-block
  Bsizenow = 0;
  fulltagB = 0;
  fullB = 0;

  // scenario 1
  if ((dataflow == Inner) || (dataflow == Gust)) {

    // Gust
    Bsizenow = 0;

    // check fulltagA
    fulltagB = 0;
    fullB = 0;

    if ((dataflow == Gust) && ((format == CR) || (format == RR))) {
      // consistent format

      int tj;

      // equals to 0(when jjj in the first half) or 1(when jjj in the second
      // half);
      int _TJ, _TK;

      for (tj = TJ; tj < TJ + jjj; tj++) {
        if (tj > J)
          break;

        if ((tj - TJ) < (jjj / 2)) {
          _TJ = 0;
        } else {
          _TJ = 1;
        }

        // on-chip fiber start
        Bsizenow++;

        int startk = beginB[tj], tmpk = beginB[tj],
            maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

        int halfk = beginB[tj];

        while (halfk < maxk && B[tj][halfk] < (kkk / 2) + TK) {
          halfk++;
        }
        tmpk = halfk;

        while (tmpk < maxk && B[tj][tmpk] < kkk + TK) {
          tmpk++;
        }

        int tmpsize = (tmpk - startk);

        updateDynamicTile(_TJ, (halfk - startk), tmpk - halfk);
        // Tcnt[_TJ][0] += (halfk-startk);
        // Tcnt[_TJ][1] += (tmpk-halfk);

        if (Bsizenow + tmpsize * 3 >= Bsize) {
          if (!fulltagB) {
            fulltagB = 1;
            fullB = tj;
          }
          //  currsizeB[tj] = tmpsize;
        } else {

          Bsizenow += tmpsize * 3;

          preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
          preB += memoryBandwidthWhole(tmpsize * 3 + 2);
          preSramAccess += sramWriteBandwidth(tmpsize * 3 + 2);
          AccessByte += tmpsize * 3 + 2;
        }
      }

      if (ISDYNAMICJ) {

        // dynamic growing if the buffer is not full!
        if (fulltagB == 0) {

          // start to grow from the last tj
          for (; tj < J; tj++) {

            // on-chip fiber start
            Bsizenow++;

            int startk = beginB[tj], tmpk = beginB[tj],
                maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

            while (tmpk < maxk && B[tj][tmpk] < kkk + TK) {
              tmpk++;
            }

            int tmpsize = (tmpk - startk);

            if (Bsizenow + tmpsize * 3 >= Bsize) {
              dynj = tj - TJ;
              break;
            } else {

              Bsizenow += tmpsize * 3;

              preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
              preB += memoryBandwidthWhole(tmpsize * 3 + 2);
              preSramAccess += sramWriteBandwidth(tmpsize * 3 + 2);
              AccessByte += tmpsize * 3 + 2;
            }
          }

          dynj = tj - TJ;

        } else {
          // the buffer is already full. don't need to increase. (will it
          // shrink? ) we can try both the two mode : shrink or not.

          dynj = jjj;
        }
      }
    }

    if ((dataflow == Gust) && ((format == CC) || (format == RC))) {
      // inconsistent format
      // implicit transform while loading

      // Unlike the consistent version, inconsistent version
      // can't do this

      // estimated fiberlet fragment waste
      Bsizenow += ((fiberletlength + 1) / 2) * jjj;

      // on-chip fiber current
      Bsizenow += jjj;

      // Initialize
      for (int tj = TJ; tj < TJ + jjj; tj++) {
        bufferedsizeB[tj] = 0;
      }

      for (int tk = TK; tk < TK + kkk; tk++) {
        if (tk > K)
          break;

        int startj = beginBc[tk], tmpj = beginBc[tk],
            maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

        while (tmpj < maxj) {
          int tmpindex = Bc[tk][tmpj];
          if (tmpindex >= TJ + jjj) {
            break;
          }
          tmpj++;

          bufferedsizeB[tmpindex]++;
        }

        int tmpsize = (tmpj - startj);

        if (Bsizenow + tmpsize * 3 >= Bsize) {
          if (!fulltagB) {
            fulltagB = 1;
            fullB = tk;
          }
          // cache the csc size of each col block
          //   currsizeBc[tk] = tmpsize;
        } else {
          //   currsizeBc[tk] = tmpsize;
          preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
          preB += memoryBandwidthWhole(tmpsize * 3 + 2);
          AccessByte += tmpsize * 3 + 2;

          // for each element need:
          // 1) get pos: one read (current position)
          // 2) add to chain: 1 data write; 1/block chain index write
          preSramAccess += sramWriteBandwidth(tmpsize);
          preSramAccess +=
              sramReadBandwidth(tmpsize + tmpsize / fiberletlength) * 3;
        }
      }
    }

    // Inner

    if ((dataflow == Inner) && (((format == RC) || (format == CC)))) {
      // consistent format

      Bsizenow = 0;
      fulltagB = 0;
      fullB = 0;

      int tk;
      // 这里preload就是为了确定dynamic的
      // for(tk = TK; tk < TK+((ISDYNAMICK)?dynk:kkk); tk ++){
      for (tk = TK; tk < TK + kkk; tk++) {
        if (tk > K)
          break;

        // on-chip fiber start
        Bsizenow++;

        int startj = beginBc[tk], tmpj = beginBc[tk],
            maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

        while (tmpj < maxj && Bc[tk][tmpj] < jjj + TJ) {
          tmpj++;
        }

        int tmpsize = (tmpj - startj);

        if (Bsizenow + tmpsize * 3 >= Bsize) {
          if (!fulltagB) {
            fulltagB = 1;
            fullB = tk;
            // printf("!!!! %d %d %d %d %d %d %d\n", TI, TJ, TK, iii, jjj, kkk,
            // tk);
          }
          //  currsizeB[tj] = tmpsize;
        } else {

          Bsizenow += tmpsize * 3;

          preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
          preB += memoryBandwidthWhole(tmpsize * 3 + 2);
          preSramAccess += sramWriteBandwidth(tmpsize * 3 + 2);
          AccessByte += tmpsize * 3 + 2;
        }
      }

      // update the IP dynamic here
      // IPTODO !!!
      if (ISDYNAMICK) {

        // dynamic growing if the buffer is not full!
        if (fulltagB == 0) {

          // start to grow from the last tk
          for (; tk < K; tk++) {

            // on-chip fiber start
            Bsizenow++;

            int startj = beginBc[tk], tmpj = beginBc[tk],
                maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

            while (tmpj < maxj && Bc[tk][tmpj] < jjj + TJ) {
              tmpj++;
            }

            int tmpsize = (tmpj - startj);

            if (Bsizenow + tmpsize * 3 >= Bsize) {
              dynk = tk - TK;
              break;
            } else {

              Bsizenow += tmpsize * 3;

              preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
              preB += memoryBandwidthWhole(tmpsize * 3 + 2);
              preSramAccess += sramWriteBandwidth(tmpsize * 3 + 2);
              AccessByte += tmpsize * 3 + 2;
            }
          }

          dynk = tk - TK;

        } else {
          // the buffer is already full. don't need to increase. (will it
          // shrink? ) we can try both the two mode : shrink or not.

          dynk = kkk;
        }
      }
    }

    if ((dataflow == Inner) && (((format == RR) || (format == CR)))) {
      // inconsistent format

      // estimated fiberlet fragment waste
      Bsizenow += ((fiberletlength + 1) / 2) * kkk;

      // on-chip fiber current
      Bsizenow += kkk;

      // initialize
      for (int tk = TK; tk < TK + ((ISDYNAMICK) ? dynk : kkk); tk++) {
        bufferedsizeB[tk] = 0;
      }

      for (int tj = TJ; tj < TJ + jjj; tj++) {
        if (tj > J)
          break;

        int startk = beginB[tj], tmpk = beginB[tj],
            maxk = offsetarrayB[tj + 1] - offsetarrayB[tj];

        while (tmpk < maxk) {
          int tmpindex = B[tj][tmpk];
          if (tmpindex >= TK + ((ISDYNAMICK) ? dynk : kkk)) {
            break;
          }
          tmpk++;

          bufferedsizeB[tmpindex]++;
        }

        int tmpsize = (tmpk - startk);

        if (Bsizenow + tmpsize * 3 >= Bsize) {
          if (!fulltagB) {
            fulltagB = 1;
            fullB = tj;
          }
          // cache the csc size of each col block
          //   currsizeBc[tk] = tmpsize;
        } else {
          //   currsizeBc[tk] = tmpsize;
          preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
          preB += memoryBandwidthWhole(tmpsize * 3 + 2);
          AccessByte += tmpsize * 3 + 2;

          // for each element need:
          // 1) get pos: one read (current position)
          // 2) add to chain: 1 data write; 1/block chain index write
          preSramAccess += sramWriteBandwidth(tmpsize);
          preSramAccess +=
              sramReadBandwidth(tmpsize + tmpsize / fiberletlength) * 3;
        }
      }
    }
  }

  // scenario 2: mismatch

  // mismatch of the Gust and Inner have been considered in scenario 1
  if ((dataflow == Outer)) {

    Bsizenow = 0;
    fulltagB = 0;
    fullB = 0;

    // match: don't need buffer
    if ((format == RR) || (format == CR)) {
      // don't need pre load
      return;
    }

    // mismatch: buffered; same as Gust mismatch!
    if ((format == RC) || (format == CC)) {
      // inconsistent format
      // implicit transform while loading

      // estimated fiberlet fragment waste
      Bsizenow += ((fiberletlength + 1) / 2) * jjj;

      // on-chip fiber current
      Bsizenow += jjj;

      // Initialize
      for (int tj = TJ; tj < TJ + jjj; tj++) {
        bufferedsizeB[tj] = 0;
      }

      for (int tk = TK; tk < TK + kkk; tk++) {
        if (tk > K)
          break;

        int startj = beginBc[tk], tmpj = beginBc[tk],
            maxj = offsetarrayBc[tk + 1] - offsetarrayBc[tk];

        while (tmpj < maxj) {
          int tmpindex = Bc[tk][tmpj];
          if (tmpindex >= TJ + jjj) {
            break;
          }
          tmpj++;

          bufferedsizeB[tmpindex]++;
        }

        int tmpsize = (tmpj - startj);

        if (Bsizenow + tmpsize * 3 >= Bsize) {
          if (!fulltagB) {
            fulltagB = 1;
            fullB = tk;
          }
          // cache the csc size of each col block
          //   currsizeBc[tk] = tmpsize;
        } else {
          //   currsizeBc[tk] = tmpsize;
          preDramAccess += memoryBandwidthWhole(tmpsize * 3 + 2);
          preB += memoryBandwidthWhole(tmpsize * 3 + 2);
          AccessByte += tmpsize * 3 + 2;

          // for each element need:
          // 1) get pos: one read (current position)
          // 2) add to chain: 1 data write; 1/block chain index write
          preSramAccess += sramWriteBandwidth(tmpsize);
          preSramAccess +=
              sramReadBandwidth(tmpsize + tmpsize / fiberletlength) * 3;
        }
      }
    }
  }
}

/*
Load streams(A/B) into buffer before calculation.
*/
void pre_calculate_load() {

  preSramAccess = preDramAccess = 0;

  if (ISCACHE) {
    initializeCacheValid();
  }

  // only will preload in blocking mode
  // cache mode don't need preload
  if (!ISCACHE) {
    pre_load_B();
  }

  pre_load_A();

  preSramAccess /= sramBank;

  totalCycle += max(preSramAccess, preDramAccess);
  preCycle += max(preSramAccess, preDramAccess);
}

bool consistent_B() {
  if (dataflow == Gust) {
    if ((format == RR) || (format == CR))
      return 1;
    if ((format == RC) || (format == CC))
      return 0;
  }
  if (dataflow == Inner) {
    if ((format == RC) || (format == CC))
      return 1;
    if ((format == RR) || (format == CR))
      return 0;
  }
  if (dataflow == Outer) {
    if ((format == RR) || (format == CR))
      return 1;
    if ((format == RC) || (format == CC))
      return 0;
  }
  return 0;
}

bool consistent_A() {
  if (dataflow == Gust) {
    if ((format == RR) || (format == RC))
      return 1;
    if ((format == CR) || (format == CC))
      return 0;
  }
  if (dataflow == Inner) {
    if ((format == RR) || (format == RC))
      return 1;
    if ((format == CR) || (format == CC))
      return 0;
  }
  if (dataflow == Outer) {
    if ((format == CR) || (format == CC))
      return 1;
    if ((format == RR) || (format == RC))
      return 0;
  }
  return 0;
}

// ii here means the now access position for OPT policy
void get_B_fiber(int jj, int ii) {

  // In Blocking Mode
  if (!ISCACHE) {

    // two decisions: 1) consistent or not; 2) buffer or not (may bypass)

    if (consistent_B()) {
      // B[jj] is on the buffer
      if (fulltagB == 0 || jj < fullB) {
        // hit!
        // different access with B format:
        // continuous or chained
        computeSramAccess += sramReadBandwidth(currsizeB[jj] * 3 + 2);

      } else {
        // B[jj] is not on the buffer, need to access dram
        // different access with B format
        // access one dram fiber all check all

        computeDramAccess += memoryBandwidthPE(currsizeB[jj] * 3 + 2);

        computeB += memoryBandwidthPE(currsizeB[jj] * 3 + 2);
        AccessByte += currsizeB[jj] * 3 + 2;
      }
    } else {
      // hit part (chained)
      computeSramAccess +=
          sramReadBandwidth(fiberletlength * 3) * ((bufferedsizeB[jj] + 3) / 4);

      // miss part (need to check every uncached)

      if (fulltagB) {
        computeDramAccess +=
            (memoryBandwidthPE(3)) * ((long long)TK + kkk - fullB);
        computeB +=
            (memoryBandwidthPE(3)) * (long long)((long long)TK + kkk - fullB);
        AccessByte += 3 * (long long)((long long)TK + kkk - fullB);
      }
    }

  }
  // In cache Mode
  // address in cache mode is : fiberid + (relative << bias)  where relative =
  // (relative coordinate in fiber)/CACHEBLOCK
  else {
    int fibersize = currsizeB[jj] * 3 + 2;
    cacheAccessFiber(jj, fibersize);
  }
}

// in IP
void get_B_fiber_col_iii(int kk, int iii) {

  // B[jj] is on the buffer
  if (fulltagB == 0 || kk < fullB) {
    // hit!
    // different access with B format:
    // continuous or chained
    computeSramAccess += sramReadBandwidth(currsizeBc[kk] * 3 + 2) * iii;

  } else {
    // B[jj] is not on the buffer, need to access dram
    // different access with B format
    // access one dram fiber all check all

    computeDramAccess += memoryBandwidthPE(currsizeBc[kk] * 3 + 2) * iii;

    computeB += memoryBandwidthPE(currsizeBc[kk] * 3 + 2) * iii;
    AccessByte += (currsizeBc[kk] * 3 + 2) * iii;
  }
}
void get_B_fiber_col(int kk) {
  if (consistent_B()) {
    // B[jj] is on the buffer
    if (fulltagB == 0 || kk < fullB) {
      // hit!
      // different access with B format:
      // continuous or chained
      computeSramAccess += sramReadBandwidth(currsizeBc[kk] * 3 + 2);

    } else {
      // B[jj] is not on the buffer, need to access dram
      // different access with B format
      // access one dram fiber all check all

      computeDramAccess += memoryBandwidthPE(currsizeBc[kk] * 3 + 2);

      computeB += memoryBandwidthPE(currsizeBc[kk] * 3 + 2);
      AccessByte += currsizeBc[kk] * 3 + 2;
    }
  } else {
    // hit part (chained)
    computeSramAccess +=
        sramReadBandwidth(fiberletlength * 3) * ((bufferedsizeB[kk] + 3) / 4);

    // miss part (need to check every uncached)

    if (fulltagB) {
      computeDramAccess +=
          (memoryBandwidthPE(3)) * ((long long)TJ + jjj - fullB);
      computeB +=
          (memoryBandwidthPE(3)) * (long long)((long long)TJ + jjj - fullB);
      AccessByte += 3 * ((long long)TJ + jjj - fullB);
    }
  }
}

void get_A_fiber_col(int jj) {

  if (consistent_A()) {

    // A[ii] is on the buffer
    if (fulltagA == 0 || jj < fullA) {
      // hit
      computeSramAccess += sramReadBandwidth(currsizeAc[jj] * 3 + 2);
    } else {

      computeDramAccess += memoryBandwidthPE(currsizeAc[jj] * 3 + 2);
      computeA += memoryBandwidthPE(currsizeAc[jj] * 3 + 2);
      AccessByte += currsizeAc[jj] * 3 + 2;

      computeSramAccess += sramReadBandwidth(currsizeAc[jj] * 3 + 2) +
                           sramWriteBandwidth(currsizeAc[jj] * 3 + 2);
    }
  } else {

    // hit part (chained)
    computeSramAccess +=
        sramReadBandwidth(fiberletlength * 3) * ((bufferedsizeA[jj] + 3) / 4);

    // miss part (need to check every uncached)

    if (fulltagA) {
      computeDramAccess +=
          (memoryBandwidthPE(3)) * ((long long)TI + iii - fullA);
      computeB +=
          (memoryBandwidthPE(3)) * (long long)((long long)TI + iii - fullA);
      AccessByte += 3 * ((long long)TI + iii - fullA);
    }
  }
}

void get_A_fiber(int ii) {

  if (consistent_A()) {

    // A[ii] is on the buffer
    if (fulltagA == 0 || ii < fullA) {
      // hit
      computeSramAccess += sramReadBandwidth(currsizeA[ii] * 3 + 2);
    } else {
      computeDramAccess += memoryBandwidthPE(currsizeA[ii] * 3 + 2);
      computeA += memoryBandwidthPE(currsizeA[ii] * 3 + 2);
      AccessByte += currsizeA[ii] * 3 + 2;

      computeSramAccess += sramReadBandwidth(currsizeA[ii] * 3 + 2) +
                           sramWriteBandwidth(currsizeA[ii] * 3 + 2);
    }
  } else {

    // hit part (chained)
    computeSramAccess +=
        sramReadBandwidth(fiberletlength * 3) * ((bufferedsizeA[ii] + 3) / 4);

    // miss part (need to check every uncached)

    if (fulltagA) {
      computeDramAccess +=
          (memoryBandwidthPE(3)) * ((long long)TJ + jjj - fullA);
      computeA +=
          (memoryBandwidthPE(3)) * (long long)((long long)TJ + jjj - fullA);
      AccessByte += 3 * ((long long)TJ + jjj - fullA);
    }
  }
}

void update_c_fiber(int jj) {

  for (int k1 = beginB[jj]; k1 < beginB[jj] + currsizeB[jj]; k1++) {
    tmpC[B[jj][k1]] = 1;
  }
}

// need to store: all the stored C coords,
// in order to calculate the current and next state occuppied buffer size
void updateCAccess(int ii) {

  // for buffered C:
  if ((Csize >= 100.0) && ((interorder == IKJ) || (interorder == KIJ))) {

    // check the delta buffer: how many new C elements (indicate how many
    // increase)
    int deltaC = 0;
    int oldsize = bufferedClen[ii];
    for (int k1 = TK; k1 < TK + ((ISDYNAMICK) ? dynk : kkk); k1++) {
      if (tmpC[k1]) {
        // the k1 is a new element
        if (bufferedC[ii].find(k1) == bufferedC[ii].end()) {
          deltaC++;

          bufferedC[ii].insert(k1);
        }
      }
    }
    // update the size
    bufferedClen[ii] += deltaC;

    Csizenow += deltaC;

    // overflow! need to offload
    if (Csizenow > Csize) {

      // *2 because
      // 1 is to write to dram
      // 1 is to read from dram at the final merge stage
      computeDramAccess += memoryBandwidthPE(Csizenow * 3);
      postDramAccess += memoryBandwidthPE(Csizenow * 3);
      // write C at compute stage; read C at post merge stage
      computeC += memoryBandwidthPE(Csizenow * 3);
      postC += memoryBandwidthPE(Csizenow * 3);
      AccessByte += Csizenow * 3;
      AccessByte += Csizenow * 3;

      computeSramAccess +=
          sramReadBandwidth(Csizenow * 3) + sramWriteBandwidth(Csizenow * 3);

      Csizenow = 0;

      for (int i = TI; i < TI + iii; i++) {
        bufferedC[i].clear();
        bufferedClen[i] = 0;
      }
    }
  } else {
    // following is for the stream C
    // update with compute
    int cntc = 0;

    for (int k1 = TK; k1 < TK + ((ISDYNAMICK) ? dynk : kkk); k1++) {
      if (tmpC[k1]) {
        cntc++;
      }
    }
    // write into DRAM during the computation
    computeDramAccess += memoryBandwidthPE(cntc * 3);
    computeC += memoryBandwidthPE(cntc * 3);
    AccessByte += cntc * 3;

    if (jjj != J) {
      // multiply 2 here if kkk != K
      // because need a extra inter-tile C merge and thus need an extra load
      postDramAccess += memoryBandwidthPE(cntc * 3);
      postC += memoryBandwidthPE(cntc * 3);
      AccessByte += cntc * 3;
    }

    computeSramAccess +=
        sramReadBandwidth(cntc * 3) + sramWriteBandwidth(cntc * 3);
  }
}

void get_B_fibers(int ii) {
  if (dataflow == Gust) {
    int tmpj = beginA[ii];
    int maxj = offsetarrayA[ii + 1] - offsetarrayA[ii];

    // tmpc = 0
    for (int k1 = TK; k1 < TK + kkk; k1++) {
      tmpC[k1] = 0;
    }

    while (tmpj < maxj && A[ii][tmpj] < TJ + ((ISDYNAMICJ) ? dynj : jjj)) {
      // coordinate of required B fiber
      int jj = A[ii][tmpj];

      get_B_fiber(jj, ii);

      computePE += currsizeB[jj];

      update_c_fiber(jj);

      tmpj++;

      if (offsetarrayA[ii] + tmpj >= offsetarrayA[ii + 1]) {
        break;
      }
    }

    // update A access
    if (consistent_A()) {

      // A[ii] is on the buffer

      // updated: add the interorder judge.
      // totally can't reuse if not **K
      if ((interorder == IJK || interorder == JIK)) {
        if (fulltagA == 0 || ii < fullA) {
          // hit
          computeSramAccess += sramReadBandwidth((tmpj - beginA[ii]) * 3);
        } else {
          computeDramAccess += memoryBandwidthPE((tmpj - beginA[ii]) * 3);
          computeA += memoryBandwidthPE((tmpj - beginA[ii]) * 3);
          AccessByte += (tmpj - beginA[ii]) * 3;

          computeSramAccess += sramReadBandwidth((tmpj - beginA[ii]) * 3) +
                               sramWriteBandwidth((tmpj - beginA[ii]) * 3);
        }
      } else {
        computeDramAccess += memoryBandwidthPE((tmpj - beginA[ii]) * 3);
        computeA += memoryBandwidthPE((tmpj - beginA[ii]) * 3);
        AccessByte += (tmpj - beginA[ii]) * 3;

        computeSramAccess += sramReadBandwidth((tmpj - beginA[ii]) * 3) +
                             sramWriteBandwidth((tmpj - beginA[ii]) * 3);
      }

    } else {

      // hit part (chained)
      computeSramAccess +=
          sramReadBandwidth(fiberletlength * 3) * ((bufferedsizeB[ii] + 3) / 4);

      // miss part (need to check every uncached)

      if (fulltagA) {
        computeDramAccess +=
            (memoryBandwidthPE(3)) * ((long long)TJ + jjj - fullA);
        computeA +=
            (memoryBandwidthPE(3)) * (long long)((long long)TJ + jjj - fullA);
        AccessByte += 3 * ((long long)TJ + jjj - fullA);
      }
    }

    // update C access
    updateCAccess(ii);
  }
}

void prefetchrow(int ii) {

  int tmpj = beginA[ii];
  int maxj = offsetarrayA[ii + 1] - offsetarrayA[ii];

  while (tmpj < maxj && A[ii][tmpj] < TJ + ((ISDYNAMICJ) ? dynj : jjj)) {
    // coordinate of required B fiber
    // in this prefetch: push the next access queue of jj a ii
    int jj = A[ii][tmpj];

    tmpj++;

    if (offsetarrayA[ii] + tmpj >= offsetarrayA[ii + 1]) {
      break;
    }
  }
}

/*
Perform calculation.
stream A/B/C: need to load/store from dram
buffered A/B/C: only load/store sram

3 bandwidth constraints:
dram; sram; compute

The inconsistent format has transformed while loading
So will not influence the calculate very much
Only in two place :
1) The iterate mode (compact or chained) (but the format is same)
2) The unbuffered (missed) access
*/
void calculate() {

  computePE = computeDramAccess = computeSramAccess = 0;

  // first only for IP&Gust
  // need to change to jj when OP

  // printf("dataflow %d\n", dataflow);
  if ((dataflow == Inner) || (dataflow == Gust)) {

    if (dataflow == Gust) {

      int prefetchsize = 8;

      for (int ii = 0; ii < iii; ii++) {
        if (TI + ii > I)
          break;

        // get O(J) corresponding B (different from Gust and Inner)
        // different from other dataflow (is A-dependent)
        get_B_fibers(TI + ii);
      }
    }

    if (dataflow == Inner) {

      // update B here
      for (int k = TK; k < TK + ((ISDYNAMICK) ? dynk : kkk); k++) {
        get_B_fiber_col_iii(k, iii);
      }

      for (int ii = 0; ii < iii; ii++) {

        int cnew = 0, cnow = 0;

        // update A
        // get A
        get_A_fiber(TI + ii);

        // update C

        int tmpj = beginA[TI + ii];
        int maxj = offsetarrayA[TI + ii + 1] - offsetarrayA[TI + ii];

        // tmpc = 0
        for (int k1 = TK; k1 < TK + ((ISDYNAMICK) ? dynk : kkk); k1++) {
          tmpC[k1] = 0;
        }

        while (tmpj < maxj && A[TI + ii][tmpj] < TJ + jjj) {
          // coordinate of required B fiber
          int jj = A[TI + ii][tmpj];

          update_c_fiber(jj);

          tmpj++;

          if (offsetarrayA[TI + ii] + tmpj >= offsetarrayA[TI + ii + 1]) {
            break;
          }
        }

        updateCAccess(TI + ii);

        continue;

        for (int kk = 0; kk < kkk; kk++) {
          // get B
          get_B_fiber_col(TK + kk);

          // calculate if there is an intersect between A & B
          int tmpja = beginA[TI + ii],
              maxja = offsetarrayA[TI + ii + 1] - offsetarrayA[TI + ii];
          int tmpjb = beginBc[TK + kk],
              maxjb = offsetarrayBc[TK + kk + 1] - offsetarrayBc[TK + kk];

          bool findflag = 0;

          while (tmpja < maxja && A[TI + ii][tmpja] < TJ + jjj) {

            int findj = A[TI + ii][tmpja];

            while (tmpjb < maxjb && Bc[TK + kk][tmpjb] < TJ + jjj) {

              if (Bc[TK + kk][tmpjb] >= findj) {
                break;
              }
              tmpjb++;
            }

            // need to assure that b exit first!!
            // find the same index!
            if (tmpjb < maxjb && Bc[TK + kk][tmpjb] == findj) {
              findflag = 1;
              break;
            }

            tmpja++;
          }

          // get a C[TI+ii][TK+kk]
          if (findflag) {
            // if is the new index all update the old index
            cnow++;

          } else {
            // zero C; don't need extra operation
          }
        }

        computeDramAccess += memoryBandwidthPE(cnow * 3);
        computeC += memoryBandwidthPE(cnow * 3);
        AccessByte += cnow * 3;
      }
    }
  }
  if ((dataflow == Outer)) {

    for (int jj = 0; jj < jjj; jj++) {
      get_A_fiber_col(TJ + jj);

      // use a dumb jj
      get_B_fiber(TJ + jj, jj);
    }

    fulltagC = 0;
    Csizenow = 0;

    for (int ii = TI; ii < TI + iii; ii++) {
      int tmpj = beginA[ii];
      int maxj = offsetarrayA[ii + 1] - offsetarrayA[ii];

      // tmpc = 0
      for (int k1 = TK; k1 < TK + kkk; k1++) {
        tmpC[k1] = 0;
      }

      while (tmpj < maxj && A[ii][tmpj] < TJ + jjj) {
        // coordinate of required B fiber
        int jj = A[ii][tmpj];

        computePE += currsizeB[jj];

        update_c_fiber(jj);

        tmpj++;

        if (offsetarrayA[ii] + tmpj >= offsetarrayA[ii + 1]) {
          break;
        }
      }

      int cntc = 0;
      for (int k1 = TK; k1 < TK + kkk; k1++) {
        if (tmpC[k1]) {
          cntc++;
        }
      }

      if (!fulltagC) {
        if (Csizenow + cntc * 3 >= Csize) {
          fulltagC = 1;
          Csizenow += cntc * 3;
        } else {
          Csizenow += cntc * 3;
          // can be stored in sram!
          computeSramAccess +=
              sramReadBandwidth(cntc * 3 + 2) * (jjj / mergecnt);
        }
      } else {
        computeDramAccess += memoryBandwidthPE(cntc * 3 + 2) * (jjj / mergecnt);
        computeC += memoryBandwidthPE(cntc * 3 + 2) * (jjj / mergecnt);
        AccessByte += (cntc * 3 + 2) * (jjj / mergecnt);
      }
    }
  }

  totalCycle += max(computePE / PEcnt, max(computeDramAccess / PEcnt,
                                           computeSramAccess / sramBank));
  calCycle += max(computePE / PEcnt,
                  max(computeDramAccess / PEcnt, computeSramAccess / sramBank));

  totalSram += computeSramAccess / sramBank;
  totalDram += computeDramAccess / PEcnt;
  totalPE += computePE / PEcnt;
}

void post_calculate_store() {

  if (checkReuseC()) {
    return;
  }
}

int minBlock = 2000;

int getkbound() {
  return minBlock;
  return max(23, min(minBlock, K / minBlock));
}

int getjbound() {
  return minBlock;
  return max(23, min(minBlock, J / minBlock));
}

int getibound() {
  return minBlock;
  return max(23, min(minBlock, I / minBlock));
}

void configPartial(float partialA, float partialB, float partialC) {

  Asize = cachesize * partialA;
  Bsize = cachesize * partialB;
  Csize = cachesize * partialC;

  // no C reuse -> only A/B
  if ((interorder != JIK) && (interorder != JKI)) {
  }
}

void reinitialize() {

  // reinitialize statistics
  totalCycle = 0;
  preCycle = calCycle = postCycle = 0;
  computeA = computeB = computeC = 0;
  totalA = totalB = totalC = 0;
  preA = preB = postC = 0;
  totalSram = totalDram = totalPE = 0;
  TI = TJ = TK = 0;
  AccessByte = 0;

  postDramAccess = postSramAccess = 0;

  if (ISCACHE) {
    initializeCacheValid();
  }

  // reinitialize buffer c
  Csizenow = 0;
  for (int i = 0; i < I; i++) {
    bufferedC[i].clear();
    bufferedClen[i] = 0;
  }

  // reinitialize management dtaa
  reinitialize_beginA();
  reinitialize_beginAc();
  reinitialize_beginB();
  reinitialize_beginBc();
  reinitialize_beginC();
}

int getcntc(int ii) {
  int tmpj = beginA[ii];
  int maxj = offsetarrayA[ii + 1] - offsetarrayA[ii];

  for (int k1 = 0; k1 < K; k1++) {
    tmpC[k1] = 0;
  }

  while (tmpj < maxj && A[ii][tmpj] < J) {
    // coordinate of required B fiber
    int jj = A[ii][tmpj];

    update_c_fiber(jj);

    tmpj++;

    if (offsetarrayA[ii] + tmpj >= offsetarrayA[ii + 1]) {
      break;
    }
  }

  int cntc = 0;

  for (int k1 = 0; k1 < K; k1++) {
    if (tmpC[k1]) {
      cntc++;
    }
  }

  return cntc;
}

// merge the partial C generated at each tile
// Read DRAM: amount of sum of all partialC offload
// Write DRAM: amount of nnzC
void postTileMerge() {
  // another way to realize this is to add once read each time when we write C
  // already added in the C writing position for gust and IP

  // 除了jjj != J 的情况，还需要加上buffer不够的情况： 前面offload过时！
  if (dataflow == Gust) {
    if (jjj != J) {

      for (int ii = 0; ii < I; ii++) {

        int cntc = getcntc(ii);

        computeDramAccess += memoryBandwidthPE(cntc * 3);
        postC += memoryBandwidthPE(cntc * 3);
        AccessByte += cntc * 3;
      }
    }
  }

  // calculate the inter-cost of outer
  if ((dataflow == Outer)) {

    for (int ii = 0; ii < I; ii++) {

      int cntc = getcntc(ii);

      postDramAccess += memoryBandwidthPE(cntc * 3) * (ttj);
      postC += memoryBandwidthPE(cntc * 3) * (ttj);
      AccessByte += cntc * 3 * ttj;
    }
    //   analyze_statistics();
  }

  postSramAccess /= sramBank;
  postDramAccess /= PEcnt;

  totalCycle += max(postDramAccess, postSramAccess);
  postCycle += max(postDramAccess, postSramAccess);
}

void run() {
  reinitialize();

  if (iii > I)
    iii = I;
  if (jjj > J)
    jjj = J;
  if (kkk > K)
    kkk = K;

  // initialize the finetune tile to the selected tile
  initialTileSize();

  // initialize Tcnt before each tile running
  reinitializecnt();

  // reinitialize**: TI/TJ/TK to 0;  put beginA/B to 0
  reinitialize_outer();
  do {

    reinitialize_mid();

    do {

      reinitialize_inner();
      do {

        initialDynamicTile();

        // need to initialize the cache each time change the tile
        if (ISCACHE) {
          initializeCacheValid();
        }

        // force for just test
        //  ensure the begin is right (with TI,TJ,TK)
        // forcebeginA();
        // forcebeginB();

        pre_calculate_load();

        updateBlockA();
        updateBlockB();

        calculate();

        post_calculate_store();

        // adddyn
        // dynamically update the estimate tile here.
        // don't change the actual TI/TJ/TK(iii, jjj, kkk in the code) here,
        // only change the _TI/_TJ/_TK(_iii, _jjj, _kkk in the code) here update
        // the _TI/_TJ/_TK to TI/TJ/TK when actually updating TI/TJ/TK (decide
        // by the interorder) this means will change immediately when the update
        // dimension is the inner-most dimension. here we use Tcnt00, Tcnt01,
        // Tcnt10, Tcnt11 to udpate _TI/_TJ/_TK

        update_T();

      } // update TI/TJ/TK; update beginA/B/C if TI/J/K don't overflow
      while (iterate_inner_loop());

    } while (iterate_mid_loop());

  } while (iterate_outer_loop());

  postTileMerge();

  analyze_statistics();
}

void runTile(bool isest, int iii, int jjj, int kkk, long long tti,
             long long ttj, long long ttk, long long SmallestTile) {

  // only prunning in the estimation mode
  if (isest) {
    long long mosttotalonchip = (nzB / (tti * ttj * ttk)) * 3LL;

    if (mosttotalonchip * 100LL < cachesize) {
      return;
    }

    // prunning
    if (((long long)jjj * kkk) * 4 < SmallestTile) {
      return;
    }
  }

  if (PartialConfig == 1) {
    // 100% B
    configPartial(0, 1, 0);
    interorder = InterOrder(2); // JKI
  }
  if (PartialConfig == 2) {
    // 50%B + 50%A
    configPartial(0.5, 0.5, 0);
    interorder = InterOrder(0); // IJK
  }
  if (PartialConfig == 3) {
    // 50%B + 50%C
    configPartial(0, 0.5, 0.5);
    interorder = InterOrder(1); // IKJ
  }
  // ********* Two Dyanmic Config ***********
  if (PartialConfig == 4) {
    // dynamic : 100%B
    ISDYNAMICJ = 1;
    configPartial(0, 1, 0);
    interorder = InterOrder(1); // IKJ
  }
  if (PartialConfig == 5) {
    // dynamic: 50%B 50%C
    ISDYNAMICJ = 1;
    configPartial(0, 0.5, 0.5);
    interorder = InterOrder(1); // IKJ
  }
  if (PartialConfig == 6) {
    configPartial(0, 1, 0);
    interorder = InterOrder(1); // IKJ
  }

  int oldsize = 0;

  if (!isest) {
    //  cout <<  printInterOrder[interorder];
    //  printf("   %d %d %d %d    !!  %d %d %d\n", iii, jjj, kkk, PartialConfig,
    //  cachesize, SET, SETLOG);
  }
  fflush(stdout);

  // run();
  if (isest) {
    gustest(0);
  } else {
    run();
  }

  // }
}
