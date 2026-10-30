#include "configs.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <format>
#include <map>
#include <string>

namespace {
constexpr auto configFileFlags = static_cast<GKeyFileFlags>(
    G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS);

const std::map<std::string, int> fuzzyFlagMap = {
    {"CommonTypo", 1 << 0},    {"V_U", 1 << 1},
    {"AN_ANG", 1 << 2},        {"EN_ENG", 1 << 3},
    {"IAN_IANG", 1 << 4},      {"IN_ING", 1 << 5},
    {"U_OU", 1 << 6},          {"UAN_UANG", 1 << 7},
    {"C_CH", 1 << 8},          {"F_H", 1 << 9},
    {"L_N", 1 << 10},          {"S_SH", 1 << 11},
    {"Z_ZH", 1 << 12},         {"VE_UE", 1 << 13},
    {"Inner", 1 << 14},        {"InnerShort", 1 << 15},
    {"PartialFinal", 1 << 16}, {"PartialSp", 1 << 17},
    {"AdvancedTypo", 1 << 18}, {"Correction", 1 << 19},
    {"L_R", 1 << 20}};
} // namespace

Config::Config()
    : keyFile_(nullptr), logLevel_(nullptr), nbest_(3), pageSize_(9),
      fuzzyFlags_(0) {
  configPath_ = getConfigFilePath();
  keyFile_ = g_key_file_new();
  loadConfig();
}

Config::~Config() {
  if (monitor_) {
    g_file_monitor_cancel(monitor_);
    g_object_unref(monitor_);
  }
  if (logLevel_) {
    g_free(logLevel_);
    logLevel_ = nullptr;
  }
  if (keyFile_) {
    g_key_file_free(keyFile_);
    keyFile_ = nullptr;
  }
}

std::string Config::getConfigFilePath() {
  const char *config_dir = g_get_user_config_dir();
  if (!config_dir || config_dir[0] == '\0') {
    return "/tmp/ibus-libime.ini"; // Fallback
  }
  return std::format("{}/ibus-libime.ini", config_dir);
}

