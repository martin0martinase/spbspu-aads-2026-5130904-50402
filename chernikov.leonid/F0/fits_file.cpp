#include "fits_file.h"
#include <iostream>
#include <iomanip>

std::string FitsFile::trim(const std::string &s)
{
  size_t start = s.find_first_not_of(" \t");
  if (start == std::string::npos)
    return "";
  size_t end = s.find_last_not_of(" \t");
  return s.substr(start, end - start + 1);
}

void FitsFile::parseHeaderBlock(const char *block)
{
  for (int line = 0; line < 36; line++)
  {
    const char *start = block + line * 80;

    std::string first8(start, 8);
    if (first8.substr(0, 3) == "END")
    {
      return;
    }

    std::string key = trim(std::string(start, 8));
    if (key.empty())
      continue;

    std::string rest(start + 8, 72);

    size_t eqPos = rest.find("= ");
    if (eqPos == std::string::npos)
      continue;

    std::string valuePart = rest.substr(eqPos + 2);
    std::string value;
    std::string comment;

    valuePart = trim(valuePart);
    if (!valuePart.empty() && valuePart[0] == '\'')
    {
      size_t closeQuote = valuePart.find('\'', 1);
      if (closeQuote != std::string::npos)
      {
        value = valuePart.substr(1, closeQuote - 1);
        std::string afterQuote = valuePart.substr(closeQuote + 1);
        size_t commentPos = afterQuote.find(" /");
        if (commentPos != std::string::npos)
        {
          comment = trim(afterQuote.substr(commentPos + 2));
        }
      }
    }
    else
    {
      size_t commentPos = valuePart.find(" /");
      if (commentPos != std::string::npos)
      {
        value = trim(valuePart.substr(0, commentPos));
        comment = trim(valuePart.substr(commentPos + 2));
      }
      else
      {
        value = trim(valuePart);
      }
    }

    if (!key.empty() && !value.empty())
    {
      header.insert(key, value, comment);
    }
  }
}

bool FitsFile::readData(std::ifstream &file, int headerBlocks)
{
  std::string bitpixStr = header.find("BITPIX");
  std::string naxis1Str = header.find("NAXIS1");
  std::string naxis2Str = header.find("NAXIS2");

  if (bitpixStr.empty() || naxis1Str.empty() || naxis2Str.empty())
  {
    std::cerr << "Error: Missing required header keys (BITPIX, NAXIS1, NAXIS2)" << std::endl;
    return false;
  }

  int bitpix = std::stoi(bitpixStr);
  width = std::stoi(naxis1Str);
  height = std::stoi(naxis2Str);

  size_t totalPixels = width * height;
  int bytesPerPixel = std::abs(bitpix) / 8;
  size_t dataBytes = totalPixels * bytesPerPixel;

  file.seekg(headerBlocks * 2880, std::ios::beg);

  switch (bitpix)
  {
  case 8:
  {
    dataType = PixelType::UINT8;
    data_uint8.resize(totalPixels);
    file.read(reinterpret_cast<char *>(data_uint8.data()), dataBytes);
    break;
  }
  case 16:
  {
    dataType = PixelType::INT16;
    data_int16.resize(totalPixels);
    file.read(reinterpret_cast<char *>(data_int16.data()), dataBytes);
    for (auto &v : data_int16)
      swapBytes(v);
    break;
  }
  case 32:
  {
    dataType = PixelType::INT32;
    data_int32.resize(totalPixels);
    file.read(reinterpret_cast<char *>(data_int32.data()), dataBytes);
    for (auto &v : data_int32)
      swapBytes(v);
    break;
  }
  case -32:
  {
    dataType = PixelType::FLOAT32;
    data_float32.resize(totalPixels);
    file.read(reinterpret_cast<char *>(data_float32.data()), dataBytes);
    for (auto &v : data_float32)
      swapBytes(v);
    break;
  }
  case -64:
  {
    dataType = PixelType::FLOAT64;
    data_float64.resize(totalPixels);
    file.read(reinterpret_cast<char *>(data_float64.data()), dataBytes);
    for (auto &v : data_float64)
      swapBytes(v);
    break;
  }
  default:
    std::cerr << "Error: Unsupported BITPIX value: " << bitpix << std::endl;
    return false;
  }

  double bscale = header.contains("BSCALE") ? std::stod(header.find("BSCALE")) : 1.0;
  double bzero = header.contains("BZERO") ? std::stod(header.find("BZERO")) : 0.0;

  if (bscale != 1.0 || bzero != 0.0)
  {
    for (size_t i = 0; i < totalPixels; i++)
    {
      double raw = getPixelAsDouble(i % width, i / width);
      double calibrated = raw * bscale + bzero;
      if (dataType == PixelType::FLOAT64)
      {
        data_float64[i] = calibrated;
      }
    }
  }

  return true;
}

