#ifndef IBUS_ENGINE_H
#define IBUS_ENGINE_H

#include <ibus.h>

#include <memory>

class PinyinEngine;

class IBusEngineWrapper {
public:
  static void init();
  static void run();

private:
  static IBusBus *bus_;
  static IBusFactory *factory_;

  // IBus callbacks
  static GObject *create_engine(IBusFactory *factory, const gchar *engine_name,
                                gpointer user_data);

  static void bus_disconnected_cb(IBusBus *bus, gpointer user_data);
};

// IBusEngine GObject structure
#define IBUS_TYPE_LIBIME_ENGINE (ibus_libime_engine_get_type())

GType ibus_libime_engine_get_type(void);

typedef struct _IBusLibIMEEngine IBusLibIMEEngine;
typedef struct _IBusLibIMEEngineClass IBusLibIMEEngineClass;

struct _IBusLibIMEEngine {
  IBusEngine parent;

  // Private data
  PinyinEngine *pinyin_engine;
};

struct _IBusLibIMEEngineClass {
  IBusEngineClass parent;
};

// GObject methods
void ibus_libime_engine_register_type(IBusFactory *factory);

#endif // IBUS_ENGINE_H
