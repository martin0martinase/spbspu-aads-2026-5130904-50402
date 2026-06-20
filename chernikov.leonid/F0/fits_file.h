#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include "bst.h"
#include "pixel_matrix.h"

enum class PixelType
{
  UINT8,
  INT16,
  INT32,
  FLOAT32,
  FLOAT64
};

class FitsFile
{
public:
  BinarySearchTree header;

  PixelType dataType;
  std::vector<uint8_t> data_uint8;
  std::vector<int16_t> data_int16;
  std::vector<int32_t> data_int32;
  std::vector<float> data_float32;
  std::vector<double> data_float64;

  int width = 0;
  int height = 0;
  bool loaded = false;

  FitsFile() = default;

  bool load(const std::string &filename);
  bool save(const std::string &filename);
  void printInfo() const;

  double getPixelAsDouble(int x, int y) const;

  double computeMin() const;
  double computeMax() const;
  double computeMean() const;
  double computeStdDev() const;

private:
  static std::string trim(const std::string &s);
  void parseHeaderBlock(const char *block);
  bool readData(std::ifstream &file, int headerBlocks);

  template <typename T>
  static void swapBytes(T &value)
  {
    char *bytes = reinterpret_cast<char *>(&value);
    std::reverse(bytes, bytes + sizeof(T));
  }
};