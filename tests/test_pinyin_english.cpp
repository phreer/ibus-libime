#include "pinyin_engine.h"
#include "ibus_engine.h"
#include "configs.h"

#include <glibmm.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

struct Output {
  std::mutex mutex;
  std::vector<std::string> candidates;
  std::vector<std::string> commits;
  std::string preedit;
  bool lookupVisible = false;
  std::string modeLabel;

  static GDBusMessage *filter(GDBusConnection *, GDBusMessage *message,
                              gboolean incoming, gpointer data) {
    if (incoming || g_dbus_message_get_message_type(message) !=
                        G_DBUS_MESSAGE_TYPE_SIGNAL) {
      return message;
    }
    const char *member = g_dbus_message_get_member(message);
    auto &output = *static_cast<Output *>(data);
    std::lock_guard lock(output.mutex);
    if (g_strcmp0(member, "HideLookupTable") == 0) {
      output.candidates.clear();
      output.lookupVisible = false;
    } else if (g_strcmp0(member, "UpdateProperty") == 0 ||
               g_strcmp0(member, "RegisterProperties") == 0 ||
               g_strcmp0(member, "UpdateLookupTable") == 0 ||
               g_strcmp0(member, "CommitText") == 0 ||
               g_strcmp0(member, "UpdatePreeditText") == 0) {
      GVariant *body = g_dbus_message_get_body(message);
      GVariant *value = g_variant_get_child_value(body, 0);
      auto *object = ibus_serializable_deserialize(value);
      require(object != nullptr, "Cannot deserialize IBus output");
      g_object_ref_sink(object);
      if (IBUS_IS_PROPERTY(object) || IBUS_IS_PROP_LIST(object)) {
        auto *property = IBUS_IS_PROPERTY(object)
                             ? IBUS_PROPERTY(object)
                             : ibus_prop_list_get(IBUS_PROP_LIST(object), 0);
        if (property &&
            g_strcmp0(ibus_property_get_key(property), "InputMode") == 0) {
          output.modeLabel = ibus_property_get_label(property)->text;
        }
      } else if (IBUS_IS_LOOKUP_TABLE(object)) {
        gboolean visible;
        g_variant_get_child(body, 1, "b", &visible);
        output.lookupVisible = visible;
        output.candidates.clear();
        auto *table = IBUS_LOOKUP_TABLE(object);
        for (guint i = 0; i < ibus_lookup_table_get_number_of_candidates(table);
             ++i) {
          output.candidates.emplace_back(
              ibus_lookup_table_get_candidate(table, i)->text);
        }
      } else if (IBUS_IS_TEXT(object)) {
        if (g_strcmp0(member, "UpdatePreeditText") == 0) {
          output.preedit = IBUS_TEXT(object)->text;
        } else {
          output.commits.emplace_back(IBUS_TEXT(object)->text);
        }
      }
      g_object_unref(object);
      g_variant_unref(value);
    }
    return message;
  }
};

