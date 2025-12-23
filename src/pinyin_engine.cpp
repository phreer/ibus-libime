#include "pinyin_engine.h"

#include <glibmm.h>
#include <ibus.h>
#include <libime/core/userlanguagemodel.h>
#include <libime/pinyin/pinyindictionary.h>

#include <cstdlib>
#include <iostream>
#include <map>

#include "config.h"
#include "logger.h"

using namespace libime;

struct PinyinContextStatus {
  bool englishMode;
};

static std::map<std::string, PinyinContextStatus> context_status_map;

// Initialize static member
std::shared_ptr<PinyinIME> PinyinEngine::shared_ime_ = nullptr;

PinyinEngine::PinyinEngine(IBusEngine *engine)
    : engine_(engine), page_size_(9), current_page_(0), english_mode_(false),
      shift_pressed_(false), mode_prop_(nullptr), prop_list_(nullptr) {
  LOG_INFO("PinyinEngine constructor called");
  initializeIME();
  initProperties();
  LOG_INFO("PinyinEngine initialized successfully");
}

PinyinEngine::~PinyinEngine() {
  // Clean up properties
  if (mode_prop_) {
    g_object_unref(mode_prop_);
    mode_prop_ = nullptr;
  }
  if (prop_list_) {
    g_object_unref(prop_list_);
    prop_list_ = nullptr;
  }
}

void PinyinEngine::initializeSharedIME() {
  if (shared_ime_) {
    LOG_INFO("Shared IME already initialized");
    return;
  }

  LOG_INFO("Initializing shared LibIME components");

  // Initialize LibIME components
  shared_ime_ = std::make_shared<PinyinIME>(
      std::make_unique<PinyinDictionary>(),
      std::make_unique<libime::UserLanguageModel>(
          libime::DefaultLanguageModelResolver::instance()
              .languageModelFileForLanguage("zh_CN")));
  LOG_INFO("Shared PinyinIME created");

  // Load system dictionary
  std::string dict_path = getDataPath("sc.dict");
  LOG_INFO("Dictionary path: {}", dict_path);
  shared_ime_->dict()->load(PinyinDictionary::SystemDict, dict_path.c_str(),
                            PinyinDictFormat::Binary);
  LOG_INFO("Dictionary loaded");

  // Configure IME
  shared_ime_->setNBest(3);
  PinyinFuzzyFlags flags{PinyinFuzzyFlag::Inner, PinyinFuzzyFlag::CommonTypo};
  shared_ime_->setFuzzyFlags(flags);
  LOG_INFO("Shared IME configured: NBest=3, FuzzyFlags enabled");
}

void PinyinEngine::cleanupSharedIME() {
  if (shared_ime_) {
    LOG_INFO("Cleaning up shared IME");
    shared_ime_.reset();
  }
}

std::string PinyinEngine::getDataPath(const std::string &filename) {
  const char *data_dir = std::getenv("LIBIME_DATA_DIR");
  if (data_dir) {
    return std::string(data_dir) + "/" + filename;
  }
  // Default fallback path
  return std::string(LIBIME_INSTALL_PKGDATADIR) + "/" + filename;
}

void PinyinEngine::initializeIME() {
  LOG_INFO("Initializing PinyinContext for this engine instance");

  // Ensure shared IME is initialized
  if (!shared_ime_) {
    initializeSharedIME();
  }

  // Create context using shared IME
  context_ = std::make_unique<PinyinContext>(shared_ime_.get());
  LOG_INFO("PinyinContext created for this instance");
}

