#ifndef UTIL_H
#define UTIL_H

#include <random>
#include <vector>

int getlog(int x);

// Global variables
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<> dis(0.0, 1.0);

double hash1(int x, double a, double b, int pmod);
double hash2(int x, double a, double b, int pmod);
double h(int x, int y, double a1, double b1, double a2, double b2, int pmod);
double h(double h1, double h2);
double getRandomCoefficient();
bool sampleP();

std::vector<double> mergevector(const std::vector<double> &left,
                                const std::vector<double> &right);

#endif // UTIL_H
