#include "configs.h"

#include <cstdlib>
#include <format>

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
    int fuzzyFlags =
        g_key_file_get_integer(keyFile_, "general", "fuzzyflags", &error);
    if (!error) {
      fuzzyFlags_ = fuzzyFlags;
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
