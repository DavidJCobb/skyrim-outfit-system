#include "tests.h"
#include "string.h"
#include "utf8string.h"
#include "utf8naturalsort.h"
#include <algorithm>
#include <string>
#include <vector>

namespace cobb {
   void run_misc_tests() {
      _MESSAGE("UTF8NaturalSort unit tests!");
      std::vector<std::string> result;
      result.push_back("Vampire");
      result.push_back("Greybeard");
      result.push_back("Elven");
      result.push_back("test");
      result.push_back("asdf");
      _MESSAGE("Unsorted list:");
      for (auto it = result.begin(); it != result.end(); ++it) {
         _MESSAGE(" - %s", it->c_str());
      }
      std::sort(
         result.begin(),
         result.end(),
         [](const std::string& a, const std::string& b) {
            return cobb::utf8::naturalcompare(a, b) > 0;
         }
      );
      _MESSAGE("Sorted list:");
      for (auto it = result.begin(); it != result.end(); ++it) {
         _MESSAGE(" - %s", it->c_str());
      }
      _MESSAGE("UTF8NaturalSort unit tests complete!");
   }
}