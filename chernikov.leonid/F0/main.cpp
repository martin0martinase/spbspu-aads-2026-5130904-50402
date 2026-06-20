#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include "fits_file.h"
#include "quad_tree.h"

double getFitsPixel(void *context, int x, int y)
{
  FitsFile *fits = static_cast<FitsFile *>(context);
  return fits->getPixelAsDouble(x, y);
}

void printHistogram(FitsFile &fits, int bins = 20)
{
  double minVal = fits.computeMin();
  double maxVal = fits.computeMax();
  double binWidth = (maxVal - minVal) / bins;

  std::vector<int> histogram(bins, 0);
  int maxCount = 0;

  for (int y = 0; y < fits.height; y++)
  {
    for (int x = 0; x < fits.width; x++)
    {
      double val = fits.getPixelAsDouble(x, y);
      int bin = static_cast<int>((val - minVal) / binWidth);
      if (bin >= bins)
        bin = bins - 1;
      if (bin < 0)
        bin = 0;
      histogram[bin]++;
      if (histogram[bin] > maxCount)
        maxCount = histogram[bin];
    }
  }

  // ASCII-гистограмма
  int maxBarWidth = 50;
  std::cout << "\nHistogram (" << bins << " bins):\n";
  std::cout << "Range: [" << minVal << ", " << maxVal << "]\n\n";

  for (int i = 0; i < bins; i++)
  {
    double binStart = minVal + i * binWidth;
    std::cout << std::setw(10) << std::fixed << std::setprecision(2) << binStart << " | ";
    int barWidth = (maxCount > 0) ? (histogram[i] * maxBarWidth / maxCount) : 0;
    for (int j = 0; j < barWidth; j++)
      std::cout << "#";
    std::cout << " (" << histogram[i] << ")\n";
  }
}

int main()
{
  FitsFile currentFile;
  QuadTree quadTree;
  bool treeBuilt = false;

  std::cout << "FITS Analyzer v0.1\n";
  std::cout << "Commands: LOAD, HEADER, INFO, STAT, HISTOGRAM, BUILD_TREE, GET_PIXEL, FIND_SOURCES, SAVE, EXIT\n\n";

  std::string line;
  while (true)
  {
    std::cout << "> ";
    std::getline(std::cin, line);
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    if (cmd == "LOAD")
    {
      std::string filename;
      iss >> filename;
      if (!filename.empty())
      {
        currentFile.load(filename);
        treeBuilt = false;
      }
      else
      {
        std::cout << "Usage: LOAD filename.fit\n";
      }
    }
    else if (cmd == "HEADER")
    {
      if (currentFile.loaded)
      {
        std::cout << "\n--- FITS Header ---\n";
        currentFile.header.printInOrder();
        std::cout << "--- End Header ---\n\n";
      }
      else
      {
        std::cout << "No file loaded.\n";
      }
    }
    else if (cmd == "INFO")
    {
      currentFile.printInfo();
    }
    else if (cmd == "STAT")
    {
      if (!currentFile.loaded)
      {
        std::cout << "No file loaded.\n";
        continue;
      }
      std::cout << "\n--- Statistics ---\n";
      std::cout << "Min:    " << currentFile.computeMin() << "\n";
      std::cout << "Max:    " << currentFile.computeMax() << "\n";
      std::cout << "Mean:   " << currentFile.computeMean() << "\n";
      std::cout << "StdDev: " << currentFile.computeStdDev() << "\n";
      std::cout << "--- End Statistics ---\n\n";
    }
    else if (cmd == "HISTOGRAM")
    {
      if (!currentFile.loaded)
      {
        std::cout << "No file loaded.\n";
        continue;
      }
      printHistogram(currentFile);
    }
    else if (cmd == "BUILD_TREE")
    {
      if (!currentFile.loaded)
      {
        std::cout << "No file loaded.\n";
        continue;
      }
      quadTree.build(&currentFile, getFitsPixel, currentFile.width, currentFile.height);
      treeBuilt = true;
    }
    else if (cmd == "GET_PIXEL")
    {
      if (!treeBuilt)
      {
        std::cout << "Build QuadTree first (BUILD_TREE).\n";
        continue;
      }
      int x, y;
      iss >> x >> y;
      double pixel = quadTree.getPixel(&currentFile, getFitsPixel, x, y);
      std::cout << "Pixel(" << x << ", " << y << ") = " << pixel << "\n";
    }
    else if (cmd == "FIND_SOURCES")
    {
      if (!treeBuilt)
      {
        std::cout << "Build QuadTree first (BUILD_TREE).\n";
        continue;
      }
      double sigma;
      iss >> sigma;
      double mean = currentFile.computeMean();
      double stddev = currentFile.computeStdDev();
      double threshold = mean + sigma * stddev;

      std::cout << "Threshold: " << threshold << " (mean + " << sigma << " * stddev)\n";
      std::vector<Source> sources = quadTree.findSources(&currentFile, getFitsPixel, threshold);

      std::cout << "Found " << sources.size() << " bright pixels:\n";
      int show = std::min(20, (int)sources.size());
      for (int i = 0; i < show; i++)
      {
        std::cout << "  (" << sources[i].x << ", " << sources[i].y
                  << ") brightness = " << sources[i].brightness << "\n";
      }
      if ((int)sources.size() > show)
      {
        std::cout << "  ... and " << (sources.size() - show) << " more\n";
      }
    }
    else if (cmd == "CALIBRATE")
    {
      std::string darkFile;
      iss >> darkFile;
      std::cout << "CALIBRATE not fully implemented yet. Dark file: " << darkFile << "\n";
    }
    else if (cmd == "SAVE")
    {
      std::string filename;
      iss >> filename;
      if (!filename.empty() && currentFile.loaded)
      {
        currentFile.save(filename);
      }
      else
      {
        std::cout << "Usage: SAVE filename.fit\n";
      }
    }
    else if (cmd == "EXIT" || cmd == "QUIT")
    {
      std::cout << "Bye!\n";
      break;
    }
    else if (!cmd.empty())
    {
      std::cout << "Unknown command: " << cmd << "\n";
    }
  }

  return 0;
}