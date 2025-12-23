#ifndef IBUS_LIBIME_CONFIGS_H
#define IBUS_LIBIME_CONFIGS_H

#include <glib.h>

#include <string>

class Config {
public:
  static Config &getInstance() {
    static Config instance;
    return instance;
  }

  // Get log level from config file or environment variable
  // Returns: "DEBUG", "INFO", "WARN", "ERROR", or nullptr if not set
  const char *getLogLevel();

  // Get NBest value (number of candidates)
  int getNBest() const;

  // Get page size (candidates per page)
  int getPageSize() const;

  // Get fuzzy flags as integer
  int getFuzzyFlags() const;

  // Get config file path
  const std::string &getConfigPath() const { return configPath_; }

private:
  Config();
  ~Config();

  void loadConfig();
  std::string getConfigFilePath();

  std::string configPath_;
  GKeyFile *keyFile_;
  char *logLevel_;
  int nbest_;
  int pageSize_;
  int fuzzyFlags_;

  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;
};

#endif // IBUS_LIBIME_CONFIGS_H
