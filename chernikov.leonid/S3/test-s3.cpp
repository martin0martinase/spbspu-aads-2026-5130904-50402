#include <boost/test/unit_test.hpp>
#include "hash_table.hpp"
#include <string>
#include <stdexcept>

namespace chernikov {

  BOOST_AUTO_TEST_SUITE(HashTableTests)

  BOOST_AUTO_TEST_CASE(HashTableEmptyInitially)
  {
    HashTable< std::string, int > ht;
    BOOST_CHECK(ht.empty());
    BOOST_CHECK_EQUAL(ht.size(), 0);
  }

  BOOST_AUTO_TEST_CASE(HashTableAddAndHas)
  {
    HashTable< std::string, int > ht;
    ht.add("key1", 100);
    BOOST_CHECK(!ht.empty());
    BOOST_CHECK_EQUAL(ht.size(), 1);
    BOOST_CHECK(ht.has("key1"));
    BOOST_CHECK(!ht.has("key2"));
  }

  BOOST_AUTO_TEST_CASE(HashTableAddUpdate)
  {
    HashTable< std::string, int > ht;
    ht.add("key1", 100);
    ht.add("key1", 200);
    BOOST_CHECK_EQUAL(ht.size(), 1);
    BOOST_CHECK_EQUAL(ht.get("key1"), 200);
  }

  BOOST_AUTO_TEST_CASE(HashTableDrop)
  {
    HashTable< std::string, int > ht;
    ht.add("key1", 100);
    ht.add("key2", 200);

    int val = ht.drop("key1");
    BOOST_CHECK_EQUAL(val, 100);
    BOOST_CHECK(!ht.has("key1"));
    BOOST_CHECK_EQUAL(ht.size(), 1);
  }

}