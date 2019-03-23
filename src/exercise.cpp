#include <catch.hpp>

#include <map>
#include <limits>

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

// Key type that implements only the specified operations
class Val {
  char m_val;

public:
  Val(char v) : m_val(v) {}
  Val(const Val& v) { m_val = v.m_val; }
  Val& operator=(const Val& v) { m_val = v.m_val; return *this; }
  bool operator==(const Val& v) const { return m_val == v.m_val; }

  int val() const { return m_val; };
};

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
    if (!(keyBegin < keyEnd))
      return;
    const V endVal = (*this)[keyEnd];
    m_map.erase(m_map.lower_bound(keyBegin), m_map.upper_bound(keyEnd));
    m_map.insert(std::make_pair(keyBegin, val));
    m_map.insert(std::make_pair(keyEnd, endVal));
  }

  // look-up of the value associated with key
  V const& operator[](K const& key) const {
    return (--m_map.upper_bound(key))->second;
  }

  // little backdoor for verifying canonical representation in tests
  const std::map<K, V>& map() const { return m_map; }
};

// Unit tests
#include <random>
#include <optional>

TEST_CASE("interval_map") {
  SECTION("Key type") {
    CHECK(Key(4) < Key(5));
    CHECK_FALSE(Key(5) < Key(5));
    CHECK(Key(5) < Key(6));

    Key key1(99);
    Key key2(100);
    Key key3(101);
    CHECK_FALSE(Key(key1) < key1);
    CHECK(Key(key2) < key3);
    CHECK_FALSE(Key(key2) < key1);
  }

  SECTION("Val type") {
    CHECK_FALSE(Val(4) == Val(5));
    CHECK(Val(5) == Val(5));
    CHECK_FALSE(Val(6) == Val(5));

    Val val1(99);
    Val val2(100);
    CHECK(Val(val1) == val1);
    CHECK_FALSE(Val(val1) == val2);
  }

  SECTION("interval_map Constructor") {
    interval_map<Key, Val> m(Val(5));

    Key key = std::numeric_limits<Key>::lowest();

    CHECK_FALSE(m[key] == Val(4));
    CHECK(m[key] == Val(5));
    CHECK_FALSE(m[key] == Val(6));

    CHECK(m.map().size() == 1);
  }

  SECTION("their example") {
    interval_map<Key, Val> m(Val('A'));
    m.assign(Key(3), Key(5), Val('B'));

    CHECK(m[Key(0)] == 'A');
    CHECK(m[Key(1)] == 'A');
    CHECK(m[Key(2)] == 'A');
    CHECK(m[Key(3)] == 'B');
    CHECK(m[Key(4)] == 'B');
    CHECK(m[Key(5)] == 'A');
    CHECK(m[Key(6)] == 'A');
    CHECK(m[Key(7)] == 'A');

    CHECK(m.map().size() == 3);
  }

  SECTION("interval_map with the initial range (min, max) -> 'a'") {
    interval_map<Key, Val> m(Val('a'));

    auto checkCanonicity = [](interval_map<Key,Val>& m) {
    };

    SECTION("with the additional range (10, 100) -> 'b'") {
      const Key min = std::numeric_limits<Key>::lowest();
      const Key max = std::numeric_limits<Key>::max();

      // Check min, max, and arbitrary middle values
      CHECK(m[min] == Val('a'));
      CHECK(m[Key(0)] == Val('a'));
      CHECK(m[Key(1000)] == Val('a'));
      CHECK(m[max] == Val('a'));

      // Assign a range to a different value
      m.assign(Key(10), Key(100), Val('b'));

      // Check our keys again
      CHECK(m[min] == Val('a'));
      CHECK(m[Key(0)] == Val('a'));
      CHECK(m[Key(1000)] == Val('a'));
      CHECK(m[max] == Val('a'));

      // Check boundaries
      CHECK(m[Key(9)] == Val('a'));
      CHECK(m[Key(10)] == Val('b'));
      CHECK(m[Key(11)] == Val('b'));
      CHECK(m[Key(99)] == Val('b'));
      CHECK(m[Key(100)] == Val('a'));

      CHECK(m.map().size() == 3);

      SECTION("Assigning a range that matches the value of the existing range") {
        CHECK(m.map().size() == 3);
        m.assign(Key(1000), Key(2000), Val('a'));
        CHECK(m.map().size() == 3);
      }

      SECTION("Assigning a range that includes a but doesn't touch the next range") {
        // Assign a range that corresponds with our a key
        m.assign(Key(-10), Key(9), Val('c'));

        // Check our keys again
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('c'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));

        // Check boundaries
        CHECK(m[Key(-11)] == Val('a'));
        CHECK(m[Key(-10)] == Val('c'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));

        CHECK(m.map().size() == 5);
      }

      SECTION("Assigning a range that includes a and touches the next range") {
        // Assign a range that corresponds with our a key
        m.assign(Key(-10), Key(10), Val('c'));

        // Check our keys again
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('c'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));

        // Check boundaries
        CHECK(m[Key(-11)] == Val('a'));
        CHECK(m[Key(-10)] == Val('c'));
        CHECK(m[Key(9)] == Val('c'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));

        CHECK(m.map().size() == 4);
      }

      SECTION("try a range where !(begin < end)") {
        // Assign a range with !(begin < end), which should designate an empty
        // interval and do nothing
        m.assign(Key(50), Key(0), Val('c'));

        // Check that the previous is still correct
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(10000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Check boundaries
        CHECK(m[Key(49)] == Val('b'));
        CHECK(m[Key(50)] == Val('b'));
        CHECK(m[Key(51)] == Val('b'));
        CHECK(m[Key(-1)] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1)] == Val('a'));
      }

      SECTION("the whole range can be replaced") {
        m.assign(min, max, Val('c'));

        // Check previous values have all updated
        CHECK(m[min] == Val('c'));
        CHECK(m[Key(0)] == Val('c'));
        CHECK(m[Key(1000)] == Val('c'));
        // todo: I think it's weird that you can't change the interval_map's full
        // range using assign, but because keyEnd is exclusive, the max key value
        // will always return the Val('a') value for the map
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('c'));
        CHECK(m[Key(10)] == Val('c'));
        CHECK(m[Key(11)] == Val('c'));
        CHECK(m[Key(99)] == Val('c'));
        CHECK(m[Key(100)] == Val('c'));
      }

      SECTION("try overwriting an existing range from the start") {
        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Overwrite the start of the range 10,100
        m.assign(Key(5), Key(15), Val('c'));

        // Check previous values have updated where appropriate
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('c'));
        CHECK(m[Key(10)] == Val('c'));
        CHECK(m[Key(11)] == Val('c'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Check boundary conditions
        CHECK(m[Key(4)] == Val('a'));
        CHECK(m[Key(5)] == Val('c'));
        for (int i = 5; i < 15; ++i) {
          CHECK(m[Key(i)] == Val('c'));
        }
        CHECK(m[Key(14)] == Val('c'));
        CHECK(m[Key(15)] == Val('b'));
        CHECK(m[Key(16)] == Val('b'));
      }

      SECTION("try overwriting an existing range from the end") {
        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Overwrite the start of the range 10,100
        m.assign(Key(95), Key(105), Val('c'));

        // Check previous values have updated where appropriate
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('c'));
        CHECK(m[Key(100)] == Val('c'));

        // Check boundary conditions
        CHECK(m[Key(94)] == Val('b'));
        CHECK(m[Key(95)] == Val('c'));
        for (int i = 95; i < 104; ++i) {
          CHECK(m[Key(i)] == Val('c'));
        }
        CHECK(m[Key(104)] == Val('c'));
        CHECK(m[Key(105)] == Val('a'));
        CHECK(m[Key(106)] == Val('a'));
      }

      SECTION("try overwriting an existing range in the center") {
        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Overwrite the start of the range 10,100
        m.assign(Key(45), Key(55), Val('c'));

        // Check previous values are the same
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Check boundary conditions
        CHECK(m[Key(44)] == Val('b'));
        CHECK(m[Key(45)] == Val('c'));
        for (int i = 45; i < 54; ++i) {
          CHECK(m[Key(i)] == Val('c'));
        }
        CHECK(m[Key(54)] == Val('c'));
        CHECK(m[Key(55)] == Val('b'));
        CHECK(m[Key(56)] == Val('b'));
      }

      SECTION("assign to max value") {
        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Overwrite max
        m.assign(max, max, Val('c'));

        // Check interval map hasn't changed
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));
      }

      SECTION("assign around the min value") {
        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        for (int i = min.val(); i < min.val() + 5; ++i) {
          CHECK(m[Key(i)] == Val('a'));
        }
        CHECK(m[Key(min.val() + 5)] == Val('a'));

        CHECK(m.map().size() == 3);

        // Overwrite min
        m.assign(Key(min.val()), Key(min.val() + 5), Val('c'));

        // Ensure previous state
        CHECK(m[min] == Val('c'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        // Check boundaries have changed
        for (int i = min.val(); i < min.val() + 5; ++i) {
          CHECK(m[Key(i)] == Val('c'));
        }
        CHECK(m[Key(min.val() + 5)] == Val('a'));

        CHECK(m.map().size() == 4);

        SECTION("restore the map to the original state with the same operation but the original value") {
          m.assign(Key(min.val()), Key(min.val() + 5), Val('a'));

          // Ensure previous state
          CHECK(m[min] == Val('a'));
          CHECK(m[Key(0)] == Val('a'));
          CHECK(m[Key(1000)] == Val('a'));
          CHECK(m[max] == Val('a'));
          CHECK(m[Key(9)] == Val('a'));
          CHECK(m[Key(10)] == Val('b'));
          CHECK(m[Key(11)] == Val('b'));
          CHECK(m[Key(99)] == Val('b'));
          CHECK(m[Key(100)] == Val('a'));

          for (int i = min.val(); i < min.val() + 5; ++i) {
            CHECK(m[Key(i)] == Val('a'));
          }
          CHECK(m[Key(min.val() + 5)] == Val('a'));

          CHECK(m.map().size() == 3);
        }

        SECTION("restore the map to the original state with a slightly wider operation") {
          m.assign(Key(min.val()), Key(min.val() + 10), Val('a'));

          // Ensure previous state
          CHECK(m[min] == Val('a'));
          CHECK(m[Key(0)] == Val('a'));
          CHECK(m[Key(1000)] == Val('a'));
          CHECK(m[max] == Val('a'));
          CHECK(m[Key(9)] == Val('a'));
          CHECK(m[Key(10)] == Val('b'));
          CHECK(m[Key(11)] == Val('b'));
          CHECK(m[Key(99)] == Val('b'));
          CHECK(m[Key(100)] == Val('a'));

          for (int i = min.val(); i < min.val() + 5; ++i) {
            CHECK(m[Key(i)] == Val('a'));
          }
          CHECK(m[Key(min.val() + 5)] == Val('a'));

          CHECK(m.map().size() == 3);
        }

        SECTION("restore the min value, leaving only one 'c'") {
          m.assign(Key(min.val()), Key(min.val() + 4), Val('a'));

          // Ensure previous state
          CHECK(m[min] == Val('a'));
          CHECK(m[Key(0)] == Val('a'));
          CHECK(m[Key(1000)] == Val('a'));
          CHECK(m[max] == Val('a'));
          CHECK(m[Key(9)] == Val('a'));
          CHECK(m[Key(10)] == Val('b'));
          CHECK(m[Key(11)] == Val('b'));
          CHECK(m[Key(99)] == Val('b'));
          CHECK(m[Key(100)] == Val('a'));

          for (int i = min.val(); i < min.val() + 4; ++i) {
            CHECK(m[Key(i)] == Val('a'));
          }
          CHECK(m[Key(min.val() + 4)] == Val('c'));
          CHECK(m[Key(min.val() + 5)] == Val('a'));

          CHECK(m.map().size() == 5);
        }
      }

      SECTION("assign to just before the max value") {
        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        for (int i = max.val() - 5; i < max.val(); ++i) {
          CHECK(m[Key(i)] == Val('a'));
        }

        CHECK(m.map().size() == 3);

        // Overwrite max
        m.assign(Key(max.val() - 5), max, Val('c'));

        // Check interval map has only changed where it should
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        for (int i = max.val() - 5; i < max.val(); ++i) {
          CHECK(m[Key(i)] == Val('c'));
        }

        CHECK(m.map().size() == 5);

        // Try and restore old map state
        m.assign(Key(max.val() - 5), max, Val('a'));

        // Ensure previous state
        CHECK(m[min] == Val('a'));
        CHECK(m[Key(0)] == Val('a'));
        CHECK(m[Key(1000)] == Val('a'));
        CHECK(m[max] == Val('a'));
        CHECK(m[Key(9)] == Val('a'));
        CHECK(m[Key(10)] == Val('b'));
        CHECK(m[Key(11)] == Val('b'));
        CHECK(m[Key(99)] == Val('b'));
        CHECK(m[Key(100)] == Val('a'));

        for (int i = max.val() - 5; i < max.val(); ++i) {
          CHECK(m[Key(i)] == Val('a'));
        }

        CHECK(m.map().size() == 3);
      }
    }

    SECTION("random tests") {
      // todo: try with min,min and max,max?
      const Key randomMin = Key(-1000);
      const Key randomMax = Key(1000);

      std::random_device rd;
      std::mt19937 mt(rd());
      std::uniform_int_distribution<int> keyDist(randomMin.val(), randomMax.val());
      std::uniform_int_distribution<int> valDist('A', 'z');

      std::map<Key, Val> tests;
      auto runTests = [&tests, &m]() {
        for (const auto& test : tests) {
          CHECK(m[test.first] == test.second);
        }
      };

      SECTION("a random range is added") {
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
          REQUIRE(a.size() == a.size());
          if (Key(std::numeric_limits<Key>::lowest()) < rangeMin)
            CHECK(a.at(Key(rangeMin.val() - 1)) == b.at(Key(rangeMin.val() - 1)));
          CHECK(b.at(Key(rangeMin.val())) == rangeVal);
          if (rangeMin < Key(std::numeric_limits<Key>::max()) && (rangeMin.val() + 1) < rangeMax.val())
            CHECK(b.at(Key(rangeMin.val() + 1)) == rangeVal);
          if (Key(std::numeric_limits<Key>::lowest()) < rangeMax && (rangeMax.val() - 1) > rangeMin.val())
            CHECK(b.at(Key(rangeMax.val() - 1)) == rangeVal);
          CHECK(a.at(Key(rangeMax.val())) == b.at(Key(rangeMax.val())));
          if (rangeMax < Key(std::numeric_limits<Key>::max()))
            CHECK(a.at(Key(rangeMax.val() + 1)) == b.at(Key(rangeMax.val() + 1)));
        };

        // Generate range
        Key rangeMin = Key(keyDist(mt));
        Key rangeMax = Key(keyDist(mt));
        Val rangeVal = Val(valDist(mt));

        // todo: make sure the range isn't 0 or add support for it above
        if (rangeMax < rangeMin)
          std::swap(rangeMin, rangeMax);

        // Generate some tests within this range
        {
          std::uniform_int_distribution<int> rangeDist(rangeMin.val(), rangeMax.val());
          for (int i = 0; i < 10; ++i) {
            Key testKey = Key(rangeDist(mt));
            // Make sure they have the new value
            tests.insert(std::make_pair(testKey, rangeVal));
          }
        }

        // Generate some tests within all ranges
        for (int i = 0; i < 10; ++i) {
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
        runTests();
      }
    }
  }
}