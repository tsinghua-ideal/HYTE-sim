#include "estimation.h"
#include "headers.h"

// this parameters are for the dynamic tile fine-tuning
// _iii/_jjj/_kkk represents the current adjust tile size (may haven't
// implemented yet) _iii/_jjj/_kkk initialized to the selected iii/jjj/kkk
int _iii, _jjj, _kkk;
int _tti, _ttj, _ttk;
// Tcnt are for counting non-zeros in each subtile
// initialize to 0 at each round
int Tcnt[2][2];
// store the now sum of sizejk in ttjsum*ttksum tiles
// store all 7 types
long long sizejksum[10];
// ttjsum, ttksum means the now ttj*ttk
// initialize to zero if jjj*kkk changes,
int tilesum;

void initialTileSize() {
  _iii = iii;
  _tti = tti;
  _jjj = jjj;
  _ttj = ttj;
  _kkk = kkk;
  _ttk = ttk;
}

// adddyn
void reinitializecnt() {
  memset(Tcnt, 0, sizeof(Tcnt));
  memset(sizejksum, 0, sizeof(sizejksum));
  tilesum = 0;
}

void dynamicupdatei() {
  if (_iii != iii) {
    iii = _iii;
    tti = _tti;
    puts("*****************************  update i");
    reinitializecnt();
  }
}

void dynamicupdatej() {
  if (_jjj != jjj) {
    jjj = _jjj;
    ttj = _ttj;
    puts("*****************************  update j");
    reinitializecnt();
  }
}

void dynamicupdatek() {
  if (_kkk != kkk) {
    kkk = _kkk;
    ttk = _ttk;
    puts("*****************************  update k");
    fflush(stdout);
    reinitializecnt();
  }
}

void initialDynamicTile() {
  tilesum++;
  memset(Tcnt, 0, sizeof(Tcnt));
}

void updateDynamicTile(int _tj, int t0, int t1) {
  Tcnt[_tj][0] += t0;
  Tcnt[_tj][1] += t1;
}

