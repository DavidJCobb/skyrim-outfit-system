#include "utf8string.h"

namespace cobb {
   namespace utf8 {
      inline static bool _is_bytecount(char c) {
         return c < 0xF8;
      }
      inline static bool _is_continuation(char c) {
         return ((c & 0xC0) == 0x80);
      }
      //
      void advance(const std::string& container, std::string::iterator& iterator) {
         if (iterator == container.end())
            return;
         unsigned char c;
         //
         c = iterator[0];
         ++iterator;
         if (c < 0x80) // single-byte glyph
            return;
         if (!_is_bytecount(c)) // error
            return;
         //
         c = iterator[0];
         ++iterator;
         if (!_is_continuation(c)) // error
            return;
         //
         c = iterator[0];
         ++iterator;
         if (!_is_continuation(c)) // error
            return;
         //
         c = iterator[0];
         ++iterator;
      }
      void append(std::string& target, const unicodechar c) {
         if (c < 0x80) {
            target.push_back(c);
            return;
         }
         if (c < 0x800) {
            target.push_back(0xE0 | ((c >> 6) & 0x1F));
            target.push_back(0x80 | (c & 0x3F));
            return;
         }
         if (c < 0x10000) {
            target.push_back(0xE0 | ((c >> 12) & 0xF));
            target.push_back(0x80 | ((c >> 6) & 0x3F));
            target.push_back(0x80 | (c & 0x3F));
            return;
         }
         target.push_back(0xE0 | ((c >> 18) & 0x7));
         target.push_back(0x80 | ((c >> 12) & 0x3F));
         target.push_back(0x80 | ((c >> 6) & 0x3F));
         target.push_back(0x80 | (c & 0x1F));
      }
      size_t count(std::string& container) {
         return count_from(container, container.begin());
      }
      size_t count_from(const std::string& container, const std::string::iterator& iterator) {
         std::string::iterator working = iterator;
         size_t count = 0;
         while (working != container.end()) {
            advance(container, working);
            count++;
         }
         return count;
      }
      unicodechar get(const std::string& container, const std::string::const_iterator& iterator) {
         if (iterator == container.end())
            return 0;
         unsigned char a = iterator[0];
         if (a < 0x80)
            return a;
         if (!_is_bytecount(a))
            return invalid_glyph;
         //
         // Two bytes:
         //
         UInt32 result;
         char b = iterator[1];
         if (!_is_continuation(b))
            return invalid_glyph;
         if ((a & 0xE0) == 0xC0) {
            result = (a & 0x1F) << 6;
            result |= (b & 0x3F);
            return result;
         }
         //
         // Three bytes:
         //
         char c = iterator[2];
         if (!_is_continuation(c))
            return invalid_glyph;
         if ((a & 0xF0) == 0xE0) { // three bytes
            result = (a & 0xF) << 12;
            result |= (b & 0x3F) << 6;
            result |= (c & 0x3F);
            if (result >= 0xD800 && result <= 0xDFFF)
               return invalid_glyph;
            return result;
         }
         //
         // Four bytes:
         //
         char d = iterator[3];
         if (!_is_continuation(d))
            return invalid_glyph;
         result = (a & 7) << 18;
         result |= (b & 0x3F) << 12;
         result |= (c & 0x3F) << 6;
         result |= (d & 0x3F);
         if (result > 0x10FFFF)
            return invalid_glyph;
         return result;
      }
      unicodechar get(const std::string& container, const std::string::iterator& iterator) {
         if (iterator == container.end())
            return 0;
         unsigned char a = iterator[0];
         if (a < 0x80)
            return a;
         if (!_is_bytecount(a))
            return invalid_glyph;
         //
         // Two bytes:
         //
         UInt32 result;
         char b = iterator[1];
         if (!_is_continuation(b))
            return invalid_glyph;
         if ((a & 0xE0) == 0xC0) {
            result = (a & 0x1F) << 6;
            result |= (b & 0x3F);
            return result;
         }
         //
         // Three bytes:
         //
         char c = iterator[2];
         if (!_is_continuation(c))
            return invalid_glyph;
         if ((a & 0xF0) == 0xE0) { // three bytes
            result = (a & 0xF) << 12;
            result |= (b & 0x3F) << 6;
            result |= (c & 0x3F);
            if (result >= 0xD800 && result <= 0xDFFF)
               return invalid_glyph;
            return result;
         }
         //
         // Four bytes:
         //
         char d = iterator[3];
         if (!_is_continuation(d))
            return invalid_glyph;
         result = (a & 7) << 18;
         result |= (b & 0x3F) << 12;
         result |= (c & 0x3F) << 6;
         result |= (d & 0x3F);
         if (result > 0x10FFFF)
            return invalid_glyph;
         return result;
      }
      rawchar get_raw(const std::string& container, const std::string::iterator& iterator) {
         if (iterator == container.end())
            return 0;
         char a = iterator[0];
         char b = iterator[1];
         char c = iterator[2];
         char d = iterator[3];
         if (!_is_bytecount(a))
            return invalid_glyph;
         {  // 2 bytes
            if (!_is_continuation(b))
               return invalid_glyph;
            if ((a & 0xE0) == 0xC0)
               return (a << 0x08) | b;
         }
         {  // 3 bytes
            if (!_is_continuation(c))
               return invalid_glyph;
            if ((a & 0xF0) == 0xE0)
               return (a << 0x10) | (b << 0x08) | c;
         }
         {  // 4 bytes
            if (!_is_continuation(d))
               return invalid_glyph;
            return (a << 0x18) | (b << 0x10) | (c << 0x08) | d;
         }
      }
   }
};