void Config::loadConfig() {
  GError *error = nullptr;
  GKeyFile *loaded = g_key_file_new();
  const bool exists = g_key_file_load_from_file(loaded, configPath_.c_str(),
                                                configFileFlags, &error);
  if (!exists && !g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
    g_warning("Cannot load configuration %s: %s", configPath_.c_str(),
              error->message);
    g_error_free(error);
    g_key_file_free(loaded);
    return;
  }
  g_clear_error(&error);
  g_key_file_free(keyFile_);
  keyFile_ = loaded;
  g_clear_pointer(&logLevel_, g_free);
  nbest_ = 3;
  pageSize_ = 9;
  fuzzyFlags_ = 0;
  englishCandidates_ = true;
  recordInputHistory_ = true;
  learnUserDictionary_ = true;

  if (exists) {
    // Successfully loaded, read configurations
    logLevel_ = g_key_file_get_string(keyFile_, "general", "loglevel", nullptr);

    // Read NBest (default: 3)
    error = nullptr;
    int nbest = g_key_file_get_integer(keyFile_, "general", "nbest", &error);
    if (!error && nbest > 0) {
      nbest_ = nbest;
    }
    if (error) {
      g_error_free(error);
      error = nullptr;
    }

    // Read page size (default: 9)
    int pageSize =
        g_key_file_get_integer(keyFile_, "general", "pagesize", &error);
    if (!error && pageSize > 0 && pageSize <= 16) {
      pageSize_ = pageSize;
    } else if (!error) {
      g_warning("Invalid pagesize setting: expected a value from 1 to 16");
    }
    if (error) {
      g_error_free(error);
      error = nullptr;
    }

    // Read fuzzy flags (default: 0)
    // Try to read as string first (comma-separated flag names)
    char *fuzzyFlagsStr =
        g_key_file_get_string(keyFile_, "general", "fuzzyflags", &error);
    if (!error && fuzzyFlagsStr) {
      fuzzyFlags_ = parseFuzzyFlagsString(fuzzyFlagsStr);
      g_free(fuzzyFlagsStr);
    } else {
      // Fallback: try to read as integer
      if (error) {
        g_error_free(error);
        error = nullptr;
      }
      int fuzzyFlags =
          g_key_file_get_integer(keyFile_, "general", "fuzzyflags", &error);
      if (!error) {
        fuzzyFlags_ = fuzzyFlags;
      }
    }
    if (error) {
      g_error_free(error);
      error = nullptr;
    }

    gboolean englishCandidates = g_key_file_get_boolean(
        keyFile_, "general", "englishcandidates", &error);
    if (!error) {
      englishCandidates_ = englishCandidates;
    } else {
      if (!g_error_matches(error, G_KEY_FILE_ERROR,
                           G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
        g_warning("Invalid englishcandidates setting: %s", error->message);
      }
      g_error_free(error);
      error = nullptr;
    }

    gboolean recordInputHistory = g_key_file_get_boolean(
        keyFile_, "general", "recordinputhistory", &error);
    if (!error) {
      recordInputHistory_ = recordInputHistory;
    } else {
      if (!g_error_matches(error, G_KEY_FILE_ERROR,
                           G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
        g_warning("Invalid recordinputhistory setting: %s", error->message);
      }
      g_error_free(error);
      error = nullptr;
    }

    gboolean learnUserDictionary = g_key_file_get_boolean(
        keyFile_, "general", "learnuserdictionary", &error);
    if (!error) {
      learnUserDictionary_ = learnUserDictionary;
    } else {
      if (!g_error_matches(error, G_KEY_FILE_ERROR,
                           G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
        g_warning("Invalid learnuserdictionary setting: %s", error->message);
      }
      g_error_free(error);
    }
  }
}

void Config::watch() {
  if (monitor_) {
    return;
  }
  GFile *file = g_file_new_for_path(configPath_.c_str());
  GError *error = nullptr;
  monitor_ =
      g_file_monitor_file(file, G_FILE_MONITOR_WATCH_MOVES, nullptr, &error);
  g_object_unref(file);
  if (!monitor_) {
    g_warning("Cannot monitor configuration: %s", error->message);
    g_error_free(error);
    return;
  }
  g_signal_connect(monitor_, "changed",
                   G_CALLBACK(+[](GFileMonitor *, GFile *, GFile *,
                                  GFileMonitorEvent event, gpointer data) {
                     if (event == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT ||
                         event == G_FILE_MONITOR_EVENT_CREATED ||
                         event == G_FILE_MONITOR_EVENT_MOVED_IN ||
                         event == G_FILE_MONITOR_EVENT_RENAMED) {
                       static_cast<Config *>(data)->loadConfig();
                     }
                   }),
                   this);
}

Config::Settings Config::getSettings() const {
  Settings settings;
  settings.logLevel = logLevel_ ? logLevel_ : "";
  settings.nbest = nbest_;
  settings.pageSize = pageSize_;
  gchar *flags =
      g_key_file_get_string(keyFile_, "general", "fuzzyflags", nullptr);
  settings.fuzzyFlags = flags ? flags : "";
  g_free(flags);
  settings.englishCandidates = englishCandidates_;
  settings.recordInputHistory = recordInputHistory_;
  settings.learnUserDictionary = learnUserDictionary_;
  return settings;
}

bool Config::saveSettings(const Settings &settings, GError **error) {
  if (settings.nbest <= 0 || settings.pageSize <= 0 || settings.pageSize > 16 ||
      (!settings.logLevel.empty() && settings.logLevel != "DEBUG" &&
       settings.logLevel != "INFO" && settings.logLevel != "WARN" &&
       settings.logLevel != "ERROR")) {
    g_set_error_literal(
        error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE,
        "候选数量必须为正整数，每页数量范围为 1–16，日志级别必须有效。");
    return false;
  }
  gchar **tokens = g_strsplit(settings.fuzzyFlags.c_str(), ",", -1);
  bool validFlags = true;
  for (gchar **token = tokens; *token; ++token) {
    const char *name = g_strstrip(*token);
    if (*name && !fuzzyFlagMap.contains(name)) {
      validFlags = false;
    }
  }
  g_strfreev(tokens);
  if (!validFlags) {
    g_set_error_literal(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_INVALID_VALUE,
                        "模糊音包含未知标志，请使用逗号分隔的有效名称。");
    return false;
  }

  GKeyFile *file = g_key_file_new();
  GError *loadError = nullptr;
  if (!g_key_file_load_from_file(file, configPath_.c_str(), configFileFlags,
                                 &loadError) &&
      !g_error_matches(loadError, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
    g_propagate_error(error, loadError);
    g_key_file_free(file);
    return false;
  }
  g_clear_error(&loadError);
  if (settings.logLevel.empty()) {
    g_key_file_remove_key(file, "general", "loglevel", nullptr);
  } else {
    g_key_file_set_string(file, "general", "loglevel",
                          settings.logLevel.c_str());
  }
  g_key_file_set_integer(file, "general", "nbest", settings.nbest);
  g_key_file_set_integer(file, "general", "pagesize", settings.pageSize);
  g_key_file_set_string(file, "general", "fuzzyflags",
                        settings.fuzzyFlags.c_str());
  g_key_file_set_boolean(file, "general", "englishcandidates",
                         settings.englishCandidates);
  g_key_file_set_boolean(file, "general", "recordinputhistory",
                         settings.recordInputHistory);
  g_key_file_set_boolean(file, "general", "learnuserdictionary",
                         settings.learnUserDictionary);
  gchar *parent = g_path_get_dirname(configPath_.c_str());
  bool saved = false;
  if (g_mkdir_with_parents(parent, 0700) != 0) {
    const int code = errno;
    g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(code),
                "无法创建配置目录：%s", g_strerror(code));
  } else {
    gsize length = 0;
    gchar *contents = g_key_file_to_data(file, &length, error);
    if (contents) {
      saved = g_file_set_contents_full(
          configPath_.c_str(), contents, length,
          static_cast<GFileSetContentsFlags>(G_FILE_SET_CONTENTS_CONSISTENT |
                                             G_FILE_SET_CONTENTS_DURABLE),
          0600, error);
      g_free(contents);
    }
  }
  g_free(parent);
  g_key_file_free(file);
  if (saved) {
    loadConfig();
  }
  return saved;
}

const char *Config::getLogLevel() {
  // Priority: environment variable > config file
  const char *envLogLevel = std::getenv("IBUS_LIBIME_LOG_LEVEL");
  if (envLogLevel) {
    return envLogLevel;
  }

  return logLevel_;
}

int Config::getNBest() const { return nbest_; }

int Config::getPageSize() const { return pageSize_; }

int Config::getFuzzyFlags() const { return fuzzyFlags_; }

int Config::parseFuzzyFlagsString(const char *flagsStr) {
  if (!flagsStr || flagsStr[0] == '\0') {
    return 0;
  }

  int result = 0;
  std::string str(flagsStr);

  // Split by comma and parse each flag
  size_t start = 0;
  size_t end = 0;

  while ((end = str.find(',', start)) != std::string::npos) {
    std::string token = str.substr(start, end - start);
    // Trim whitespace
    size_t first = token.find_first_not_of(" \t");
    size_t last = token.find_last_not_of(" \t");
    if (first != std::string::npos) {
      token = token.substr(first, last - first + 1);
      auto it = fuzzyFlagMap.find(token);
      if (it != fuzzyFlagMap.end()) {
        result |= it->second;
      }
    }
    start = end + 1;
  }

  // Process the last token
  std::string token = str.substr(start);
  size_t first = token.find_first_not_of(" \t");
  size_t last = token.find_last_not_of(" \t");
  if (first != std::string::npos) {
    token = token.substr(first, last - first + 1);
    auto it = fuzzyFlagMap.find(token);
    if (it != fuzzyFlagMap.end()) {
      result |= it->second;
    }
  }

  return result;
}