class Fixture {
public:
  explicit Fixture(const char *dataPath, const std::string &mode) {
    gchar *path = g_dir_make_tmp("ibus-libime-engine-XXXXXX", nullptr);
    require(path != nullptr, "Cannot create isolated test directory");
    directory = path;
    g_free(path);
    g_setenv("XDG_CONFIG_HOME", directory.c_str(), TRUE);
    g_setenv("XDG_DATA_HOME", directory.c_str(), TRUE);
    g_setenv("XDG_STATE_HOME", directory.c_str(), TRUE);
    g_setenv("IBUS_LIBIME_DATA_DIR",
             mode == "missing" ? directory.c_str() : dataPath, TRUE);
    {
      std::ofstream config(directory / "ibus-libime.ini");
      config << "[general]\npagesize=2\nenglishcandidates="
             << (mode == "disabled" ? "false" : "true") << '\n';
      if (mode == "history-disabled") {
        config << "recordinputhistory=false\n";
      }
      if (mode == "learning-disabled") {
        config << "learnuserdictionary=false\n";
      }
    }
    Glib::init();
    ibus_init();
    bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    g_test_dbus_up(bus);
    GError *error = nullptr;
    connection = g_dbus_connection_new_for_address_sync(
        g_test_dbus_get_bus_address(bus),
        static_cast<GDBusConnectionFlags>(
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
            G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
        nullptr, nullptr, &error);
    require(connection != nullptr, error ? error->message : "No connection");
    filter = g_dbus_connection_add_filter(connection, Output::filter, &output,
                                          nullptr);
    ibusEngine = IBUS_ENGINE(g_object_new(
        IBUS_TYPE_LIBIME_ENGINE, "engine-name", "test", "object-path",
        "/org/freedesktop/IBus/Engine/Test", "connection", connection, nullptr));
    g_object_ref_sink(ibusEngine);
    engine = reinterpret_cast<IBusLibIMEEngine *>(ibusEngine)->pinyin_engine;
  }

  ~Fixture() {
    g_object_unref(ibusEngine);
    PinyinEngine::cleanupSharedIME();
    g_dbus_connection_flush_sync(connection, nullptr, nullptr);
    g_dbus_connection_remove_filter(connection, filter);
    g_dbus_connection_close_sync(connection, nullptr, nullptr);
    g_object_unref(connection);
    g_test_dbus_down(bus);
    g_object_unref(bus);
    std::filesystem::remove_all(directory);
  }

  bool key(guint key, guint modifiers = 0) {
    const bool handled = engine->processKeyEvent(key, 0, modifiers);
    flush();
    return handled;
  }

  void type(const std::string &text) {
    for (unsigned char ch : text) {
      require(key(ch), "Letter must be consumed in Chinese mode");
    }
  }

  void flush() {
    require(g_dbus_connection_flush_sync(connection, nullptr, nullptr),
            "Cannot flush IBus output");
  }

  std::vector<std::string> candidates() {
    flush();
    std::lock_guard lock(output.mutex);
    return output.candidates;
  }

  std::string committed() {
    flush();
    std::lock_guard lock(output.mutex);
    require(!output.commits.empty(), "Expected an IBus commit");
    return output.commits.back();
  }

  void focusIn(const char *id, const char *client = "test") {
    g_signal_emit_by_name(ibusEngine, "focus-in-id", id, client);
  }

  void focusOut(const char *id) {
    g_signal_emit_by_name(ibusEngine, "focus-out-id", id);
  }

  void activateMode() {
    g_signal_emit_by_name(ibusEngine, "property-activate", "InputMode",
                          PROP_STATE_UNCHECKED);
  }

  void requireMode(const char *label) {
    flush();
    std::lock_guard lock(output.mutex);
    require(output.modeLabel == std::string("输入模式：") + label,
            "Tray property must identify and match input mode");
  }

  std::filesystem::path directory;
  GTestDBus *bus = nullptr;
  GDBusConnection *connection = nullptr;
  IBusEngine *ibusEngine = nullptr;
  guint filter = 0;
  Output output;
  PinyinEngine *engine = nullptr;
};

bool hasEnglish(const std::vector<std::string> &words) {
  return std::any_of(words.begin(), words.end(), [](const std::string &word) {
    return !word.empty() && std::all_of(word.begin(), word.end(), [](char ch) {
      return ch >= 'a' && ch <= 'z';
    });
  });
}
} // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    return 1;
  }
  try {
    g_log_set_always_fatal(static_cast<GLogLevelFlags>(
        G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING));
    Fixture fixture(argv[1], argv[2]);
    const std::string mode = argv[2];
    if (mode == "enabled") {
      require(Config::getInstance().getRecordInputHistory(),
              "Input history recording must be enabled by default");
      require(Config::getInstance().getLearnUserDictionary(),
              "User dictionary learning must be enabled by default");
    }
    if (mode == "history-disabled" || mode == "learning-disabled") {
      require(Config::getInstance().getRecordInputHistory() ==
                  (mode != "history-disabled"),
              "Input history setting must match its configured value");
      require(Config::getInstance().getLearnUserDictionary() ==
                  (mode != "learning-disabled"),
              "User dictionary learning setting must match its configured "
              "value");
      fixture.focusIn("/org/freedesktop/IBus/InputContext/Test", "test");
      fixture.type("nihao");
      fixture.key(' ');
      require(fixture.committed() == "你好", "Chinese commit must still work");
      const auto historyPath =
          fixture.directory / "ibus-libime" / "input-history.tsv";
      if (mode == "history-disabled") {
        require(!std::filesystem::exists(historyPath),
                "Disabled input history must not create a history file");
      } else {
        std::ifstream history(historyPath);
        const std::string contents(
            (std::istreambuf_iterator<char>(history)), {});
        require(contents == "nihao\t你好\n",
                "Disabling learning must not disable input history recording");
      }
      std::cout << mode << " setting tests passed\n";
      return 0;
    }
    const char *inputContext = "/org/freedesktop/IBus/InputContext/Test";
    const char *otherContext = "/org/freedesktop/IBus/InputContext/Other";
    const char *fakeContext = "/org/freedesktop/IBus/InputContext/Fake";
    fixture.focusIn(inputContext, "gnome-shell");
    fixture.requireMode("中");
    fixture.focusOut(inputContext);
    fixture.focusIn(fakeContext, "fake");
    fixture.activateMode();
    fixture.requireMode("En");
    fixture.focusOut(fakeContext);
    fixture.focusIn(inputContext, "gnome-shell");
    fixture.requireMode("En");
    require(!fixture.key('a'),
            "Tray switch to English must survive restoring input focus");
    fixture.focusOut(inputContext);
    fixture.focusIn(fakeContext, "fake");
    fixture.requireMode("En");
    fixture.focusOut(fakeContext);
    fixture.focusIn(otherContext);
    fixture.requireMode("中");
    require(fixture.key('a'), "Other input contexts must retain Chinese mode");
    fixture.key(IBUS_KEY_Escape);
    fixture.focusOut(otherContext);
    fixture.focusIn(inputContext, "gnome-shell");
    fixture.requireMode("En");
    require(!fixture.key('a'), "Original input context must retain English mode");
    fixture.focusOut(inputContext);
    fixture.focusIn(fakeContext, "fake");
    fixture.activateMode();
    fixture.requireMode("中");
    fixture.focusOut(fakeContext);
    fixture.focusIn(inputContext, "gnome-shell");
    fixture.requireMode("中");
    require(fixture.key('a'),
            "Tray switch to Chinese must survive restoring input focus");
    fixture.key(IBUS_KEY_Escape);
    fixture.activateMode();
    fixture.focusIn(inputContext, "gnome-shell");
    require(!fixture.key('a'),
            "Focused tray switch must persist without a focus-out callback");
    fixture.key(IBUS_Shift_L);
    fixture.key(IBUS_Shift_L, IBUS_RELEASE_MASK);
    fixture.focusIn(inputContext, "gnome-shell");
    require(fixture.key('a'), "Shift mode changes must persist on focus-in");
    fixture.key(IBUS_KEY_Escape);
    if (std::string(argv[2]) != "enabled") {
      fixture.type("hello");
      require(!hasEnglish(fixture.candidates()),
              "Disabled/missing dictionary must not generate English");
      fixture.key(IBUS_KEY_Return);
      require(fixture.committed() == "hello", "Raw commit must still work");
      fixture.type("nihao");
      require(!fixture.candidates().empty(), "Chinese must remain available");
      fixture.key(' ');
      require(fixture.committed() == "你好", "Chinese commit must still work");
      {
        std::ifstream history(fixture.directory / "ibus-libime" /
                              "input-history.tsv");
        const std::string contents(
            (std::istreambuf_iterator<char>(history)), {});
        require(contents == "nihao\t你好\n",
                "Only committed Chinese input must be recorded as TSV");
      }
      if (std::string(argv[2]) == "missing") {
        std::ifstream log(fixture.directory / "ibus-libime" /
                          "ibus-libime.log");
        const std::string contents((std::istreambuf_iterator<char>(log)), {});
        require(contents.find("English completion unavailable") !=
                    std::string::npos,
                "Missing dictionary must produce an explicit warning");
      }
      std::cout << argv[2] << " fallback tests passed\n";
      return 0;
    }

    for (const auto *input :
         {"zhi", "zhiv", "shi", "she", "chi", "shang", "chang", "zhong"}) {
      fixture.type(input);
      require(!hasEnglish(fixture.candidates()),
              "Short input, rare prefixes and full pinyin syllables must not "
              "insert English candidates");
      fixture.key(IBUS_KEY_Escape);
    }

    for (const auto *input : {"github", "android"}) {
      fixture.type(input);
      bool foundCanonical = false;
      std::vector<std::string> previousPage;
      while (true) {
        const auto page = fixture.candidates();
        if (page == previousPage) {
          break;
        }
        for (const auto &word : page) {
          const bool hasLetters =
              word.find_first_of("abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ") !=
              std::string::npos;
          require(!hasLetters || EnglishDictionary::matchesWord(word, input),
                  "Exact English must hide unconverted pinyin on every page");
          foundCanonical =
              foundCanonical || word == "GitHub" || word == "Android";
        }
        previousPage = page;
        fixture.key(IBUS_KEY_Page_Down);
      }
      require(foundCanonical,
              "Canonical English candidate must remain visible");
      fixture.key(IBUS_KEY_Escape);
    }

    fixture.type("github");
    fixture.key(' ');
    require(fixture.committed() == "GitHub",
            "Space must commit GitHub after filtering");
    fixture.type("github");
    fixture.engine->selectPageCandidate(0);
    require(fixture.committed() == "GitHub",
            "Page selection must commit GitHub after filtering");
    fixture.type("github");
    fixture.key('1');
    require(fixture.committed() == "GitHub",
            "Number selection must commit GitHub after filtering");
    fixture.type("githubx");
    const auto unmatched = fixture.candidates();
    require(std::none_of(unmatched.begin(), unmatched.end(),
                         [](const std::string &word) {
                           return word.find_first_of(
                                      "abcdefghijklmnopqrstuvwxyz"
                                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ") !=
                                  std::string::npos;
                         }),
            "Unmatched input must not append raw letters to pinyin candidates");
    fixture.key(IBUS_KEY_Return);
    require(fixture.committed() == "githubx",
            "Enter must still commit unmatched raw input");

    for (const auto *selection : {"space", "number", "page"}) {
      fixture.type("git");
      const auto prefixes = fixture.candidates();
      require(!prefixes.empty(), "git must offer converted g prefixes");
      for (const auto &word : prefixes) {
        require(word.find_first_of(
                    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") ==
                    std::string::npos,
                "git candidates must not contain the unconverted it suffix");
      }
      size_t commitsBeforeSelection;
      {
        std::lock_guard lock(fixture.output.mutex);
        commitsBeforeSelection = fixture.output.commits.size();
      }
      if (std::string(selection) == "space") {
        fixture.key(' ');
      } else if (std::string(selection) == "number") {
        fixture.key('1');
      } else {
        fixture.engine->selectPageCandidate(0);
      }
      fixture.flush();
      {
        std::lock_guard lock(fixture.output.mutex);
        require(
            fixture.output.preedit.starts_with(prefixes.front()) &&
                fixture.output.preedit.ends_with("it"),
            "Selecting g must preserve it in preedit after selected Chinese");
        require(
            fixture.output.commits.size() == commitsBeforeSelection,
            "Selecting a prefix must not prematurely commit the composition");
      }
      require(fixture.candidates() == std::vector<std::string>{"it"},
              "Unconverted remainder must stay selectable in the lookup table");
      {
        std::lock_guard lock(fixture.output.mutex);
        require(fixture.output.lookupVisible,
                "The lookup table must remain visible for the raw remainder");
      }
      fixture.key(IBUS_KEY_BackSpace);
      fixture.key('t');
      require(fixture.candidates() == std::vector<std::string>{"it"},
              "Editing the remainder must preserve its raw candidate");
      if (std::string(selection) == "space") {
        fixture.key(' ');
      } else if (std::string(selection) == "number") {
        fixture.key('1');
      } else {
        fixture.engine->selectPageCandidate(0);
      }
      require(
          fixture.committed() == prefixes.front() + "it",
          "Selecting the raw remainder must commit selected Chinese and it");
      require(fixture.candidates().empty(),
              "Completing the remainder must clear the lookup table");
    }

    fixture.type("android");
    auto androidCandidates = fixture.candidates();
    require(!androidCandidates.empty() &&
                androidCandidates.front() == "Android",
            "Exact English must precede Chinese with unconverted letters");
    fixture.key(' ');
    require(fixture.committed() == "Android",
            "Space must commit the promoted English candidate");

    fixture.type("andro");
    auto prefixCandidates = fixture.candidates();
    auto android =
        std::find(prefixCandidates.begin(), prefixCandidates.end(), "Android");
    require(android != prefixCandidates.end(),
            "Lowercase prefix must offer canonical Android spelling");
    fixture.engine->selectPageCandidate(
        std::distance(prefixCandidates.begin(), android));
    require(fixture.committed() == "Android",
            "Selecting a completion must preserve canonical casing");

    fixture.type("android");
    auto lowercaseCandidates = fixture.candidates();
    auto lowercase = std::find(lowercaseCandidates.begin(),
                               lowercaseCandidates.end(), "android");
    require(lowercase != lowercaseCandidates.end(),
            "Lowercase android must remain selectable");
    fixture.engine->selectPageCandidate(
        std::distance(lowercaseCandidates.begin(), lowercase));
    require(fixture.committed() == "android",
            "Lowercase variant must commit without case conversion");

    fixture.type("and");
    auto ambiguousCandidates = fixture.candidates();
    require(!ambiguousCandidates.empty() &&
                ambiguousCandidates.front() != "and",
            "Exact English must not displace a fully converted Chinese first "
            "candidate");
    fixture.key(IBUS_KEY_Escape);

    fixture.type("he");
    require(!hasEnglish(fixture.candidates()),
            "Short prefix must not show English");
    fixture.type("llo");
    auto words = fixture.candidates();
    auto hello = std::find(words.begin(), words.end(), "hello");
    require(hello != words.end(), "Exact English word must be on first page");
    fixture.key('1' + std::distance(words.begin(), hello));
    require(fixture.committed() == "hello",
            "Number must commit English without space");
    require(fixture.candidates().empty(), "Commit must clear candidates");

    fixture.type("comput");
    words = fixture.candidates();
    require(
        !words.empty() && words.front() != "computer",
        "Prefix-only completion must not displace the Chinese first candidate");
    require(std::find(words.begin(), words.end(), "computer") != words.end(),
            "An unfinished prefix must offer a complete word");
    fixture.key(IBUS_KEY_Escape);

    fixture.type("comp");
    words = fixture.candidates();
    auto company = std::find(words.begin(), words.end(), "company");
    if (company == words.end()) {
      fixture.key(IBUS_KEY_Page_Down);
      words = fixture.candidates();
      company = std::find(words.begin(), words.end(), "company");
    }
    require(company != words.end(), "Prefix completion must be visible");
    fixture.engine->selectPageCandidate(std::distance(words.begin(), company));
    require(fixture.committed() == "company",
            "Mouse/page selection must commit English");

    fixture.type("nihao");
    words = fixture.candidates();
    require(!words.empty() && words[0] == "你好",
            "Chinese first candidate must be preserved");
    fixture.key(' ');
    require(fixture.committed() == "你好", "Space must still commit Chinese");

    fixture.type("comp");
    fixture.key(IBUS_KEY_Page_Down);
    words = fixture.candidates();
    require(!words.empty(), "Second page must exist");
    auto expected = words.front();
    fixture.engine->selectPageCandidate(0);
    require(fixture.committed() == expected,
            "Page-local selection must use current page");

    fixture.type("compu");
    fixture.key(IBUS_KEY_Page_Down);
    fixture.key(IBUS_KEY_BackSpace);
    require(hasEnglish(fixture.candidates()),
            "Editing must return to first page");
    fixture.key(IBUS_KEY_Left);
    require(!hasEnglish(fixture.candidates()),
            "Middle cursor must suppress completion");
    fixture.key(IBUS_KEY_Right);
    require(hasEnglish(fixture.candidates()),
            "End cursor must restore completion");
    fixture.key('\'');
    require(!hasEnglish(fixture.candidates()),
            "Pinyin separator must suppress completion");
    fixture.key(IBUS_KEY_Escape);
    require(fixture.candidates().empty(), "Escape must clear candidates");

    fixture.type("comp");
    fixture.engine->focusOut();
    require(fixture.candidates().empty(), "Focus out must clear candidates");
    fixture.type("comp");
    fixture.key(IBUS_KEY_Return);
    require(fixture.committed() == "comp", "Enter must commit raw prefix");
    fixture.type("hello");
    fixture.key(IBUS_Shift_L);
    fixture.key(IBUS_Shift_L, IBUS_RELEASE_MASK);
    require(fixture.committed() == "hello", "Shift must commit raw input");
    require(!fixture.key('c') && !fixture.key(' ') && !fixture.key('1'),
            "En mode must pass keys through");
    require(fixture.candidates().empty(), "En mode must not show candidates");
    require(!fixture.key('c', IBUS_CONTROL_MASK),
            "Ctrl shortcuts must pass through");
    std::cout << "Chinese/English candidate integration tests passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
