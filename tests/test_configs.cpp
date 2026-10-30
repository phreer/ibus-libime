#include "configs.h"

#include <glib/gstdio.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::string read(const std::filesystem::path &path) {
  std::ifstream input(path);
  return {(std::istreambuf_iterator<char>(input)), {}};
}
} // namespace

int main() {
  gchar *temporary = g_dir_make_tmp("ibus-libime-config-XXXXXX", nullptr);
  if (!temporary) {
    return 1;
  }
  const std::filesystem::path directory(temporary);
  g_free(temporary);
  int result = 0;
  try {
    g_setenv("XDG_CONFIG_HOME", directory.c_str(), TRUE);
    auto &config = Config::getInstance();
    auto settings = config.getSettings();
    require(settings.nbest == 3 && settings.pageSize == 9 &&
                settings.recordInputHistory && settings.learnUserDictionary &&
                settings.englishCandidates && settings.fuzzyFlags.empty() &&
                settings.logLevel.empty(),
            "Missing configuration must expose engine defaults");
    const auto path = directory / "ibus-libime.ini";
    config.watch();
    GError *error = nullptr;
    require(config.saveSettings(settings, &error),
            error ? error->message : "Cannot create initial configuration");
    {
      std::ofstream output(path);
      output << "# Keep this comment\n[other]\nunknown=value\n"
                "label[zh_CN]=translated\n";
    }
    settings.nbest = 5;
    settings.pageSize = 7;
    settings.fuzzyFlags = "Inner, AN_ANG";
    settings.logLevel = "WARN";
    settings.recordInputHistory = false;
    settings.learnUserDictionary = false;
    settings.englishCandidates = false;
    require(config.saveSettings(settings, &error),
            error ? error->message : "Cannot save settings");
    auto loaded = config.getSettings();
    require(loaded.nbest == 5 && loaded.pageSize == 7 &&
                loaded.logLevel == "WARN" && !loaded.recordInputHistory &&
                !loaded.learnUserDictionary && !loaded.englishCandidates &&
                config.getFuzzyFlags() == ((1 << 14) | (1 << 2)),
            "Saved settings must round-trip through engine configuration");
    require(read(path).find("# Keep this comment") != std::string::npos &&
                read(path).find("unknown=value") != std::string::npos &&
                read(path).find("label[zh_CN]=translated") != std::string::npos,
            "Saving must retain comments, localized keys and unknown groups");
    struct stat status{};
    require(g_stat(path.c_str(), &status) == 0 &&
                (status.st_mode & 0777) == 0600,
            "New configuration must be private");
    const auto before = read(path);
    settings.fuzzyFlags = "TypoNotSupported";
    require(!config.saveSettings(settings, &error) && error &&
                read(path) == before,
            "Invalid fuzzy flags must fail without modifying configuration");
    g_clear_error(&error);
    settings.fuzzyFlags.clear();
    settings.pageSize = 0;
    require(!config.saveSettings(settings, &error) && error &&
                read(path) == before,
            "Invalid numeric values must fail without modifying configuration");
    g_clear_error(&error);
    settings.pageSize = 17;
    require(!config.saveSettings(settings, &error) && error &&
                read(path) == before,
            "Page sizes above the IBus limit must be rejected");
    g_clear_error(&error);
    settings.pageSize = 9;
    settings.logLevel.clear();
    settings.recordInputHistory = true;
    settings.learnUserDictionary = true;
    settings.englishCandidates = true;
    require(config.saveSettings(settings, &error),
            error ? error->message : "Cannot restore defaults");
    loaded = config.getSettings();
    require(loaded.logLevel.empty() && loaded.recordInputHistory &&
                loaded.learnUserDictionary && loaded.englishCandidates &&
                config.getFuzzyFlags() == 0 &&
                config.getEffectiveFuzzyFlags() == ((1 << 14) | 1),
            "Cleared/default settings and enabled switches must round-trip");

    settings.fuzzyFlags = "None";
    require(config.saveSettings(settings, &error),
            error ? error->message : "Cannot disable fuzzy matching");
    require(config.getSettings().fuzzyFlags == "None" &&
                config.getEffectiveFuzzyFlags() == 0,
            "Explicitly disabling all fuzzy rules must not restore defaults");
    settings.fuzzyFlags = "None,C_CH";
    require(!config.saveSettings(settings, &error) && error &&
                config.getEffectiveFuzzyFlags() == 0,
            "Disabling all rules cannot be combined with selected rules");
    g_clear_error(&error);
    settings.fuzzyFlags = "C_CH,AN_ANG,L_R";
    require(config.saveSettings(settings, &error),
            error ? error->message : "Cannot save multiple fuzzy options");
    require(config.getEffectiveFuzzyFlags() ==
                ((1 << 8) | (1 << 2) | (1 << 20)),
            "Multiple fuzzy selections must combine their matching rules");
    settings.fuzzyFlags.clear();
    require(config.saveSettings(settings, &error),
            error ? error->message : "Cannot restore recommended rules");
    require(config.getEffectiveFuzzyFlags() == ((1 << 14) | 1),
            "Recommended mode must restore built-in fuzzy rules");

    const char *external =
        "[general]\nrecordinputhistory=false\nlearnuserdictionary=false\n"
        "nbest=8\npagesize=4\nfuzzyflags=EN_ENG\n";
    require(g_file_set_contents_full(path.c_str(), external, -1,
                                     G_FILE_SET_CONTENTS_CONSISTENT, 0600,
                                     &error),
            "Cannot simulate atomic external settings replacement");
    const auto deadline = g_get_monotonic_time() + 3 * G_TIME_SPAN_SECOND;
    while (config.getRecordInputHistory() &&
           g_get_monotonic_time() < deadline) {
      while (g_main_context_iteration(nullptr, FALSE)) {
      }
      g_usleep(1000);
    }
    require(!config.getRecordInputHistory() &&
                !config.getLearnUserDictionary() && config.getNBest() == 8 &&
                config.getPageSize() == 4 && config.getFuzzyFlags() == (1 << 3),
            "File monitoring must reload externally saved settings");
    {
      std::ofstream output(path);
      output << "not a valid key file\n";
    }
    const auto invalid = read(path);
    require(!config.saveSettings(settings, &error) && error &&
                read(path) == invalid,
            "Saving must not overwrite an unreadable configuration");
    g_clear_error(&error);
    std::filesystem::remove(path);
    std::filesystem::create_directory(path);
    require(!config.saveSettings(settings, &error) && error,
            "File I/O failures must be reported");
    g_clear_error(&error);
    std::cout << "Configuration persistence and live reload tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    result = 1;
  }
  std::filesystem::remove_all(directory);
  return result;
}
