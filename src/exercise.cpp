#include <map>
#include <limits>

template<typename K, typename V>
class interval_map {
  std::map<K, V> m_map;

public:
  // constructor associates whole range of K with val by inserting (K_min, val)
  // into the map
  interval_map(V const& val) {
    m_map.insert(m_map.end(), std::make_pair(std::numeric_limits<K>::lowest(), val));
  }

  // Assign value val to interval [keyBegin, keyEnd).
  // Overwrite previous values in this interval.
  // Conforming to the C++ Standard Library conventions, the interval
  // includes keyBegin, but excludes keyEnd.
  // If !( keyBegin < keyEnd ), this designates an empty interval,
  // and assign must do nothing.
  void assign(K const& keyBegin, K const& keyEnd, V const& val) {

    // If !(keyBegin < keyEnd), assign should do nothing
    if (!(keyBegin < keyEnd))
      return;

    // Store the end value to reinsert it at the end of the range
    const V endVal = (*this)[keyEnd];

    // Erase values in the range
    auto beginIt = m_map.lower_bound(keyBegin);
    auto endIt = m_map.lower_bound(keyEnd);

    // If the value at keyBegin isn't already the right value, insert it
    if (beginIt == m_map.end() || m_map.key_comp()(keyBegin, beginIt->first)) {
      beginIt = m_map.insert(beginIt, std::make_pair(keyBegin, val));
    }
    else {
      beginIt->second = val;
    }

    // If the value at keyEnd isn't already the right value, insert it
    if (endIt == m_map.end() || m_map.key_comp()(keyEnd, endIt->first)) {
      endIt = m_map.insert(endIt, std::make_pair(keyEnd, endVal));
    }
    else {
      endIt->second = endVal;
    }

    if (++beginIt != endIt)
      m_map.erase(beginIt, endIt);
  }

  // look-up of the value associated with key
  V const& operator[](K const& key) const {
    return (--m_map.upper_bound(key))->second;
  }

  // little backdoor for verifying canonical representation in tests
  const std::map<K, V>& map() const { return m_map; }
};

// Unit tests
#include <catch.hpp>
#include <random>
#include <optional>

#define TEST_MACRO REQUIRE
#define TEST_MACRO_FALSE REQUIRE_FALSE

// Key type that implements only the specified
class Key {
  int m_val;

public:
  Key(int v) : m_val(v) {}
  Key(const Key& k) { m_val = k.m_val; }
  Key& operator=(const Key& k) { m_val = k.m_val; return *this; }
  bool operator<(const Key& k) const { return m_val < k.m_val; }

  int val() const { return m_val; };
};

namespace std {
  template<> class numeric_limits<Key> : public numeric_limits<int> {};
}

std::ostream& operator << (std::ostream& os, Key const& value) {
  os << value.val();
  return os;
}

// Key type that implements only the specified operations
class Val {
  char m_val;

public:
  Val(char v) : m_val(v) {}
  Val(const Val& v) { m_val = v.m_val; }
  Val& operator=(const Val& v) { m_val = v.m_val; return *this; }
  bool operator==(const Val& v) const { return m_val == v.m_val; }

  char val() const { return m_val; };
};

std::ostream& operator << (std::ostream& os, Val const& value) {
  os << value.val();
  return os;
}

auto checkCanonicity = [](const interval_map<Key,Val>& m) {
  INFO("check map canonicity");
  const auto& internalMap = m.map();
  // Check this is the minimal representation
  std::optional<Val> lastVal;
  for (const auto& key : internalMap) {
    if (lastVal.has_value())
      TEST_MACRO_FALSE(lastVal.value().val() == key.second.val());
    lastVal = key.second;
  }
};

auto checkSize = [](const interval_map<Key, Val>& m, size_t size) {
  INFO("check map size");
  TEST_MACRO(m.map().size() == size);
};

