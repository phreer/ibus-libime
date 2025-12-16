#ifndef PINYIN_ENGINE_H
#define PINYIN_ENGINE_H

#include <memory>
#include <string>
#include <vector>
#include <ibus.h>
#include <libime/pinyin/pinyincontext.h>
#include <libime/pinyin/pinyinime.h>

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
    void toggleInputMode();

private:
    IBusEngine *engine_;
    static std::shared_ptr<libime::PinyinIME>
        shared_ime_; // Shared across all instances
    std::unique_ptr<libime::PinyinContext> context_;

    // UI state
    size_t page_size_;
    size_t current_page_;

    // Input mode state
    bool english_mode_; // true = English mode, false = Chinese mode
    bool shift_pressed_;

    // Properties
    IBusProperty *mode_prop_;
    IBusPropList *prop_list_;

    // Helper methods
    void updateInputMode();
    void initProperties();
    void updateModeProperty();
    void initializeIME();
    void updateUI();
    void updatePreedit();
    void updateLookupTable();
    void commitString(const std::string &text);
    bool processInputKey(guint keyval, guint modifiers);
    bool processControlKey(guint keyval, guint modifiers);
    std::string convertPunctuation(guint keyval);
    static std::string getDataPath(const std::string &filename);
};

#endif // PINYIN_ENGINE_H
