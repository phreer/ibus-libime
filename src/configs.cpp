#include "configs.h"

#include <cstdlib>
#include <cstring>
#include <format>
#include <map>
#include <string>

Config::Config()
    : keyFile_(nullptr), logLevel_(nullptr), nbest_(3), pageSize_(9),
      fuzzyFlags_(0) {
  configPath_ = getConfigFilePath();
  keyFile_ = g_key_file_new();
  loadConfig();
}

Config::~Config() {
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

  // Try to load the config file
  if (g_key_file_load_from_file(keyFile_, configPath_.c_str(), G_KEY_FILE_NONE,
                                &error)) {
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
    if (!error && pageSize > 0) {
      pageSize_ = pageSize;
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
    }
  } else {
    // Failed to load (file might not exist), ignore the error
    if (error) {
      g_error_free(error);
    }
  }
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

  // Map of flag names to their bit positions
  static const std::map<std::string, int> flagMap = {
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
      auto it = flagMap.find(token);
      if (it != flagMap.end()) {
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
    auto it = flagMap.find(token);
    if (it != flagMap.end()) {
      result |= it->second;
    }
  }

  return result;
}