TEST_CASE("interval_map") {
  SECTION("Key type") {
    TEST_MACRO(Key(4) < Key(5));
    TEST_MACRO_FALSE(Key(5) < Key(5));
    TEST_MACRO(Key(5) < Key(6));

    Key key1(99);
    Key key2(100);
    Key key3(101);
    TEST_MACRO_FALSE(Key(key1) < key1);
    TEST_MACRO(Key(key2) < key3);
    TEST_MACRO_FALSE(Key(key2) < key1);
  }

  SECTION("Val type") {
    TEST_MACRO_FALSE(Val(4) == Val(5));
    TEST_MACRO(Val(5) == Val(5));
    TEST_MACRO_FALSE(Val(6) == Val(5));

    Val val1(99);
    Val val2(100);
    TEST_MACRO(Val(val1) == val1);
    TEST_MACRO_FALSE(Val(val1) == val2);
  }

  SECTION("interval_map Constructor") {
    interval_map<Key, Val> m(Val(5));

    Key key = std::numeric_limits<Key>::lowest();

    TEST_MACRO_FALSE(m[key] == Val(4));
    TEST_MACRO(m[key] == Val(5));
    TEST_MACRO_FALSE(m[key] == Val(6));

    checkSize(m, 1);
    checkCanonicity(m);
  }

  SECTION("their example") {
    interval_map<Key, Val> m(Val('A'));
    m.assign(Key(3), Key(5), Val('B'));

    TEST_MACRO(m[Key(0)] == 'A');
    TEST_MACRO(m[Key(1)] == 'A');
    TEST_MACRO(m[Key(2)] == 'A');
    TEST_MACRO(m[Key(3)] == 'B');
    TEST_MACRO(m[Key(4)] == 'B');
    TEST_MACRO(m[Key(5)] == 'A');
    TEST_MACRO(m[Key(6)] == 'A');
    TEST_MACRO(m[Key(7)] == 'A');

    checkSize(m, 3);
    checkCanonicity(m);
  }

  SECTION("interval_map with the initial range (min, max) -> 'a'") {
    interval_map<Key, Val> m(Val('a'));
  
    SECTION("with the additional range (10, 100) -> 'b'") {
      const Key min = std::numeric_limits<Key>::lowest();
      const Key max = std::numeric_limits<Key>::max();

      // Check min, max, and arbitrary middle values
      TEST_MACRO(m[min] == Val('a'));
      TEST_MACRO(m[Key(0)] == Val('a'));
      TEST_MACRO(m[Key(1000)] == Val('a'));
      TEST_MACRO(m[max] == Val('a'));

      // Assign a range to a different value
      m.assign(Key(10), Key(100), Val('b'));

      // Check our keys again
      TEST_MACRO(m[min] == Val('a'));
      TEST_MACRO(m[Key(0)] == Val('a'));
      TEST_MACRO(m[Key(1000)] == Val('a'));
      TEST_MACRO(m[max] == Val('a'));

      // Check boundaries
      TEST_MACRO(m[Key(9)] == Val('a'));
      TEST_MACRO(m[Key(10)] == Val('b'));
      TEST_MACRO(m[Key(11)] == Val('b'));
      TEST_MACRO(m[Key(99)] == Val('b'));
      TEST_MACRO(m[Key(100)] == Val('a'));

      checkSize(m, 3);
      checkCanonicity(m);

      SECTION("updating a key range that already exists") {
        m.assign(Key(10), Key(100), Val('c'));

        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('c'));
        TEST_MACRO(m[Key(11)] == Val('c'));
        TEST_MACRO(m[Key(99)] == Val('c'));
        TEST_MACRO(m[Key(100)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
      }

      SECTION("assigning a range that matches the key of the existing range") {
        checkSize(m, 3);
        checkCanonicity(m);
        m.assign(Key(1000), Key(2000), Val('a'));
        checkSize(m, 3);
        checkCanonicity(m);
      }

      SECTION("Assigning a range that includes a but doesn't touch the next range") {
        // Assign a range that corresponds with our a key
        m.assign(Key(-10), Key(9), Val('c'));

        // Check our keys again
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('c'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));

        // Check boundaries
        TEST_MACRO(m[Key(-11)] == Val('a'));
        TEST_MACRO(m[Key(-10)] == Val('c'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));

        checkSize(m, 5);
        checkCanonicity(m);
      }

      SECTION("Assigning a range that includes a and touches the next range") {
        // Assign a range that corresponds with our a key
        m.assign(Key(-10), Key(10), Val('c'));

        // Check our keys again
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('c'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));

        // Check boundaries
        TEST_MACRO(m[Key(-11)] == Val('a'));
        TEST_MACRO(m[Key(-10)] == Val('c'));
        TEST_MACRO(m[Key(9)] == Val('c'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));

        checkSize(m, 4);
        checkCanonicity(m);
      }

      SECTION("try a range where !(begin < end)") {
        // Assign a range with !(begin < end), which should designate an empty
        // interval and do nothing
        m.assign(Key(50), Key(0), Val('c'));

        // Check that the previous is still correct
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(10000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Check boundaries
        TEST_MACRO(m[Key(49)] == Val('b'));
        TEST_MACRO(m[Key(50)] == Val('b'));
        TEST_MACRO(m[Key(51)] == Val('b'));
        TEST_MACRO(m[Key(-1)] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1)] == Val('a'));
      }

      SECTION("the whole range can be replaced") {
        m.assign(min, max, Val('c'));

        // Check previous values have all updated
        TEST_MACRO(m[min] == Val('c'));
        TEST_MACRO(m[Key(0)] == Val('c'));
        TEST_MACRO(m[Key(1000)] == Val('c'));
        // todo: I think it's weird that you can't change the interval_map's full
        // range using assign, but because keyEnd is exclusive, the max key value
        // will always return the Val('a') value for the map
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('c'));
        TEST_MACRO(m[Key(10)] == Val('c'));
        TEST_MACRO(m[Key(11)] == Val('c'));
        TEST_MACRO(m[Key(99)] == Val('c'));
        TEST_MACRO(m[Key(100)] == Val('c'));
      }

      SECTION("try overwriting an existing range from the start") {
        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Overwrite the start of the range 10,100
        m.assign(Key(5), Key(15), Val('c'));

        // Check previous values have updated where appropriate
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('c'));
        TEST_MACRO(m[Key(10)] == Val('c'));
        TEST_MACRO(m[Key(11)] == Val('c'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Check boundary conditions
        TEST_MACRO(m[Key(4)] == Val('a'));
        TEST_MACRO(m[Key(5)] == Val('c'));
        for (int i = 5; i < 15; ++i) {
          TEST_MACRO(m[Key(i)] == Val('c'));
        }
        TEST_MACRO(m[Key(14)] == Val('c'));
        TEST_MACRO(m[Key(15)] == Val('b'));
        TEST_MACRO(m[Key(16)] == Val('b'));
      }

      SECTION("try overwriting an existing range from the end") {
        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Overwrite the start of the range 10,100
        m.assign(Key(95), Key(105), Val('c'));

        // Check previous values have updated where appropriate
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('c'));
        TEST_MACRO(m[Key(100)] == Val('c'));

        // Check boundary conditions
        TEST_MACRO(m[Key(94)] == Val('b'));
        TEST_MACRO(m[Key(95)] == Val('c'));
        for (int i = 95; i < 104; ++i) {
          TEST_MACRO(m[Key(i)] == Val('c'));
        }
        TEST_MACRO(m[Key(104)] == Val('c'));
        TEST_MACRO(m[Key(105)] == Val('a'));
        TEST_MACRO(m[Key(106)] == Val('a'));
      }

      SECTION("try overwriting an existing range in the center") {
        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Overwrite the start of the range 10,100
        m.assign(Key(45), Key(55), Val('c'));

        // Check previous values are the same
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Check boundary conditions
        TEST_MACRO(m[Key(44)] == Val('b'));
        TEST_MACRO(m[Key(45)] == Val('c'));
        for (int i = 45; i < 54; ++i) {
          TEST_MACRO(m[Key(i)] == Val('c'));
        }
        TEST_MACRO(m[Key(54)] == Val('c'));
        TEST_MACRO(m[Key(55)] == Val('b'));
        TEST_MACRO(m[Key(56)] == Val('b'));
      }

      SECTION("assign to max value") {
        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Overwrite max
        m.assign(max, max, Val('c'));

        // Check interval map hasn't changed
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));
      }

      SECTION("assign around the min value") {
        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        for (int i = min.val(); i < min.val() + 5; ++i) {
          TEST_MACRO(m[Key(i)] == Val('a'));
        }
        TEST_MACRO(m[Key(min.val() + 5)] == Val('a'));

        checkSize(m, 3);
        checkCanonicity(m);

        // Overwrite min
        m.assign(Key(min.val()), Key(min.val() + 5), Val('c'));

        // Ensure previous state
        TEST_MACRO(m[min] == Val('c'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        // Check boundaries have changed
        for (int i = min.val(); i < min.val() + 5; ++i) {
          TEST_MACRO(m[Key(i)] == Val('c'));
        }
        TEST_MACRO(m[Key(min.val() + 5)] == Val('a'));

        checkSize(m, 4);
        checkCanonicity(m);

        SECTION("restore the map to the original state with the same operation but the original value") {
          m.assign(Key(min.val()), Key(min.val() + 5), Val('a'));

          // Ensure previous state
          TEST_MACRO(m[min] == Val('a'));
          TEST_MACRO(m[Key(0)] == Val('a'));
          TEST_MACRO(m[Key(1000)] == Val('a'));
          TEST_MACRO(m[max] == Val('a'));
          TEST_MACRO(m[Key(9)] == Val('a'));
          TEST_MACRO(m[Key(10)] == Val('b'));
          TEST_MACRO(m[Key(11)] == Val('b'));
          TEST_MACRO(m[Key(99)] == Val('b'));
          TEST_MACRO(m[Key(100)] == Val('a'));

          for (int i = min.val(); i < min.val() + 5; ++i) {
            TEST_MACRO(m[Key(i)] == Val('a'));
          }
          TEST_MACRO(m[Key(min.val() + 5)] == Val('a'));

          checkSize(m, 3);
          checkCanonicity(m);
        }

        SECTION("restore the map to the original state with a slightly wider operation") {
          m.assign(Key(min.val()), Key(min.val() + 10), Val('a'));

          // Ensure previous state
          TEST_MACRO(m[min] == Val('a'));
          TEST_MACRO(m[Key(0)] == Val('a'));
          TEST_MACRO(m[Key(1000)] == Val('a'));
          TEST_MACRO(m[max] == Val('a'));
          TEST_MACRO(m[Key(9)] == Val('a'));
          TEST_MACRO(m[Key(10)] == Val('b'));
          TEST_MACRO(m[Key(11)] == Val('b'));
          TEST_MACRO(m[Key(99)] == Val('b'));
          TEST_MACRO(m[Key(100)] == Val('a'));

          for (int i = min.val(); i < min.val() + 5; ++i) {
            TEST_MACRO(m[Key(i)] == Val('a'));
          }
          TEST_MACRO(m[Key(min.val() + 5)] == Val('a'));

          checkSize(m, 3);
          checkCanonicity(m);
        }

        SECTION("restore the min value, leaving only one 'c'") {
          m.assign(Key(min.val()), Key(min.val() + 4), Val('a'));

          // Ensure previous state
          TEST_MACRO(m[min] == Val('a'));
          TEST_MACRO(m[Key(0)] == Val('a'));
          TEST_MACRO(m[Key(1000)] == Val('a'));
          TEST_MACRO(m[max] == Val('a'));
          TEST_MACRO(m[Key(9)] == Val('a'));
          TEST_MACRO(m[Key(10)] == Val('b'));
          TEST_MACRO(m[Key(11)] == Val('b'));
          TEST_MACRO(m[Key(99)] == Val('b'));
          TEST_MACRO(m[Key(100)] == Val('a'));

          for (int i = min.val(); i < min.val() + 4; ++i) {
            TEST_MACRO(m[Key(i)] == Val('a'));
          }
          TEST_MACRO(m[Key(min.val() + 4)] == Val('c'));
          TEST_MACRO(m[Key(min.val() + 5)] == Val('a'));

          checkSize(m, 5);
          checkCanonicity(m);
        }
      }

      SECTION("assign to just before the max value") {
        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        for (int i = max.val() - 5; i < max.val(); ++i) {
          TEST_MACRO(m[Key(i)] == Val('a'));
        }

        checkSize(m, 3);
        checkCanonicity(m);

        // Overwrite max
        m.assign(Key(max.val() - 5), max, Val('c'));

        // Check interval map has only changed where it should
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        for (int i = max.val() - 5; i < max.val(); ++i) {
          TEST_MACRO(m[Key(i)] == Val('c'));
        }

        checkSize(m, 5);
        checkCanonicity(m);

        // Try and restore old map state
        m.assign(Key(max.val() - 5), max, Val('a'));

        // Ensure previous state
        TEST_MACRO(m[min] == Val('a'));
        TEST_MACRO(m[Key(0)] == Val('a'));
        TEST_MACRO(m[Key(1000)] == Val('a'));
        TEST_MACRO(m[max] == Val('a'));
        TEST_MACRO(m[Key(9)] == Val('a'));
        TEST_MACRO(m[Key(10)] == Val('b'));
        TEST_MACRO(m[Key(11)] == Val('b'));
        TEST_MACRO(m[Key(99)] == Val('b'));
        TEST_MACRO(m[Key(100)] == Val('a'));

        for (int i = max.val() - 5; i < max.val(); ++i) {
          TEST_MACRO(m[Key(i)] == Val('a'));
        }

        checkSize(m, 3);
        checkCanonicity(m);
      }
    }

    SECTION("random tests") {
      std::map<Key, Val> tests;

      auto runTests = [&tests, &m]() {
        checkCanonicity(m);
        for (const auto& test : tests) {
          INFO(std::string("Testing for key ") + std::to_string(test.first.val()))
          TEST_MACRO(m[test.first] == test.second);
        }
      };

      auto resetTests = [&tests, &m]() {
        m = interval_map<Key, Val>(Val('a'));
        tests.clear();
      };

      // Generate a random testing range and tests for it, updating old tests
      std::function<void(Key, Key, int, int)> testRandomRange = [&](Key randomMin, Key randomMax, int depth, int maxDepth) {
        if (depth >= maxDepth)
          return;

        // Initialise random number generator
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> keyDist(randomMin.val(), randomMax.val());
        std::uniform_int_distribution<int> valDist('A', 'z');

        Key rangeMin = Key(keyDist(mt));
        Key rangeMax = Key(keyDist(mt));
        Val rangeVal = Val(valDist(mt));
        if (rangeMax < rangeMin)
          std::swap(rangeMin, rangeMax);

        auto sectionName = std::string("the random range (")
          + std::to_string(rangeMin.val()) + ", "
          + std::to_string(rangeMax.val()) + ") -> "
          + std::to_string(rangeVal.val()) + " is added";
        INFO(sectionName);
        {
          // Read a value out of m and store it in a map
          auto saveKeyVal = [&](std::map<Key, Val>& keyValMap, const Key& val) {
            keyValMap.insert(std::make_pair(val, m[val]));
          };

          // Save the boundaries of a range in m to a map
          auto saveBoundaries = [&](std::map<Key, Val>& boundaries, const Key& rangeMin, const Key& rangeMax) {
            if (Key(std::numeric_limits<Key>::lowest()) < rangeMin)
              saveKeyVal(boundaries, Key(rangeMin.val() - 1));
            saveKeyVal(boundaries, Key(rangeMin.val()));
            if (rangeMin < Key(std::numeric_limits<Key>::max()) && (rangeMin.val() + 1) < rangeMax.val())
              saveKeyVal(boundaries, Key(rangeMin.val() + 1));
            if (Key(std::numeric_limits<Key>::lowest()) < rangeMax && (rangeMax.val() - 1) > rangeMin.val())
              saveKeyVal(boundaries, Key(rangeMax.val() - 1));
            saveKeyVal(boundaries, Key(rangeMax.val()));
            if (rangeMax < Key(std::numeric_limits<Key>::max()))
              saveKeyVal(boundaries, Key(rangeMax.val() + 1));
          };

          // Compare two sets of boundaries
          auto compareBoundaries = [&](const std::map<Key, Val>& a, const std::map<Key, Val>& b, const Key& rangeMin, const Key& rangeMax, const Val& rangeVal) {
            INFO(std::string("Comparing boundaries ") + std::to_string(rangeMin.val()) + ", " + std::to_string(rangeMax.val()));
            if (rangeMin < rangeMax) {
              REQUIRE(a.size() == a.size());
              if (Key(std::numeric_limits<Key>::lowest()) < rangeMin)
                TEST_MACRO(a.at(Key(rangeMin.val() - 1)) == b.at(Key(rangeMin.val() - 1)));
              TEST_MACRO(b.at(Key(rangeMin.val())) == rangeVal);
              if (rangeMin < Key(std::numeric_limits<Key>::max()) && (rangeMin.val() + 1) < rangeMax.val())
                TEST_MACRO(b.at(Key(rangeMin.val() + 1)) == rangeVal);
              if (Key(std::numeric_limits<Key>::lowest()) < rangeMax && (rangeMax.val() - 1) > rangeMin.val())
                TEST_MACRO(b.at(Key(rangeMax.val() - 1)) == rangeVal);
              TEST_MACRO(a.at(Key(rangeMax.val())) == b.at(Key(rangeMax.val())));
              if (rangeMax < Key(std::numeric_limits<Key>::max()))
                TEST_MACRO(a.at(Key(rangeMax.val() + 1)) == b.at(Key(rangeMax.val() + 1)));
            }
            else {
              REQUIRE(a.size() == a.size());
              if (Key(std::numeric_limits<Key>::lowest()) < rangeMin)
                TEST_MACRO(a.at(Key(rangeMin.val() - 1)) == b.at(Key(rangeMin.val() - 1)));
              TEST_MACRO(a.at(Key(rangeMin.val())) == b.at(Key(rangeMin.val())));
              if (rangeMin < Key(std::numeric_limits<Key>::max()) && (rangeMin.val() + 1) < rangeMax.val())
                TEST_MACRO(a.at(Key(rangeMin.val() + 1)) == b.at(Key(rangeMin.val() + 1)));
              if (Key(std::numeric_limits<Key>::lowest()) < rangeMax && (rangeMax.val() - 1) > rangeMin.val())
                TEST_MACRO(a.at(Key(rangeMax.val() - 1)) == b.at(Key(rangeMax.val() - 1)));
              TEST_MACRO(a.at(Key(rangeMax.val())) == b.at(Key(rangeMax.val())));
              if (rangeMax < Key(std::numeric_limits<Key>::max()))
                TEST_MACRO(a.at(Key(rangeMax.val() + 1)) == b.at(Key(rangeMax.val() + 1)));
            }
          };

          // Update old tests
          for (auto& kv : tests) {
            if (rangeMin.val() <= kv.first.val() && kv.first < rangeMax) {
              kv.second = rangeVal;
            }
          }

          // Generate some tests within this range
          {
            std::uniform_int_distribution<int> rangeDist(rangeMin.val(), rangeMax.val());
            for (int i = 0; i < 1000; ++i) {
              Key testKey = Key(rangeDist(mt));
              if (rangeMin.val() <= testKey.val() && testKey < rangeMax) {
                // Check the value has changed
                tests.insert(std::make_pair(testKey, rangeVal));
              }
              else {
                // Make sure the value hasn't changed
                tests.insert(std::make_pair(testKey, m[testKey]));
              }
            }
          }

          // Generate some tests within all ranges
          for (int i = 0; i < 1000; ++i) {
            Key testKey = Key(keyDist(mt));
            if (rangeMin.val() <= testKey.val() && testKey < rangeMax) {
              // Check the value has changed
              tests.insert(std::make_pair(testKey, rangeVal));
            }
            else {
              // Make sure the value hasn't changed
              tests.insert(std::make_pair(testKey, m[testKey]));
            }
          }

          // Get current boundary conditions for range
          std::map<Key, Val> oldBoundaries;
          saveBoundaries(oldBoundaries, rangeMin, rangeMax);

          // Assign random range
          m.assign(rangeMin, rangeMax, rangeVal);

          // Test boundaries
          std::map<Key, Val> newBoundaries;
          saveBoundaries(newBoundaries, rangeMin, rangeMax);

          // Compare boundaries before and after
          compareBoundaries(oldBoundaries, newBoundaries, rangeMin, rangeMax, rangeVal);

          // Run tests
          {
            INFO("running all the current tests");
            runTests();
          }

          // Recurse
          testRandomRange(randomMin, randomMax, depth+1, maxDepth);
        }
      };

      // Do tests, clearing each time
      // todo: try like 0,0 0,1
      testRandomRange(Key(0), Key(0), 0, 5);
      resetTests();
      testRandomRange(Key(std::numeric_limits<Key>::lowest()), Key(std::numeric_limits<Key>::lowest()), 0, 5);
      resetTests();
      testRandomRange(Key(std::numeric_limits<Key>::max()), Key(std::numeric_limits<Key>::max()), 0, 5);
      resetTests();
      testRandomRange(Key(-1000), Key(1000), 0, 5);
      resetTests();
      testRandomRange(Key(std::numeric_limits<Key>::lowest()), Key(std::numeric_limits<Key>::lowest()+100), 0, 5);
      resetTests();
      testRandomRange(Key(std::numeric_limits<Key>::max()-100), Key(std::numeric_limits<Key>::max()), 0, 5);
      resetTests();
      testRandomRange(Key(std::numeric_limits<Key>::lowest()), Key(std::numeric_limits<Key>::max()), 0, 5);
      resetTests();

      // Do tests again without clearing
      testRandomRange(Key(0), Key(0), 0, 5);
      testRandomRange(Key(std::numeric_limits<Key>::lowest()), Key(std::numeric_limits<Key>::lowest()), 0, 5);
      testRandomRange(Key(std::numeric_limits<Key>::max()), Key(std::numeric_limits<Key>::max()), 0, 5);
      testRandomRange(Key(0), Key(10), 0, 5);
      testRandomRange(Key(-1000), Key(1000), 0, 5);
      testRandomRange(Key(std::numeric_limits<Key>::lowest()), Key(std::numeric_limits<Key>::lowest()+100), 0, 5);
      testRandomRange(Key(std::numeric_limits<Key>::max()-100), Key(std::numeric_limits<Key>::max()), 0, 5);
      testRandomRange(Key(std::numeric_limits<Key>::lowest()), Key(std::numeric_limits<Key>::max()), 0, 5);
      resetTests();

      // Do tests with random ranges resetting each time
      for (int i = 0; i < 10; ++i) {
        const Key min = std::numeric_limits<Key>::lowest();
        const Key max = std::numeric_limits<Key>::max();
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> keyDist(min.val(), max.val());

        Key testMin = keyDist(mt);
        Key testMax = keyDist(mt);
        if (testMax < testMin)
          std::swap(testMin, testMax);

        testRandomRange(testMin, testMax, 0, 5);
        resetTests();
      }

      // Do tests with random ranges without resetting each time
      for (int i = 0; i < 5; ++i) {
        const Key min = std::numeric_limits<Key>::lowest();
        const Key max = std::numeric_limits<Key>::max();
        std::random_device rd;
        std::mt19937 mt(rd());
        std::uniform_int_distribution<int> keyDist(min.val(), max.val());

        Key testMin = keyDist(mt);
        Key testMax = keyDist(mt);
        if (testMax < testMin)
          std::swap(testMin, testMax);

        testRandomRange(testMin, testMax, 0, 5);
      }
    }
  }
}