// adddyn
// finetune the tile size after each tile calculating it's Tcnt.
// similar to the static finetune stage
// use _iii/_jjj/_kkk represent the tile after finetune.
void update_T() {

  int oldiii = iii, oldjjj = jjj, oldkkk = kkk;
  int oldtti = tti, oldttj = ttj, oldttk = ttk;
  int iii2 = (iii + 1) / 2, jjj2 = (jjj + 1) / 2, kkk2 = (kkk + 1) / 2;
  int tti2 = tti * 2, ttj2 = ttj * 2, ttk2 = ttk * 2;
  int estsum = 0;

  // jjj+kkk type 0
  // Tcnt need to clear and recalculate at each round, sizejksum and tilesum not
  int sizejk = (Tcnt[0][0] + Tcnt[0][1] + Tcnt[1][0] + Tcnt[1][1]) * 3 + oldjjj;
  sizejksum[0] += min(sizejk, Bsize);
  iii = oldiii;
  jjj = oldjjj;
  kkk = oldkkk;
  tti = oldtti;
  ttj = oldttj;
  ttk = oldttk;
  //  printf("!!! sizejksum:%lld  tilesum:%d\n", sizejksum[0], tilesum);
  fflush(stdout);
  long long mintime = gustest(sizejksum[0] / (tilesum));
  int mintype = 0;

  // jjj/2 + kkk type1
  sizejk = (Tcnt[0][0] + Tcnt[0][1]) * 3 + jjj2;
  sizejksum[1] += min(sizejk, Bsize);
  sizejk = (Tcnt[1][0] + Tcnt[1][1]) * 3 + jjj2;
  sizejksum[1] += min(sizejk, Bsize);

  iii = oldiii;
  jjj = jjj2;
  kkk = oldkkk;
  tti = oldtti;
  ttj = ttj2;
  ttk = oldttk;
  long long tmptime = gustest(sizejksum[1] / (tilesum * 2));
  tmptime *= 1.1;
  if (tmptime < mintime) {
    mintime = tmptime;
    mintype = 1;
  }

  // jjj + kkk/2 type2
  sizejk = (Tcnt[0][0] + Tcnt[1][0]) * 3 + oldjjj;
  sizejksum[2] += min(sizejk, Bsize);
  sizejk = (Tcnt[0][1] + Tcnt[1][1]) * 3 + oldjjj;
  sizejksum[2] += min(sizejk, Bsize);

  iii = oldiii;
  jjj = oldjjj;
  kkk = kkk2;
  tti = oldtti;
  ttj = oldttj;
  ttk = ttk2;
  tmptime = gustest(sizejksum[2] / (tilesum * 2));
  tmptime *= 1.1;
  if (tmptime < mintime) {
    mintime = tmptime;
    mintype = 2;
  }

  // jjj/2 + kkk/2 type3
  sizejk = (Tcnt[0][0]) * 3 + jjj2;
  sizejksum[3] += min(sizejk, Bsize);
  sizejk = (Tcnt[0][1]) * 3 + jjj2;
  sizejksum[3] += min(sizejk, Bsize);
  sizejk = (Tcnt[1][0]) * 3 + jjj2;
  sizejksum[3] += min(sizejk, Bsize);
  sizejk = (Tcnt[1][1]) * 3 + jjj2;
  sizejksum[3] += min(sizejk, Bsize);

  iii = oldiii;
  jjj = jjj2;
  kkk = kkk2;
  tti = oldtti;
  ttj = ttj2;
  ttk = ttk2;
  tmptime = gustest(sizejksum[3] / (tilesum * 4));
  tmptime *= 1.1;
  if (tmptime < mintime) {
    mintime = tmptime;
    mintype = 3;
  }

  // only can increase when *2 <= I/J/K, otherwise overflow
  // jjj*2 + kkk type4
  if (oldjjj * 2 <= J) {
    sizejk = (Tcnt[0][0] + Tcnt[0][1] + Tcnt[1][0] + Tcnt[1][1]) * 3 * 2 +
             oldjjj * 2;
    sizejksum[4] += min(sizejk, Bsize);
    iii = oldiii;
    jjj = oldjjj * 2;
    kkk = oldkkk;
    tti = oldtti;
    ttj = oldttj / 2;
    ttk = oldttk;
    tmptime = gustest(sizejksum[4] / (tilesum));
    tmptime *= 1.1;
    if (tmptime < mintime) {
      mintime = tmptime;
      mintype = 4;
    }
  }

  // jjj + kkk*2 type5
  if (oldkkk * 2 <= K) {
    sizejk =
        (Tcnt[0][0] + Tcnt[0][1] + Tcnt[1][0] + Tcnt[1][1]) * 3 * 2 + oldjjj;
    sizejksum[5] += min(sizejk, Bsize);
    iii = oldiii;
    jjj = oldjjj;
    kkk = oldkkk * 2;
    tti = oldtti;
    ttj = oldttj;
    ttk = oldttk / 2;
    tmptime = gustest(sizejksum[5] / (tilesum));
    tmptime *= 1.1;
    if (tmptime < mintime) {
      mintime = tmptime;
      mintype = 5;
    }
  }

  // jjj*2 + kkk*2 type6
  if ((oldjjj * 2 <= J) && (oldkkk * 2 <= K)) {
    sizejk = (Tcnt[0][0] + Tcnt[0][1] + Tcnt[1][0] + Tcnt[1][1]) * 3 * 4 +
             oldjjj * 2;
    sizejksum[6] += min(sizejk, Bsize);
    iii = oldiii;
    jjj = oldjjj * 2;
    kkk = oldkkk * 2;
    tti = oldtti;
    ttj = oldttj / 2;
    ttk = oldttk / 2;
    tmptime = gustest(sizejksum[6] / (tilesum));
    tmptime *= 1.1;
    if (tmptime < mintime) {
      mintime = tmptime;
      mintype = 6;
    }
  }

  // jjj/2 + kkk*2  type7
  if (oldkkk * 2 <= K) {
    sizejk = (Tcnt[0][0] + Tcnt[0][1]) * 3 * 2 + jjj2;
    sizejksum[7] += min(sizejk, Bsize);

    sizejk = (Tcnt[1][0] + Tcnt[1][1]) * 3 * 2 + jjj2;
    sizejksum[7] += min(sizejk, Bsize);

    iii = oldiii;
    jjj = jjj2;
    kkk = oldkkk * 2;
    tti = oldtti;
    ttj = ttj2;
    ttk = oldttk / 2;
    tmptime = gustest(sizejksum[7] / (tilesum * 2));
    tmptime *= 1.1;
    if (tmptime < mintime) {
      mintime = tmptime;
      mintype = 7;
    }
  }

  // jjj*2 + kkk/2 type8
  if (oldjjj * 2 <= J) {
    sizejk = (Tcnt[0][0] + Tcnt[1][0]) * 3 * 2 + jjj;
    sizejksum[8] += min(sizejk, Bsize);

    sizejk = (Tcnt[0][1] + Tcnt[1][1]) * 3 * 2 + jjj;
    sizejksum[8] += min(sizejk, Bsize);

    iii = oldiii;
    jjj = oldjjj * 2;
    kkk = kkk2;
    tti = oldtti;
    ttj = oldttj / 2;
    ttk = ttk2;
    tmptime = gustest(sizejksum[8] / (tilesum * 2));
    tmptime *= 1.1;
    if (tmptime < mintime) {
      mintime = tmptime;
      mintype = 8;
    }
  }

  if (mintype == 0) {
    _iii = oldiii;
    _jjj = oldjjj;
    _kkk = oldkkk;
    _tti = oldtti;
    _ttj = oldttj;
    _ttk = oldttk;
  } else {
    if (mintype == 1) {
      _iii = oldiii;
      _kkk = oldkkk;
      _tti = oldtti;
      _ttk = oldttk;
      _jjj = jjj2;
      _ttj = ttj2;
    }
    if (mintype == 2) {
      _iii = oldiii;
      _jjj = oldjjj;
      _tti = oldtti;
      _ttj = oldttj;
      _kkk = kkk2;
      _ttk = ttk2;
    }
    if (mintype == 3) {
      _iii = oldiii;
      _tti = oldtti;
      _jjj = jjj2;
      _ttj = ttj2;
      _kkk = kkk2;
      _ttk = ttk2;
    }
    if (mintype == 4) {
      _iii = oldiii;
      _kkk = oldkkk;
      _tti = oldtti;
      _ttk = oldttk;
      _jjj = oldjjj * 2;
      _ttj = oldttj / 2;
    }
    if (mintype == 5) {
      _iii = oldiii;
      _jjj = oldjjj;
      _tti = oldtti;
      _ttj = oldttj;
      _kkk = oldkkk * 2;
      _ttk = oldttk / 2;
    }
    if (mintype == 6) {
      _iii = oldiii;
      _tti = oldtti;
      _jjj = oldjjj * 2;
      _ttj = oldttj / 2;
      _kkk = oldkkk * 2;
      _ttk = oldttk / 2;
    }
    if (mintype == 7) {
      _iii = oldiii;
      _kkk = oldkkk * 2;
      _tti = oldtti;
      _ttk = oldttk / 2;
      _jjj = jjj2;
      _ttj = ttj2;
    }
    if (mintype == 8) {
      _iii = oldiii;
      _jjj = oldjjj * 2;
      _tti = oldtti;
      _ttj = oldttj / 2;
      _kkk = kkk2;
      _ttk = ttk2;
    }
  }

  // don't change the actual tiling here, (only change _iii/_jjj/_kkk)
  // change until next time inter-tile update of i/j/k.
  iii = oldiii;
  jjj = oldjjj;
  kkk = oldkkk;
  tti = oldtti;
  ttj = oldttj;
  ttk = oldttk;
}