bool PinyinEngine::processKeyEvent(guint keyval, guint keycode,
                                   guint modifiers) {
  LOG_DEBUG("processKeyEvent: keyval=0x{:x} keycode={} modifiers=0x{:x}",
            keyval, keycode, modifiers);

  // Handle Shift key for mode switching
  bool is_shift = (keyval == IBUS_Shift_L || keyval == IBUS_Shift_R);

  if (is_shift && !(modifiers & IBUS_RELEASE_MASK)) {
    // Shift key pressed
    if (!shift_pressed_) {
      shift_pressed_ = true;
    }
    return FALSE; // Let other modifiers work
  }

  if (is_shift && (modifiers & IBUS_RELEASE_MASK)) {
    // Shift key released
    if (shift_pressed_) {
      if (context_->size() > 0) {
        LOG_INFO("Shift released with pending input, committing raw input: {}",
                 context_->userInput());
        std::string raw_input = context_->userInput();
        reset();
        commitString(raw_input);
      }
      toggleInputMode();
    }
    shift_pressed_ = false;
    return FALSE;
  }

  // If Shift was pressed and another key is pressed, reset the flag
  if (shift_pressed_ && !is_shift) {
    shift_pressed_ = false;
  }

  // Ignore release events for other keys
  if (modifiers & IBUS_RELEASE_MASK) {
    LOG_DEBUG("Ignoring key release event");
    return FALSE;
  }

  // Pass through if has Ctrl, Alt or Super (Win) modifier
  if (modifiers &
      (IBUS_CONTROL_MASK | IBUS_MOD1_MASK | IBUS_MOD4_MASK | IBUS_SUPER_MASK)) {
    LOG_DEBUG("Passing through: has Ctrl/Alt/Super modifier");
    return FALSE;
  }

  // In English mode, pass through all keys
  if (english_mode_) {
    LOG_DEBUG("English mode: passing through");
    return FALSE;
  }

  LOG_DEBUG("Current context size: {}", context_->size());

  // If no input, pass through
  if (context_->size() == 0) {
    // Only handle input keys
    if ((keyval >= 'a' && keyval <= 'z') || keyval == '\'') {
      LOG_DEBUG("Processing input key with empty context");
      return processInputKey(keyval, modifiers);
    }
  }

  // Handle control keys when we have input
  if (processControlKey(keyval, modifiers)) {
    LOG_DEBUG("Control key handled");
    return TRUE;
  }

  // Handle input keys
  bool result = processInputKey(keyval, modifiers);
  LOG_DEBUG("Input key processed: {}", result ? "handled" : "passed through");
  return result;
}

bool PinyinEngine::processInputKey(guint keyval, guint modifiers) {
  // Handle punctuation in Chinese mode (only when no pending input)
  if (!english_mode_ && context_->size() == 0) {
    std::string punct = convertPunctuation(keyval);
    if (!punct.empty()) {
      commitString(punct);
      return TRUE;
    }
  }

  // Handle letter input
  if (keyval >= 'a' && keyval <= 'z') {
    char ch = static_cast<char>(keyval);
    LOG_DEBUG("Typing letter: {}", ch);
    context_->type(std::string(1, ch));
    LOG_DEBUG("Context after typing: size={} input={}", context_->size(),
              context_->userInput());
    updateUI();
    return TRUE;
  }

  // Handle apostrophe for pinyin separation
  if (keyval == '\'' && context_->size() > 0) {
    LOG_DEBUG("Typing apostrophe");
    context_->type("'");
    updateUI();
    return TRUE;
  }

  // Handle number selection (1-9, 0)
  if (keyval >= '0' && keyval <= '9' && context_->size() > 0) {
    size_t index = keyval - '1' + current_page_ * page_size_;
    if (keyval == '0') {
      index = 9 + current_page_ * page_size_;
    }
    LOG_DEBUG("Number key pressed: {} -> candidate index {}", keyval - '0',
              index);
    selectCandidate(index);
    return TRUE;
  }

  // Handle space - select first candidate
  if (keyval == ' ' && context_->size() > 0) {
    LOG_DEBUG("Space pressed -> selecting first candidate");
    selectCandidate(0);
    return TRUE;
  }

  return FALSE;
}

bool PinyinEngine::processControlKey(guint keyval, guint modifiers) {
  switch (keyval) {
  case IBUS_KEY_BackSpace:
    if (context_->size() > 0) {
      context_->backspace();
      if (context_->size() == 0) {
        reset();
      } else {
        updateUI();
      }
      return TRUE;
    }
    return FALSE;

  case IBUS_KEY_Delete:
    if (context_->size() > 0) {
      context_->del();
      if (context_->size() == 0) {
        reset();
      } else {
        updateUI();
      }
      return TRUE;
    }
    return FALSE;

  case IBUS_KEY_Escape:
    reset();
    return TRUE;

  case IBUS_KEY_Left:
    if (context_->cursor() > 0) {
      context_->setCursor(context_->cursor() - 1);
      updateUI();
      return TRUE;
    }
    return FALSE;

  case IBUS_KEY_Right:
    if (context_->cursor() < context_->size()) {
      context_->setCursor(context_->cursor() + 1);
      updateUI();
      return TRUE;
    }
    return FALSE;

  case IBUS_KEY_Return:
  case IBUS_KEY_KP_Enter:
    if (context_->size() > 0) {
      // Commit the raw user input (pinyin) instead of the converted sentence
      std::string raw_input = context_->userInput();
      reset();
      commitString(raw_input);
      return TRUE;
    }
    return FALSE;

  case IBUS_KEY_Page_Up:
    pageUp();
    return TRUE;

  case IBUS_KEY_minus:
    if (context_->size() > 0) {
      pageUp();
      return TRUE;
    }
    return FALSE;

  case IBUS_KEY_Page_Down:
    pageDown();
    return TRUE;

  case IBUS_KEY_equal:
    if (context_->size() > 0) {
      pageDown();
      return TRUE;
    }
    return FALSE;

  default:
    return FALSE;
  }
}

