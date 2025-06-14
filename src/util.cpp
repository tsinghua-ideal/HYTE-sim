#include "util.h"
#include "headers.h"
#include <cmath>

// calculate log base 2 of x
int getlog(int x) {
  int ret = -1;
  while (x) {
    ret++;
    x >>= 1;
  }
  return ret;
}

// Hash function h1 and h2
double hash1(int x, double a, double b, int pmod) {
  // printf("%lf %lf\n", a*x+b, (double)(std::fmod(a * x + b, pmod)));
  return (double)(std::fmod(a * x + b, pmod)) / pmod;
}

double hash2(int x, double a, double b, int pmod) {
  return (double)(std::fmod(a * x + b, pmod)) / pmod;
}

// Combined hash function h(x, y)
double h(int x, int y, double a1, double b1, double a2, double b2, int pmod) {
  double h1_value = hash1(x, a1, b1, pmod);
  double h2_value = hash2(y, a2, b2, pmod);
  double ret = h1_value - h2_value;
  if (ret < 0)
    ret += 1.0;

  return ret;
}

double h(double h1_value, double h2_value) {
  double ret = h1_value - h2_value;
  if (ret < 0)
    ret += 1.0;
  return ret;
}

// Function to generate random coefficients
double getRandomCoefficient() { return dis(gen); }

bool sampleP() { return dis(gen) < samplep; }

// merge and deduplicate two sorted vectors
std::vector<double> mergevector(const vector<double> &left,
                                const vector<double> &right) {
  std::vector<double> result(left.size() + right.size());
  std::merge(left.begin(), left.end(), right.begin(), right.end(),
             result.begin());

  auto last = unique(result.begin(), result.end());
  result.erase(last, result.end());

  return result;
}