#ifndef IBUS_LIBIME_ENGLISH_DICTIONARY_H
#define IBUS_LIBIME_ENGLISH_DICTIONARY_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class EnglishDictionary {
public:
  explicit EnglishDictionary(const std::string &path,
                             const std::string &supplementPath = {});
  static bool matchesWord(std::string_view word, std::string_view input);
  std::vector<std::string> complete(std::string_view prefix,
                                    size_t limit = 2) const;

private:
  struct Entry {
    std::string word;
    std::string key;
    uint64_t frequency;
    bool preferred = false;
  };
  std::vector<Entry> entries_;
};

#endif
