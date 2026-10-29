#include "english_dictionary.h"

#include <glib.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    return 1;
  }
  try {
    EnglishDictionary dictionary(argv[1]);
    require(dictionary.complete("he").empty(), "Short prefix must be ignored");
    require(dictionary.complete("zhi").empty(),
            "Three-letter input must not produce English candidates");
    require(dictionary.complete("zhiv").empty(),
            "Low-frequency prefix completion must be filtered");
    require(dictionary.complete("zhivago") ==
                std::vector<std::string>{"zhivago"},
            "Low-frequency full word must remain selectable");
    require(dictionary.complete("hel'lo").empty(), "Separator must be ignored");
    require(dictionary.complete("HEL").empty(), "Uppercase is out of scope");
    require(dictionary.complete("zzzzzzzz").empty(),
            "Unknown prefix must be empty");
    require(dictionary.complete("comp", 0).empty(),
            "Zero limit must be respected");
    auto exact = dictionary.complete("hello");
    require(!exact.empty() && exact.front() == "hello",
            "Exact match must rank first");
    auto words = dictionary.complete("comp");
    require(words == std::vector<std::string>{"comp", "company"},
            "Completion must use frequency ranking and limit");
    require(dictionary.complete("comp", 3) ==
                std::vector<std::string>{"comp", "company", "completely"},
            "Frequency ranking must follow the exact match");
    for (const auto &word : words) {
      require(word.starts_with("comp"), "Every result must match the prefix");
    }
    EnglishDictionary technical(
        argv[1],
        (std::filesystem::path(argv[1]).parent_path() / "english-technical.txt")
            .string());
    require(technical.complete("android") ==
                std::vector<std::string>{"Android", "android"},
            "Canonical spelling and lowercase variant must both be retained");
    require(technical.complete("andro").front() == "Android",
            "Lowercase prefix must find the canonical spelling");
    const auto linuxWords = technical.complete("linu");
    require(std::find(linuxWords.begin(), linuxWords.end(), "Linux") !=
                linuxWords.end(),
            "Linux casing must be preserved");
    require(technical.complete("gith").front() == "GitHub",
            "Mixed-case technical spelling must be preserved");
    require(technical.complete("hello").front() == "hello",
            "Ordinary words must not be capitalized");
    require(EnglishDictionary::matchesWord("Android", "android") &&
                !EnglishDictionary::matchesWord("Android", "andro"),
            "Exact match must use the normalized full word");

    std::vector<double> timings;
    for (int i = 0; i < 1000; ++i) {
      auto start = std::chrono::steady_clock::now();
      dictionary.complete(i % 2 ? "cons" : "comp");
      timings.push_back(std::chrono::duration<double, std::milli>(
                            std::chrono::steady_clock::now() - start)
                            .count());
    }
    std::sort(timings.begin(), timings.end());
    std::cout << "English completion P95: " << timings[950] << " ms\n";
    require(timings[950] < 5.0, "Warm lookup P95 must be below 5 ms");

    gchar *directory = g_dir_make_tmp("ibus-libime-dictionary-XXXXXX", nullptr);
    require(directory != nullptr, "Cannot create fixture directory");
    const auto fixture = std::filesystem::path(directory) / "words.txt";
    g_free(directory);
    {
      std::ofstream output(fixture);
      output << "testing 1000\ntest 1\ntester 1000\ntester 50\n"
                "testa 1000\ntestlow 999\nbad-word 99\nGitLab 1000\n";
    }
    EnglishDictionary small(fixture.string());
    require(
        small.complete("test", 4) ==
            std::vector<std::string>{"test", "testa", "tester", "testing"},
        "Exact match, ties, length and deduplication must be deterministic");
    require(small.complete("gitl") == std::vector<std::string>{"GitLab"},
            "Base dictionary entries must retain mixed-case spelling too");
    require(small.complete("test", 10) ==
                std::vector<std::string>{"test", "testa", "tester", "testing"},
            "Frequency 999 must be excluded and 1000 included for prefixes");
    require(small.complete("testlow") == std::vector<std::string>{"testlow"},
            "Low-frequency exact match must be exempt from the threshold");
    for (const auto *invalid : {"broken not-a-frequency\n", "broken",
                                "broken -1\n", "broken 1 extra\n", ""}) {
      {
        std::ofstream output(fixture);
        output << invalid;
      }
      bool rejected = false;
      try {
        EnglishDictionary invalidDictionary(fixture.string());
      } catch (const std::runtime_error &) {
        rejected = true;
      }
      require(rejected, "Malformed dictionary must be rejected");
    }
    std::filesystem::remove(fixture);
    bool rejected = false;
    try {
      EnglishDictionary missing(fixture.string());
    } catch (const std::runtime_error &) {
      rejected = true;
    }
    std::filesystem::remove(fixture.parent_path());
    require(rejected, "Missing dictionary must be rejected");
    std::cout << "Dictionary tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
