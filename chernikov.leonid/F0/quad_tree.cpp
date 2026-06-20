#include "quad_tree.h"

QuadNode *QuadTree::build(void *context, PixelGetter getter, int x, int y, int w, int h)
{
  QuadNode *node = new QuadNode(x, y, w, h);

  double sum = 0.0;
  int count = w * h;
  for (int dy = 0; dy < h; dy++)
  {
    for (int dx = 0; dx < w; dx++)
    {
      sum += getter(context, x + dx, y + dy);
    }
  }
  node->avgBrightness = sum / count;

  if (count <= MIN_LEAF_SIZE)
  {
    node->isLeaf = true;
    return node;
  }

  // Делим на 4 части
  node->isLeaf = false;
  int halfW = w / 2;
  int halfH = h / 2;
  int restW = w - halfW;
  int restH = h - halfH;

  // NW левый верхний
  node->children[0] = build(context, getter, x, y, halfW, halfH);
  // NE правый верхний
  node->children[1] = build(context, getter, x + halfW, y, restW, halfH);
  // SW левый нижний
  node->children[2] = build(context, getter, x, y + halfH, halfW, restH);
  // SE правый нижний
  node->children[3] = build(context, getter, x + halfW, y + halfH, restW, restH);

  return node;
}

void QuadTree::build(void *context, PixelGetter getter, int width, int height)
{
  delete root;
  root = build(context, getter, 0, 0, width, height);
  std::cout << "QuadTree built successfully." << std::endl;
}

double QuadTree::getPixelRec(QuadNode *node, void *context, PixelGetter getter, int px, int py)
{
  if (node->isLeaf)
  {
    return getter(context, px, py);
  }
  int midX = node->x + node->children[0]->width;
  int midY = node->y + node->children[0]->height;

  if (px < midX && py < midY)
  {
    return getPixelRec(node->children[0], context, getter, px, py); // NW
  }
  else if (px >= midX && py < midY)
  {
    return getPixelRec(node->children[1], context, getter, px, py); // NE
  }
  else if (px < midX && py >= midY)
  {
    return getPixelRec(node->children[2], context, getter, px, py); // SW
  }
  else
  {
    return getPixelRec(node->children[3], context, getter, px, py); // SE
  }
}

double QuadTree::getPixel(void *context, PixelGetter getter, int x, int y)
{
  if (root == nullptr)
  {
    std::cerr << "Error: QuadTree not built. Use BUILD_TREE first." << std::endl;
    return 0.0;
  }
  return getPixelRec(root, context, getter, x, y);
}

void QuadTree::findSourcesRec(QuadNode *node, void *context, PixelGetter getter,
                              double threshold, std::vector<Source> &result)
{
  if (node->avgBrightness < threshold)
  {
    return;
  }

  if (node->isLeaf)
  {
    for (int dy = 0; dy < node->height; dy++)
    {
      for (int dx = 0; dx < node->width; dx++)
      {
        int px = node->x + dx;
        int py = node->y + dy;
        double brightness = getter(context, px, py);
        if (brightness > threshold)
        {
          result.push_back({px, py, brightness});
        }
      }
    }
  }
  else
  {
    for (int i = 0; i < 4; i++)
    {
      if (node->children[i] != nullptr)
      {
        findSourcesRec(node->children[i], context, getter, threshold, result);
      }
    }
  }
}

std::vector<Source> QuadTree::findSources(void *context, PixelGetter getter, double threshold)
{
  std::vector<Source> result;
  if (root == nullptr)
  {
    std::cerr << "Error: QuadTree not built. Use BUILD_TREE first." << std::endl;
    return result;
  }
  findSourcesRec(root, context, getter, threshold, result);

  for (size_t i = 0; i < result.size(); i++)
  {
    for (size_t j = i + 1; j < result.size(); j++)
    {
      if (result[j].brightness > result[i].brightness)
      {
        std::swap(result[i], result[j]);
      }
    }
  }

  return result;
}