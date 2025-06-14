#include "cache.h"
#include "estimation.h"
#include "headers.h"
#include "json.hpp"
#include "simulator.h"
#include <fstream>

using json = nlohmann::json;

int main(int argc, char *argv[]) {

  std::string matrix_name1 = argv[1];
  std::string matrix_name2 = argv[2];
  std::string config_file = argv[3];

  std::ifstream file(config_file);
  if (!file.is_open()) {
    std::cerr << "Error opening config file." << std::endl;
    return 1;
  }

  json config;
  file >> config;

  dataflow = static_cast<DataFlow>(config["dataflow"].get<int>());
  format = static_cast<Format>(config["format"].get<int>());
  int transpose = config["transpose"].get<int>();
  minBlock = config["minBlock"].get<int>();
  float tmpsram = config["cachesize"].get<float>();
  cachesize = tmpsram * 262144;
  float tmpbandw = config["memorybandwidth"].get<float>();
  HBMbandwidth = (tmpbandw / 4.0) * 0.6;
  int tmpPE = config["PEcnt"].get<int>();
  PEcnt = tmpPE;
  mergecnt = tmpPE;
  HBMbandwidthperPE = HBMbandwidth / PEcnt;
  int tmpbank = config["srambank"].get<int>();
  sramBank = tmpbank;
  ISCACHE = config["iscache"].get<int>();
  cacheScheme = config["cacheScheme"].get<int>();
  int baselinetest = config["baselinetest"].get<int>();
  std::string output_dir = config["outputDir"].get<std::string>();

  if (!freopen(
          ("largedata/" + matrix_name1 + "/" + matrix_name1 + ".mtx").c_str(),
          "r", stdin)) {
    if (!freopen(("data/" + matrix_name1 + ".mtx").c_str(), "r", stdin)) {
      if (!freopen(("dense/" + matrix_name1 + ".mtx").c_str(), "r", stdin)) {
        if (!freopen(("bfs/" + matrix_name1 + ".mtx").c_str(), "r", stdin)) {
          std::cerr << "Error opening input file." << std::endl;
          return 1;
        }
      }
    }
  }

  if (!freopen((output_dir + (ISCACHE ? "C" : "_") + printDataFlow[dataflow] +
                (baselinetest ? "Base_" : "HYTE_") + std::to_string(tmpsram) +
                "MB_" + std::to_string(tmpbandw) + "GBs_" +
                std::to_string(tmpPE) + "PEs_" + std::to_string(tmpbank) +
                "sbanks_" + "_" + matrix_name1 + "_" + matrix_name1 + "_" +
                printFormat[format] + "_" + (transpose ? "1" : "0") + ".txt")
                   .c_str(),
               "w", stdout)) {
    std::cerr << "Error opening output folder." << std::endl;
    return 1;
  }

  const int BUFFERSIZE = 512;
  char readbuffer[BUFFERSIZE];

  // read and ignore annotation '%' lines
  while (std::cin.getline(readbuffer, BUFFERSIZE)) {
    if (readbuffer[0] != '%') {
      break;
    }
  }

  std::sscanf(readbuffer, "%d%d%d", &N, &M, &nzA);

  if (transpose) {
    swap(N, M);
  }

  printf("Matrix A: %d x %d, number of non-zeros = %d\n", N, M, nzA);
  fflush(stdout);

  // samplek = 100;
  // samplep = 0.1;

  samplek = sqrt(N);
  samplep = 1.0 / samplek;

  // samplek = 256;
  // samplep = 0.0039;

  if (!baselinetest) {
    printf("samplek:%lf  samplep:%lf\n", samplek, samplep);
  }

  I = N;
  J = M;

  string input;

  fflush(stdout);

  for (int i = 1; i <= nzA; i++) {

    std::getline(std::cin, input);

    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
      tokens.push_back(token);
    }

    int xx, yy;
    double zz, lala;

    if (tokens.size() == 2) {

      std::istringstream(tokens[0]) >> xx;
      std::istringstream(tokens[1]) >> yy;
      // std::cout << "values: " << xx << ", " << yy << std::endl;
    } else if (tokens.size() == 3) {

      std::istringstream(tokens[0]) >> xx;
      std::istringstream(tokens[1]) >> yy;
      std::istringstream(tokens[2]) >> zz;
      // std::cout << "values: " << xx << ", " << yy << ", " << zz << std::endl;
    } else if (tokens.size() == 4) {

      std::istringstream(tokens[0]) >> xx;
      std::istringstream(tokens[1]) >> yy;
      std::istringstream(tokens[2]) >> zz;
      std::istringstream(tokens[2]) >> lala;
      // std::cout << "values: " << xx << ", " << yy << ", " << zz << std::endl;
    } else {

      std::cout << "Format Incorrect! " << std::endl;
      cout << tokens.size() << endl;
      cout << i << endl << input << endl;
      return 0;
    }

    if (transpose) {
      Ac[xx].push_back(yy);
      A[yy].push_back(xx);
    } else {
      A[xx].push_back(yy);
      Ac[yy].push_back(xx);
    }
  }

  for (int i = 0; i <= I; i++) {
    sort(A[i].begin(), A[i].end());
  }
  for (int j = 0; j <= J; j++) {
    sort(Ac[j].begin(), Ac[j].end());
  }

  for (int i = 1; i <= I + 2; i++) {
    offsetarrayA[i] = offsetarrayA[i - 1] + A[i - 1].size();
  }
  for (int i = 1; i <= J + 2; i++) {
    offsetarrayAc[i] = offsetarrayAc[i - 1] + Ac[i - 1].size();
  }

  initsample();

  sampleA();

  fclose(stdin);

  /////////////////////////// input B /////////////////////////////////

  cin.clear();
  //  freopen(("data/" + matrix_name2+".mtx").c_str(), "r", stdin);
  if (!freopen(
          ("largedata/" + matrix_name2 + "/" + matrix_name2 + ".mtx").c_str(),
          "r", stdin)) {
    if (!freopen(("data/" + matrix_name2 + ".mtx").c_str(), "r", stdin)) {
      if (!freopen(("dense/" + matrix_name2 + ".mtx").c_str(), "r", stdin)) {
        if (!freopen(("bfs/" + matrix_name2 + ".mtx").c_str(), "r", stdin)) {
          std::cerr << "Error opening input file." << std::endl;
          return 1;
        }
      }
    }
  }

  // read and ignore annotation '%' lines
  while (std::cin.getline(readbuffer, BUFFERSIZE)) {
    if (readbuffer[0] != '%') {
      break;
    }
  }

  std::sscanf(readbuffer, "%d%d%d", &N, &M, &nzB);

  printf("Matrix B: %d x %d, number of non-zeros = %d\n", N, M, nzB);
  fflush(stdout);

  if (N != M)
    transpose ^= 1; // when transposeA = 0 -> transposeB = 1; when tranposeA=
                    // 1-> transposeB = 0

  if (transpose) {
    swap(N, M);
  }

  printf("transpose: %d\n", transpose);

  if (J != N) {
    printf("Mismatch J!\n");
    return 0;
  }

  K = M;

  // std::getline(std::cin, input);

  for (int i = 1; i <= nzB; i++) {

    std::getline(std::cin, input);

    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;

    // 将输入行分割为单词
    while (iss >> token) {
      tokens.push_back(token);
    }

    int xx, yy;
    double zz, lala;

    if (tokens.size() == 2) {

      std::istringstream(tokens[0]) >> xx;
      std::istringstream(tokens[1]) >> yy;
      // std::cout << "values: " << xx << ", " << yy << std::endl;
    } else if (tokens.size() == 3) {

      std::istringstream(tokens[0]) >> xx;
      std::istringstream(tokens[1]) >> yy;
      std::istringstream(tokens[2]) >> zz;
      // std::cout << "values: " << xx << ", " << yy << ", " << zz << std::endl;
    } else if (tokens.size() == 4) {

      std::istringstream(tokens[0]) >> xx;
      std::istringstream(tokens[1]) >> yy;
      std::istringstream(tokens[2]) >> zz;
      std::istringstream(tokens[2]) >> lala;
      // std::cout << "values: " << xx << ", " << yy << ", " << zz << std::endl;
    } else {
      std::cout << "Format Incorrect! " << std::endl;
      cout << i << endl << input << endl;
      return 0;
    }

    if (transpose) {
      Bc[xx].push_back(yy);
      B[yy].push_back(xx);
    } else {
      B[xx].push_back(yy);
      Bc[yy].push_back(xx);
    }
  }

  // cout << N << endl<<M <<endl<< nz << endl << nz/N <<endl;

  for (int j = 0; j <= J; j++) {
    sort(B[j].begin(), B[j].end());
  }
  for (int k = 0; k <= K; k++) {
    sort(Bc[k].begin(), Bc[k].end());
  }

  for (int j = 1; j <= J + 2; j++) {
    offsetarrayB[j] = offsetarrayB[j - 1] + B[j - 1].size();
  }
  for (int k = 1; k <= K + 2; k++) {
    offsetarrayBc[k] = offsetarrayBc[k - 1] + Bc[k - 1].size();
  }

  sampleB();

  /******************Config************************************/

  // notation of J and K in the code is swapped as in the paper
  // use the paper's notation as print output
  printf("I = %d, K = %d, J = %d\n", I, K, J);
  /************************************************************/

  if (baselinetest) {

    configPartial(0.33, 0.33, 0.33);
    interorder = IJK;

    //***********    Tailors
    //****************************************************
    printf("\n\nTailors *************************************\n");
    int pbound = K;
    int leftbound = 0; // (leftbound, k] is the current window
    int sumnow = 0;

    int Bbound = Bsize;

    for (int k = 0; k < K; k++) {
      sumnow += Bc[k].size() * 3 + 1;

      while (sumnow > Bbound && leftbound < k) {
        pbound = min(pbound, k - leftbound);
        leftbound++;
        sumnow -= Bc[leftbound].size() * 3 + 1;
      }
    }

    // printf("########## pbound = %d\n", pbound);
    // fflush(stdout);

    ISDYNAMICJ = 0;
    ISDYNAMICK = 0;
    ISDYNAMICI = 0;
    iii = I;
    jjj = J;
    kkk = pbound;
    tti = 1;
    ttj = 1;
    ttk = (K + pbound - 1) / pbound;

    reinitialize();

    double multiples[21] = {1.0, 1.0625, 1.125, 1.25, 1.5, 2,   3,   5,
                            9,   17,     33,    65,   129, 257, 513, 1025};

    double tilesize;

    for (int ib = 1; ib < 15; ib++) {

      iii = I;
      jjj = J;
      kkk = pbound * multiples[ib];

      //     printf("&& %d %d %d\n", iii, jjj, kkk);
      fflush(stdout);

      int mistilecnt = 0;
      int totaltilecnt = 0;

      for (int tktmp = 0; tktmp < K; tktmp += kkk) {
        int sumtile = 0;

        for (int kktmp = tktmp; kktmp < tktmp + kkk; kktmp++) {
          sumtile += Bc[kktmp].size() * 3 + 1;
        }

        if (sumtile > Bbound) {
          mistilecnt++;
        }
        totaltilecnt++;
      }

      //    printf("tile overflow rate is %lf\n", (double)mistilecnt /
      //    totaltilecnt);

      tilesize = K / (double)kkk;

      // add this sentence when test Tailor. (to run faster)
      if (totaltilecnt == 0)
        break;
      if (10 * mistilecnt > totaltilecnt)
        break;
      if (((double)mistilecnt / totaltilecnt) > 0.1)
        break;
    }

    reinitialize();

    configPartial(0.45, 0.5, 0.05);

    printf("Choose Tiling: TI = %d, TK = %d, TJ = %d\n", iii, jjj, kkk);

    runTile(0, iii, jjj, kkk, tti, ttk, ttj, 0);

    //***************  DRT
    //**************************************************************

    printf("\n\nDRT ************************************\n");
    configPartial(0.05, 0.45, 0.5);
    interorder = JKI;

    double tt = sqrt(tilesize);

    iii = I;
    jjj = (int)(J / tt);
    kkk = (int)(K / tt);
    tti = 1;
    ttj = (J + jjj - 1) / jjj;
    ttk = (K + kkk - 1) / kkk;

    printf("Choose Tiling: TI = %d, TK = %d, TJ = %d\n", iii, jjj, kkk);

    fflush(stdout);

    configPartial(0.45, 0.4, 0.05);

    reinitialize();

    runTile(0, iii, jjj, kkk, tti, ttk, ttj, 0);

    //***************Harp
    //*************************************************************

    printf("\n\nHarp ************************************\n");

    configPartial(0.045, 0.91, 0.045);
    interorder = JKI;

    // change: Harp only tile I not J！

    iii = (int)(I / tilesize);
    jjj = J;
    kkk = K;
    tti = (I + iii - 1) / iii;
    ttj = 1;
    ttk = 1;

    printf("Choose Tiling: TI = %d, TK = %d, TJ = %d\n", iii, jjj, kkk);

    reinitialize();

    configPartial(0.05, 0.9, 0.05);

    runTile(0, iii, jjj, kkk, tti, ttk, ttj, 0);

    return 0;
  }

  //***************  HYTE
  //********************************************************

  printf("\nHYTE ******************************\n");

  getParameterSample();

  configPartial(0.05, 0.5, 0.45);
  // also calculate the pbound here as the bound of tile search
  int pbound = K;
  int leftbound = 0; // (leftbound, k] is the current window
  int sumnow = 0;

  int Bbound = Bsize;

  for (int k = 0; k < K; k++) {
    sumnow += Bc[k].size() * 3 + 1;

    while (sumnow > Bbound && leftbound < k) {
      pbound = min(pbound, k - leftbound);
      leftbound++;
      sumnow -= Bc[leftbound].size() * 3 + 1;
    }
  }

  long long SmallestTile = ((long long)pbound) * J;

  // printf("########## pbound = %d  SmallestTile = %lld\n", pbound,
  // SmallestTile); fflush(stdout);

  // decrease jbound and kbound
  // don't need ibound rightnow (in Gust&IP) (don't need tiling I at all)

  int kbound = getkbound();
  int jbound = getjbound();
  int ibound = getibound();

  // use IKJ to eval dynamic

  auto time4 = std::chrono::high_resolution_clock::now();

  int interone;
  if (dataflow == Gust || dataflow == Outer)
    interone = 1;
  if (dataflow == Inner)
    interone = 0;
  for (int tmpinter = interone; tmpinter <= interone; tmpinter++) {

    interorder = InterOrder(tmpinter);

    tti = 1, ttk = 1, ttj = 1;
    for (iii = I, tti = 1; iii >= ibound; iii = (iii + 1) / 2, tti *= 2) {
      // first only change J, then only change K, finally change both J & K

      // only change J
      kkk = K;
      ttk = 1;
      for (jjj = J, ttj = 1; jjj >= jbound; jjj = (jjj + 1) / 2, ttj *= 2) {
        runTile(1, iii, jjj, kkk, tti, ttk, ttj, SmallestTile);
      }

      // only change K
      jjj = J;
      ttj = 1;
      for (kkk = (K + 1) / 2, ttk = 2; kkk >= kbound;
           kkk = (kkk + 1) / 2, ttk *= 2) {
        runTile(1, iii, jjj, kkk, tti, ttk, ttj, SmallestTile);
      }

      // change both J & K
      for (kkk = (K + 1) / 2, ttk = 2; kkk >= kbound;
           kkk = (kkk + 1) / 2, ttk *= 2) {
        for (jjj = (J + 1) / 2, ttj = 2; jjj >= jbound;
             jjj = (jjj + 1) / 2, ttj *= 2) {

          runTile(1, iii, jjj, kkk, tti, ttk, ttj, SmallestTile);
        }
      }
    }
  }

  auto time5 = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::micro> elapsed = time5 - time4;

  int timeSearch;
  int timePosttune;

  timeSearch = (int)(elapsed.count());

  printf("time of prunned search using the cost model:%d\n", timeSearch);

  printf("Choose Tiling: TI = %d, TK = %d, TJ = %d\n", estiii, estjjj, estkkk);

  fflush(stdout);

  reinitialize();
  PartialConfig = 4;
  ISDYNAMICJ = 1;
  configPartial(0, 1, 0);
  interorder = InterOrder(1);
  iii = estiii;
  jjj = estjjj;
  kkk = estkkk;
  tti = esttti;
  ttj = estttj;
  ttk = estttk;
  runTile(0, iii, jjj, kkk, tti, ttk, ttj, 0);

  return 0;
}