void PinyinEngine::selectCandidate(size_t index) {
  const auto &candidates = context_->candidates();
  LOG_DEBUG("selectCandidate: index={} total_candidates={}", index,
            candidates.size());

  if (index < candidates.size()) {
    std::string candidate_text = candidates[index].toString();
    LOG_INFO("Selecting candidate {}: {}", index, candidate_text);

    context_->select(index);

    if (context_->selected()) {
      std::string sentence = context_->sentence();
      LOG_INFO("Committing sentence: {}", sentence);
      commitString(sentence);
      context_->learn();
      LOG_DEBUG("Learning completed");
      reset();
    } else {
      LOG_DEBUG("Partial selection, updating UI");
      updateUI();
    }
  } else {
    LOG_WARN("Invalid candidate index: {} (total: {})", index,
             candidates.size());
  }
}

void PinyinEngine::updateUI() {
  updatePreedit();
  updateLookupTable();
}

void PinyinEngine::updatePreedit() {
  if (context_->size() == 0) {
    ibus_engine_hide_preedit_text(engine_);
    return;
  }

  ibus_engine_hide_auxiliary_text(engine_);

  auto [preedit, cursor_pos] = context_->preeditWithCursor();

  IBusText *text = ibus_text_new_from_string(preedit.c_str());
  ibus_text_append_attribute(text, IBUS_ATTR_TYPE_UNDERLINE,
                             IBUS_ATTR_UNDERLINE_SINGLE, 0, preedit.length());

  ibus_engine_update_preedit_text(engine_, text, cursor_pos, TRUE);
}

void PinyinEngine::updateLookupTable() {
  const auto &candidates = context_->candidates();

  LOG_DEBUG("updateLookupTable: {} candidates", candidates.size());

  if (candidates.empty()) {
    LOG_DEBUG("Hiding lookup table (no candidates)");
    ibus_engine_hide_lookup_table(engine_);
    return;
  }

  IBusLookupTable *table = ibus_lookup_table_new(page_size_, 0, TRUE, TRUE);

  size_t start = current_page_ * page_size_;
  size_t end = std::min(start + page_size_, candidates.size());

  LOG_DEBUG("Showing candidates {} to {} (page {})", start, end - 1,
            current_page_);

  for (size_t i = start; i < end; ++i) {
    const auto &candidate = candidates[i];
    std::string text = candidate.toString();
    LOG_DEBUG("  Candidate {}: {} (score: {})", i + 1, text, candidate.score());

    IBusText *ibus_text = ibus_text_new_from_string(text.c_str());
    ibus_lookup_table_append_candidate(table, ibus_text);
  }

  ibus_engine_update_lookup_table(engine_, table, TRUE);
  g_object_unref(table);
}

void PinyinEngine::commitString(const std::string &text) {
  LOG_INFO("Committing text: {}", text);
  IBusText *ibus_text = ibus_text_new_from_string(text.c_str());
  ibus_engine_commit_text(engine_, ibus_text);
}

std::string PinyinEngine::convertPunctuation(guint keyval) {
  // Map of English punctuation to Chinese punctuation
  static const std::map<guint, std::string> punct_map = {
      {',', "，"},  {'.', "。"}, {'?', "？"}, {'!', "！"}, {';', "；"},
      {':', "："},  {'(', "（"}, {')', "）"}, {'[', "【"}, {']', "】"},
      {'<', "《"},  {'>', "》"}, {'"', "\""}, {'\'', "'"}, {'~', "～"},
      {'\\', "、"}, {'$', "￥"}, {'^', "……"}, {'_', "——"},
  };

  auto it = punct_map.find(keyval);
  if (it != punct_map.end()) {
    LOG_DEBUG("Converting punctuation: {} -> {}", (char)keyval, it->second);
    return it->second;
  }

  return "";
}

