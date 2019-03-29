#pragma once
#include "strings.h"
#include <string> using namespace std;

namespace cobb {
   typedef uint32_t unicodechar;
   namespace utf8 {
      typedef uint32_t rawchar;
      constexpr unicodechar invalid_glyph = 0xFFFD;

      void        advance(const std::string&, std::string::iterator&);
      void        append(std::string&, const unicodechar);
      size_t      count(std::string&);
      size_t      count_from(const std::string&, const std::string::iterator&);
      unicodechar get(const std::string&, const std::string::iterator&);
      unicodechar get(const std::string&, const std::string::const_iterator&);
      rawchar     get_raw(const std::string&, const std::string::iterator&);
   }
};