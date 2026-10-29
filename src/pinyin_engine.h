#ifndef PINYIN_ENGINE_H
#define PINYIN_ENGINE_H

#include <ibus.h>
#include <libime/pinyin/pinyincontext.h>
#include <libime/pinyin/pinyinime.h>
#include <sigc++/connection.h>

#include <memory>
#include <string>
#include <vector>

#include "english_dictionary.h"

class PinyinEngine {
public:
  explicit PinyinEngine(IBusEngine *engine);
  ~PinyinEngine();

  // Static initialization for shared IME
  static void initializeSharedIME();
  static void cleanupSharedIME();

  // Key event handling
  bool processKeyEvent(guint keyval, guint keycode, guint modifiers);

  // Engine state management
  void focusIn();
  void focusInId(const gchar *object_path, const gchar *client);
  void focusOut();
  void focusOutId(const gchar *object_path);
  void reset();
  void enable();
  void disable();

  // Candidate navigation
  void pageUp();
  void pageDown();
  void cursorUp();
  void cursorDown();
  void selectCandidate(size_t index);
  void selectPageCandidate(size_t index);
  void toggleInputMode();

private:
  IBusEngine *engine_;
  static std::shared_ptr<libime::PinyinIME>
      shared_ime_; // Shared across all instances
  std::unique_ptr<libime::PinyinContext> context_;
  static std::unique_ptr<EnglishDictionary> english_dictionary_;

  struct Candidate {
    enum class Type { Pinyin, English, Raw };
    Type type;
    std::string text;
    size_t pinyin_index;
    size_t prefix_end = 0;
  };
  std::vector<Candidate> candidates_;

  // UI state
  size_t page_size_;
  size_t current_page_;

  // Input mode state
  bool english_mode_; // true = English mode, false = Chinese mode
  bool shift_pressed_;
  std::string input_context_id_;

  // Punctuation state
  bool double_quote_left_; // true = next is left quote, false = right quote
  bool single_quote_left_; // true = next is left quote, false = right quote

  // Properties
  IBusProperty *mode_prop_;
  IBusPropList *prop_list_;
  sigc::connection mode_timeout_;

  // Helper methods
  void updateInputMode();
  void initProperties();
  void updateModeProperty();
  void initializeIME();
  void updateUI();
  void rebuildCandidates();
  void updatePreedit();
  void updateLookupTable();
  void commitString(const std::string &text);
  bool processInputKey(guint keyval, guint modifiers);
  bool processControlKey(guint keyval, guint modifiers);
  std::string convertPunctuation(guint keyval);
  static std::string getDataPath(const std::string &filename);
  static std::string getUserDataDir();
  static void saveUserData();
};

#endif // PINYIN_ENGINE_H