void PinyinEngine::focusInId(const gchar *object_path, const gchar *client) {
  LOG_INFO("Focus in with ID: {} client: {}", object_path, client);
  english_mode_ = context_status_map[std::string(object_path)].englishMode;
  focusIn();
}

void PinyinEngine::focusIn() {
  LOG_INFO("Focus in");
  // Restore the mode property display
  ibus_engine_register_properties(engine_, prop_list_);

  updateInputMode();
  // Optionally load user data
}

void PinyinEngine::focusOutId(const gchar *object_path) {
  LOG_INFO("Focus out with ID: {}", object_path);
  context_status_map[std::string(object_path)] = {english_mode_};
  focusOut();
}

void PinyinEngine::focusOut() {
  LOG_INFO("Focus out");
  // Clear any pending input but keep the mode state
  if (context_->size() > 0) {
    context_->clear();
    current_page_ = 0;
    ibus_engine_hide_preedit_text(engine_);
    ibus_engine_hide_lookup_table(engine_);
  }
  // Don't call reset() which would clear everything
  // Optionally save user data
}

void PinyinEngine::reset() {
  LOG_DEBUG("Reset called");
  context_->clear();
  current_page_ = 0;
  ibus_engine_hide_preedit_text(engine_);
  ibus_engine_hide_lookup_table(engine_);
  LOG_DEBUG("Reset complete");
}

void PinyinEngine::enable() {
  LOG_INFO("Engine enabled");
  reset();
}

void PinyinEngine::disable() {
  LOG_INFO("Engine disabled");
  reset();
}

void PinyinEngine::pageUp() {
  if (current_page_ > 0) {
    current_page_--;
    updateLookupTable();
  }
}

void PinyinEngine::pageDown() {
  const auto &candidates = context_->candidates();
  size_t max_page = (candidates.size() + page_size_ - 1) / page_size_;

  if (current_page_ + 1 < max_page) {
    current_page_++;
    updateLookupTable();
  }
}

void PinyinEngine::cursorUp() {
  // Cursor up in lookup table is handled by pageUp
  pageUp();
}

void PinyinEngine::cursorDown() {
  // Cursor down in lookup table is handled by pageDown
  pageDown();
}

void PinyinEngine::toggleInputMode() {
  english_mode_ = !english_mode_;
  LOG_INFO("Input mode toggled: {}", english_mode_ ? "English" : "Chinese");
  updateInputMode();
}

void PinyinEngine::initProperties() {
  // Create property list
  prop_list_ = ibus_prop_list_new();
  g_object_ref_sink(prop_list_);

  // Create mode property - use PROP_TYPE_NORMAL for status bar display
  IBusText *label = ibus_text_new_from_string("中");
  IBusText *tooltip = ibus_text_new_from_string("切换中英文 (Shift)");

  mode_prop_ = ibus_property_new("InputMode", PROP_TYPE_NORMAL, label,
                                 nullptr, // icon - can be nullptr
                                 tooltip,
                                 TRUE, // sensitive
                                 TRUE, // visible
                                 PROP_STATE_UNCHECKED, nullptr);

  ibus_prop_list_append(prop_list_, mode_prop_);

  // Register properties with engine
  ibus_engine_register_properties(engine_, prop_list_);

  LOG_INFO("Properties initialized");
}

void PinyinEngine::updateModeProperty() {
  if (!mode_prop_)
    return;

  // Update label based on mode
  const char *label = english_mode_ ? "En" : "中";

  IBusText *label_text = ibus_text_new_from_string(label);
  ibus_property_set_label(mode_prop_, label_text);

  // Update state
  ibus_property_set_state(mode_prop_, english_mode_ ? PROP_STATE_CHECKED
                                                    : PROP_STATE_UNCHECKED);

  // Update the property in IBus
  ibus_engine_update_property(engine_, mode_prop_);

  LOG_DEBUG("Mode property updated: {}", label);
}

void PinyinEngine::updateInputMode() {
  // Update auxiliary text to show current mode
  IBusText *text =
      ibus_text_new_from_string(english_mode_ ? "English" : "中文");
  ibus_engine_update_auxiliary_text(engine_, text, TRUE);
  ibus_engine_show_auxiliary_text(engine_);

  Glib::signal_timeout().connect_seconds_once(
      [this]() { ibus_engine_hide_auxiliary_text(engine_); }, 1);

  // Update icon
  updateModeProperty();

  // Clear any pending input when switching modes
  if (context_->size() > 0) {
    reset();
  }
}
