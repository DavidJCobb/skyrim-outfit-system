#pragma once
#include <string>

namespace cobb {
   template<class T> struct is_basic_string : std::false_type {};
   template<class Ch, class Tr, class Al> struct is_basic_string<std::basic_string<Ch, Tr, Al>> : std::true_type {};

   struct char_traits_insensitive : public std::char_traits<char> {
      inline static bool eq(char c1, char c2) {
         return tolower(c1) == tolower(c2);
      }
      inline static bool ne(char c1, char c2) {
         return tolower(c1) != tolower(c2);
      }
      inline static bool lt(char c1, char c2) {
         return tolower(c1) <  tolower(c2);
      }
      inline static int compare(const char* s1, const char* s2, size_t n) {
         return _memicmp(s1, s2, n);
      }
      inline static const char* find(const char* s, int n, char a) {
         while (n-- > 0 && tolower(*s) != tolower(a))
            ++s;
         return s;
      }
   };
   typedef std::basic_string<char, char_traits_insensitive> istring; // compares case-insensitively but stores the string as it was received
};