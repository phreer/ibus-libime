#ifndef IBUS_LIBIME_CONFIGS_H
#define IBUS_LIBIME_CONFIGS_H

#include <gio/gio.h>

#include <string>

class Config {
public:
  struct Settings {
    std::string logLevel;
    int nbest = 3;
    int pageSize = 9;
    std::string fuzzyFlags;
    bool englishCandidates = true;
    bool recordInputHistory = true;
    bool learnUserDictionary = true;
  };

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
  bool getEnglishCandidates() const { return englishCandidates_; }
  bool getRecordInputHistory() const { return recordInputHistory_; }
  bool getLearnUserDictionary() const { return learnUserDictionary_; }
  Settings getSettings() const;
  bool saveSettings(const Settings &settings, GError **error);
  void watch();

  // Get config file path
  const std::string &getConfigPath() const { return configPath_; }

private:
  Config();
  ~Config();

  void loadConfig();
  std::string getConfigFilePath();
  int parseFuzzyFlagsString(const char *flagsStr);

  std::string configPath_;
  GKeyFile *keyFile_;
  GFileMonitor *monitor_ = nullptr;
  char *logLevel_;
  int nbest_;
  int pageSize_;
  int fuzzyFlags_;
  bool englishCandidates_ = true;
  bool recordInputHistory_ = true;
  bool learnUserDictionary_ = true;

  Config(const Config &) = delete;
  Config &operator=(const Config &) = delete;
};

#endif // IBUS_LIBIME_CONFIGS_H
