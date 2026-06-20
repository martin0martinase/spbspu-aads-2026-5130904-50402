#pragma once
#include <string>
#include <vector>
#include <utility>
#include <iostream>

struct BstNode
{
  std::string key;
  std::string value;
  std::string comment;
  BstNode *left;
  BstNode *right;

  BstNode(const std::string &k, const std::string &v, const std::string &c = "")
      : key(k), value(v), comment(c), left(nullptr), right(nullptr) {}
};

class BinarySearchTree
{
  BstNode *root;

  BstNode *insertRec(BstNode *node, const std::string &key, const std::string &value, const std::string &comment)
  {
    if (node == nullptr)
    {
      return new BstNode(key, value, comment);
    }
    if (key < node->key)
    {
      node->left = insertRec(node->left, key, value, comment);
    }
    else if (key > node->key)
    {
      node->right = insertRec(node->right, key, value, comment);
    }
    else
    {
      node->value = value;
      node->comment = comment;
    }
    return node;
  }

  BstNode *findRec(BstNode *node, const std::string &key) const
  {
    if (node == nullptr)
      return nullptr;
    if (key == node->key)
      return node;
    if (key < node->key)
      return findRec(node->left, key);
    return findRec(node->right, key);
  }

  void inOrderRec(BstNode *node) const
  {
    if (node == nullptr)
      return;
    inOrderRec(node->left);
    std::cout << node->key;
    int padding = 8 - node->key.length();
    for (int i = 0; i < padding; i++)
      std::cout << " ";
    std::cout << "= " << node->value;
    if (!node->comment.empty())
    {
      std::cout << " / " << node->comment;
    }
    std::cout << std::endl;
    inOrderRec(node->right);
  }

  void toVectorRec(BstNode *node, std::vector<std::pair<std::string, std::string>> &vec) const
  {
    if (node == nullptr)
      return;
    toVectorRec(node->left, vec);
    vec.push_back({node->key, node->value});
    toVectorRec(node->right, vec);
  }

  void destroyRec(BstNode *node)
  {
    if (node == nullptr)
      return;
    destroyRec(node->left);
    destroyRec(node->right);
    delete node;
  }

public:
  BinarySearchTree() : root(nullptr) {}
  ~BinarySearchTree() { destroyRec(root); }

  void insert(const std::string &key, const std::string &value, const std::string &comment = "")
  {
    root = insertRec(root, key, value, comment);
  }

  std::string find(const std::string &key) const
  {
    BstNode *node = findRec(root, key);
    return node ? node->value : "";
  }

  bool contains(const std::string &key) const
  {
    return findRec(root, key) != nullptr;
  }

  void printInOrder() const
  {
    inOrderRec(root);
  }

  std::vector<std::pair<std::string, std::string>> toVector() const
  {
    std::vector<std::pair<std::string, std::string>> result;
    toVectorRec(root, result);
    return result;
  }

  BinarySearchTree(const BinarySearchTree &) = delete;
  BinarySearchTree &operator=(const BinarySearchTree &) = delete;
};