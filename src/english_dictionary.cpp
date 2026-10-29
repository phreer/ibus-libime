#include "english_dictionary.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {
bool isLowercaseWord(std::string_view word) {
  return !word.empty() && std::all_of(word.begin(), word.end(), [](char ch) {
    return ch >= 'a' && ch <= 'z';
  });
}

std::string lowercase(std::string_view word) {
  std::string result(word);
  for (char &ch : result) {
    if (ch >= 'A' && ch <= 'Z') {
      ch += 'a' - 'A';
    }
  }
  return result;
}
} // namespace

bool EnglishDictionary::matchesWord(std::string_view word,
                                    std::string_view input) {
  return lowercase(word) == input;
}

EnglishDictionary::EnglishDictionary(const std::string &path,
                                     const std::string &supplementPath) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Cannot open English dictionary: " + path);
  }
  std::string line;
  while (std::getline(input, line)) {
    std::istringstream row(line);
    std::string word, count, extra;
    uint64_t frequency = 0;
    if (!(row >> word >> count) || (row >> extra)) {
      throw std::runtime_error("Invalid English dictionary row: " + path);
    }
    const auto parsed =
        std::from_chars(count.data(), count.data() + count.size(), frequency);
    if (parsed.ec != std::errc{} || parsed.ptr != count.data() + count.size()) {
      throw std::runtime_error("Invalid English dictionary frequency: " + path);
    }
    auto key = lowercase(word);
    if (isLowercaseWord(key)) {
      entries_.push_back({std::move(word), std::move(key), frequency});
    }
  }
  if (input.bad() || entries_.empty()) {
    throw std::runtime_error("Invalid English dictionary: " + path);
  }
  if (!supplementPath.empty()) {
    std::ifstream supplement(supplementPath);
    if (!supplement) {
      throw std::runtime_error("Cannot open English supplement: " +
                               supplementPath);
    }
    while (std::getline(supplement, line)) {
      if (line.empty() || line.front() == '#') {
        continue;
      }
      auto key = lowercase(line);
      if (!isLowercaseWord(key)) {
        throw std::runtime_error("Invalid English supplement: " +
                                 supplementPath);
      }
      uint64_t frequency = 0;
      for (const auto &entry : entries_) {
        if (entry.key == key) {
          frequency = std::max(frequency, entry.frequency);
        }
      }
      entries_.push_back({line, std::move(key), frequency, true});
    }
    if (supplement.bad()) {
      throw std::runtime_error("Cannot read English supplement: " +
                               supplementPath);
    }
  }
  std::sort(entries_.begin(), entries_.end(),
            [](const Entry &lhs, const Entry &rhs) {
              if (lhs.key != rhs.key) {
                return lhs.key < rhs.key;
              }
              if (lhs.word != rhs.word) {
                return lhs.word < rhs.word;
              }
              if (lhs.preferred != rhs.preferred) {
                return lhs.preferred;
              }
              return lhs.frequency > rhs.frequency;
            });
  entries_.erase(std::unique(entries_.begin(), entries_.end(),
                             [](const Entry &lhs, const Entry &rhs) {
                               return lhs.word == rhs.word;
                             }),
                 entries_.end());
}

std::vector<std::string> EnglishDictionary::complete(std::string_view prefix,
                                                     size_t limit) const {
  if (prefix.size() < 4 || !isLowercaseWord(prefix) || limit == 0) {
    return {};
  }
  auto begin = std::lower_bound(entries_.begin(), entries_.end(), prefix,
                                [](const Entry &entry, std::string_view value) {
                                  return entry.key < value;
                                });
  std::vector<const Entry *> best;
  auto better = [prefix](const Entry *lhs, const Entry *rhs) {
    if ((lhs->key == prefix) != (rhs->key == prefix)) {
      return lhs->key == prefix;
    }
    if (lhs->frequency != rhs->frequency) {
      return lhs->frequency > rhs->frequency;
    }
    if (lhs->preferred != rhs->preferred) {
      return lhs->preferred;
    }
    if (lhs->word.size() != rhs->word.size()) {
      return lhs->word.size() < rhs->word.size();
    }
    return lhs->word < rhs->word;
  };
  for (auto it = begin; it != entries_.end() && it->key.starts_with(prefix);
       ++it) {
    if (it->key != prefix && !it->preferred && it->frequency < 1000) {
      continue;
    }
    auto position = std::lower_bound(best.begin(), best.end(), &*it, better);
    best.insert(position, &*it);
    if (best.size() > limit) {
      best.pop_back();
    }
  }
  std::vector<std::string> result;
  for (const auto *entry : best) {
    result.push_back(entry->word);
  }
  return result;
}