bool FitsFile::load(const std::string &filename)
{
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Error: Cannot open file " << filename << std::endl;
    return false;
  }

  int headerBlocks = 0;
  while (true)
  {
    char block[2880];
    file.read(block, 2880);
    if (file.gcount() != 2880)
    {
      std::cerr << "Error: Incomplete header block" << std::endl;
      return false;
    }
    headerBlocks++;

    bool foundEnd = false;
    for (int i = 0; i < 36; i++)
    {
      if (std::strncmp(block + i * 80, "END", 3) == 0)
      {
        foundEnd = true;
        break;
      }
    }

    parseHeaderBlock(block);

    if (foundEnd)
      break;
    if (headerBlocks > 100)
    {
      std::cerr << "Error: Too many header blocks" << std::endl;
      return false;
    }
  }

  if (!readData(file, headerBlocks))
  {
    return false;
  }

  loaded = true;
  std::cout << "Loaded: " << filename << " (" << width << "x" << height << ")" << std::endl;
  return true;
}

bool FitsFile::save(const std::string &filename)
{
  std::ofstream file(filename, std::ios::binary);
  if (!file.is_open())
  {
    std::cerr << "Error: Cannot write file " << filename << std::endl;
    return false;
  }

  std::vector<std::pair<std::string, std::string>> headerVec = header.toVector();
  std::string headerStr;

  for (const auto &[key, value] : headerVec)
  {
    std::string line(80, ' ');
    for (size_t i = 0; i < std::min(key.length(), size_t(8)); i++)
    {
      line[i] = key[i];
    }
    line[8] = '=';
    line[9] = ' ';
    std::string valStr = value;
    for (size_t i = 0; i < std::min(valStr.length(), size_t(20)); i++)
    {
      line[10 + i] = valStr[i];
    }
    headerStr += line;
  }

  std::string endLine(80, ' ');
  endLine[0] = 'E';
  endLine[1] = 'N';
  endLine[2] = 'D';
  headerStr += endLine;

  while (headerStr.length() % 2880 != 0)
  {
    headerStr += std::string(80, ' ');
  }

  file.write(headerStr.c_str(), headerStr.length());

  size_t totalPixels = width * height;
  std::vector<double> allPixels(totalPixels);
  for (size_t i = 0; i < totalPixels; i++)
  {
    allPixels[i] = getPixelAsDouble(i % width, i / width);
  }
  file.write(reinterpret_cast<const char *>(allPixels.data()), totalPixels * sizeof(double));

  std::cout << "Saved: " << filename << std::endl;
  return true;
}

void FitsFile::printInfo() const
{
  if (!loaded)
  {
    std::cout << "No file loaded." << std::endl;
    return;
  }
  std::cout << "File info:" << std::endl;
  std::cout << "  NAXIS1 (width): " << width << std::endl;
  std::cout << "  NAXIS2 (height): " << height << std::endl;
  std::cout << "  BITPIX: " << header.find("BITPIX") << std::endl;
  std::cout << "  Total pixels: " << (width * height) << std::endl;
}

double FitsFile::getPixelAsDouble(int x, int y) const
{
  size_t idx = y * width + x;
  if (idx >= (size_t)(width * height))
    return 0.0;

  switch (dataType)
  {
  case PixelType::UINT8:
    return static_cast<double>(data_uint8[idx]);
  case PixelType::INT16:
    return static_cast<double>(data_int16[idx]);
  case PixelType::INT32:
    return static_cast<double>(data_int32[idx]);
  case PixelType::FLOAT32:
    return static_cast<double>(data_float32[idx]);
  case PixelType::FLOAT64:
    return data_float64[idx];
  }
  return 0.0;
}

double FitsFile::computeMin() const
{
  double minVal = getPixelAsDouble(0, 0);
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      minVal = std::min(minVal, getPixelAsDouble(x, y));
  return minVal;
}

double FitsFile::computeMax() const
{
  double maxVal = getPixelAsDouble(0, 0);
  for (int y = 0; y < height; y++)
    for (int x = 0; x < width; x++)
      maxVal = std::max(maxVal, getPixelAsDouble(x, y));
  return maxVal;
}

double FitsFile::computeMean() const
{
  double sum = 0.0;
  size_t n = width * height;
  for (size_t i = 0; i < n; i++)
    sum += getPixelAsDouble(i % width, i / width);
  return sum / n;
}

double FitsFile::computeStdDev() const
{
  double mean = computeMean();
  double sumSq = 0.0;
  size_t n = width * height;
  for (size_t i = 0; i < n; i++)
  {
    double diff = getPixelAsDouble(i % width, i / width) - mean;
    sumSq += diff * diff;
  }
  return std::sqrt(sumSq / n);
}