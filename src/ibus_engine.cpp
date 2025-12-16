#include "ibus_engine.h"
#include "logger.h"

#include <iostream>
#include <glib.h>
#include "pinyin_engine.h"

// Static members
IBusBus *IBusEngineWrapper::bus_ = nullptr;
IBusFactory *IBusEngineWrapper::factory_ = nullptr;

// Forward declarations
static void ibus_libime_engine_class_init(IBusLibIMEEngineClass *klass);
static void ibus_libime_engine_init(IBusLibIMEEngine *engine);
static void ibus_libime_engine_destroy(IBusLibIMEEngine *engine);
static GObject *
ibus_libime_engine_constructor(GType type, guint n_construct_properties,
                               GObjectConstructParam *construct_properties);

// Engine callbacks
static gboolean ibus_libime_engine_process_key_event(IBusEngine *engine,
                                                     guint keyval,
                                                     guint keycode,
                                                     guint modifiers);
static void ibus_libime_engine_focus_in(IBusEngine *engine);
static void ibus_libime_engine_focus_in_id(IBusEngine *engine, const gchar *,
                                           const gchar *);
static void ibus_libime_engine_focus_out(IBusEngine *engine);
static void ibus_libime_engine_focus_out_id(IBusEngine *engine, const gchar *);
static void ibus_libime_engine_reset(IBusEngine *engine);
static void ibus_libime_engine_enable(IBusEngine *engine);
static void ibus_libime_engine_disable(IBusEngine *engine);
static void ibus_libime_engine_page_up(IBusEngine *engine);
static void ibus_libime_engine_page_down(IBusEngine *engine);
static void ibus_libime_engine_cursor_up(IBusEngine *engine);
static void ibus_libime_engine_cursor_down(IBusEngine *engine);
static void ibus_libime_engine_property_activate(IBusEngine *engine,
                                                 const gchar *prop_name,
                                                 guint prop_state);
static void ibus_libime_engine_candidate_clicked(IBusEngine *engine,
                                                 guint index, guint button,
                                                 guint state);

G_DEFINE_TYPE(IBusLibIMEEngine, ibus_libime_engine, IBUS_TYPE_ENGINE)

static void ibus_libime_engine_class_init(IBusLibIMEEngineClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS(klass);

    object_class->constructor = ibus_libime_engine_constructor;

    engine_class->process_key_event = ibus_libime_engine_process_key_event;
    engine_class->reset = ibus_libime_engine_reset;
    engine_class->enable = ibus_libime_engine_enable;
    engine_class->disable = ibus_libime_engine_disable;
    engine_class->focus_in = ibus_libime_engine_focus_in;
    engine_class->focus_in_id = ibus_libime_engine_focus_in_id;
    engine_class->focus_out = ibus_libime_engine_focus_out;
    engine_class->focus_out_id = ibus_libime_engine_focus_out_id;
    engine_class->page_up = ibus_libime_engine_page_up;
    engine_class->page_down = ibus_libime_engine_page_down;
    engine_class->cursor_up = ibus_libime_engine_cursor_up;
    engine_class->cursor_down = ibus_libime_engine_cursor_down;
    engine_class->property_activate = ibus_libime_engine_property_activate;
    engine_class->candidate_clicked = ibus_libime_engine_candidate_clicked;
}

static GObject *
ibus_libime_engine_constructor(GType type, guint n_construct_properties,
                               GObjectConstructParam *construct_properties) {
    // Call parent constructor
    GObject *obj =
        G_OBJECT_CLASS(ibus_libime_engine_parent_class)
            ->constructor(type, n_construct_properties, construct_properties);

    // Set has-focus-id to TRUE if not set during construction
    g_object_set(obj, "has-focus-id", TRUE, NULL);
    return obj;
}

static void ibus_libime_engine_init(IBusLibIMEEngine *engine) {
    engine->pinyin_engine = new PinyinEngine(IBUS_ENGINE(engine));
}

static void ibus_libime_engine_destroy(IBusLibIMEEngine *engine) {
    delete engine->pinyin_engine;
    engine->pinyin_engine = nullptr;

    IBUS_OBJECT_CLASS(ibus_libime_engine_parent_class)
        ->destroy(IBUS_OBJECT(engine));
}

static gboolean ibus_libime_engine_process_key_event(IBusEngine *engine,
                                                     guint keyval,
                                                     guint keycode,
                                                     guint modifiers) {

    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    return libime_engine->pinyin_engine->processKeyEvent(keyval, keycode,
                                                         modifiers);
}

static void ibus_libime_engine_focus_in(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->focusIn();
}

static void ibus_libime_engine_focus_in_id(IBusEngine *engine,
                                           const gchar *object_path,
                                           const gchar *client) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->focusInId(object_path, client);
}

static void ibus_libime_engine_focus_out(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->focusOut();
}

static void ibus_libime_engine_focus_out_id(IBusEngine *engine,
                                            const gchar *object_path) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->focusOutId(object_path);
}

static void ibus_libime_engine_reset(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->reset();
}

static void ibus_libime_engine_enable(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->enable();
}

static void ibus_libime_engine_disable(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->disable();
}

static void ibus_libime_engine_page_up(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->pageUp();
}

static void ibus_libime_engine_page_down(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->pageDown();
}

static void ibus_libime_engine_cursor_up(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->cursorUp();
}

static void ibus_libime_engine_cursor_down(IBusEngine *engine) {
    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    libime_engine->pinyin_engine->cursorDown();
}

static void ibus_libime_engine_property_activate(IBusEngine *engine,
                                                 const gchar *prop_name,
                                                 guint prop_state) {
    // Handle property changes (e.g., switch to/from English mode)
}

static void ibus_libime_engine_candidate_clicked(IBusEngine *engine,
                                                 guint index, guint button,
                                                 guint state) {

    IBusLibIMEEngine *libime_engine = (IBusLibIMEEngine *)engine;
    // Button 1 is left click (value is 1)
    if (button == 1) {
        libime_engine->pinyin_engine->selectCandidate(index);
    }
}

void ibus_libime_engine_register_type(IBusFactory *factory) {
    ibus_factory_add_engine(factory, "libime-pinyin", IBUS_TYPE_LIBIME_ENGINE);
}

// IBusEngineWrapper implementation
void IBusEngineWrapper::init() {
    ibus_init();

    // Initialize shared IME before creating any engine instances
    PinyinEngine::initializeSharedIME();

    bus_ = ibus_bus_new();
    g_signal_connect(bus_, "disconnected", G_CALLBACK(bus_disconnected_cb),
                     nullptr);

    factory_ = ibus_factory_new(ibus_bus_get_connection(bus_));
    // g_signal_connect(factory_, "create-engine", G_CALLBACK(create_engine_cb),
    // NULL);
    ibus_libime_engine_register_type(factory_);

    if (!ibus_bus_request_name(bus_, "org.freedesktop.IBus.LibIME", 0)) {
        g_error("Failed to request bus name");
    }
}

void IBusEngineWrapper::run() { ibus_main(); }

void IBusEngineWrapper::bus_disconnected_cb(IBusBus *bus, gpointer user_data) {
    ibus_quit();
}
