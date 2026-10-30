#include "configs.h"

#include <gtk/gtk.h>

#include <climits>
#include <string>

namespace {
class SettingsWindow {
public:
  explicit SettingsWindow(GtkApplication *application) {
    auto &config = Config::getInstance();
    const auto settings = config.getSettings();
    window_ = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window_), "LibIME 拼音设置");
    gtk_window_set_icon_name(GTK_WINDOW(window_), "ibus-pinyin");
    gtk_window_set_default_size(GTK_WINDOW(window_), 520, 460);
    gtk_container_set_border_width(GTK_CONTAINER(window_), 20);

    auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_add(GTK_CONTAINER(window_), box);
    auto *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 18);
    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);

    nbest_ = gtk_spin_button_new_with_range(1, INT_MAX, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(nbest_), settings.nbest);
    addRow(grid, 0, "候选词数量", nbest_);
    pageSize_ = gtk_spin_button_new_with_range(1, 16, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pageSize_), settings.pageSize);
    addRow(grid, 1, "每页候选词数量", pageSize_);
    fuzzyFlags_ = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(fuzzyFlags_), settings.fuzzyFlags.c_str());
    gtk_entry_set_placeholder_text(GTK_ENTRY(fuzzyFlags_), "Inner,CommonTypo");
    gtk_widget_set_tooltip_text(
        fuzzyFlags_,
        "逗号分隔的标志名称；留空使用内置规则。\n"
        "CommonTypo, V_U, AN_ANG, EN_ENG, IAN_IANG, IN_ING,\n"
        "U_OU, UAN_UANG, C_CH, F_H, L_N, S_SH, Z_ZH, VE_UE,\n"
        "Inner, InnerShort, PartialFinal, PartialSp, AdvancedTypo,\n"
        "Correction, L_R");
    addRow(grid, 2, "模糊音标志", fuzzyFlags_);

    history_ = addCheck(box, "记录中文输入历史", settings.recordInputHistory);
    learning_ =
        addCheck(box, "学习用户词库和候选排序", settings.learnUserDictionary);
    english_ = addCheck(box, "中文模式英文补全（重启生效）",
                        settings.englishCandidates);
    auto *privacy =
        gtk_label_new("关闭历史记录不会删除已有记录。\n"
                      "关闭学习后，仍会使用已有用户词库和排序数据。");
    gtk_label_set_xalign(GTK_LABEL(privacy), 0);
    gtk_label_set_line_wrap(GTK_LABEL(privacy), TRUE);
    gtk_box_pack_start(GTK_BOX(box), privacy, FALSE, FALSE, 0);

    auto *logRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
    auto *logLabel = gtk_label_new("日志级别（重启生效）");
    gtk_box_pack_start(GTK_BOX(logRow), logLabel, FALSE, FALSE, 0);
    logLevel_ = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logLevel_), "", "默认");
    for (const char *level : {"DEBUG", "INFO", "WARN", "ERROR"}) {
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logLevel_), level, level);
    }
    // Keep an existing custom value visible instead of silently replacing it.
    if (!settings.logLevel.empty() && settings.logLevel != "DEBUG" &&
        settings.logLevel != "INFO" && settings.logLevel != "WARN" &&
        settings.logLevel != "ERROR") {
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logLevel_),
                                settings.logLevel.c_str(),
                                settings.logLevel.c_str());
    }
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(logLevel_),
                                settings.logLevel.c_str());
    gtk_widget_set_tooltip_text(logLevel_,
                                "IBUS_LIBIME_LOG_LEVEL 环境变量优先于此配置。");
    gtk_box_pack_start(GTK_BOX(logRow), logLevel_, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), logRow, FALSE, FALSE, 0);

    auto *path = gtk_label_new(config.getConfigPath().c_str());
    gtk_label_set_selectable(GTK_LABEL(path), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(path), TRUE);
    gtk_label_set_xalign(GTK_LABEL(path), 0);
    gtk_box_pack_start(GTK_BOX(box), path, FALSE, FALSE, 0);
    status_ =
        gtk_label_new("历史和学习开关保存后直接生效；候选数量、分页和模糊音\n"
                      "从下一次输入开始生效，不打断当前输入。");
    gtk_label_set_xalign(GTK_LABEL(status_), 0);
    gtk_label_set_line_wrap(GTK_LABEL(status_), TRUE);
    gtk_box_pack_start(GTK_BOX(box), status_, FALSE, FALSE, 0);

    auto *buttons = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons), GTK_BUTTONBOX_END);
    gtk_box_set_spacing(GTK_BOX(buttons), 10);
    auto *close = gtk_button_new_with_label("关闭");
    auto *save = gtk_button_new_with_label("保存");
    gtk_style_context_add_class(gtk_widget_get_style_context(save),
                                GTK_STYLE_CLASS_SUGGESTED_ACTION);
    gtk_container_add(GTK_CONTAINER(buttons), close);
    gtk_container_add(GTK_CONTAINER(buttons), save);
    gtk_box_pack_end(GTK_BOX(box), buttons, FALSE, FALSE, 0);
    g_signal_connect_swapped(close, "clicked", G_CALLBACK(gtk_window_close),
                             window_);
    g_signal_connect(save, "clicked",
                     G_CALLBACK(+[](GtkButton *, gpointer data) {
                       static_cast<SettingsWindow *>(data)->save();
                     }),
                     this);
    g_object_set_data_full(
        G_OBJECT(window_), "settings-window", this,
        +[](gpointer data) { delete static_cast<SettingsWindow *>(data); });
    gtk_widget_show_all(window_);
  }

private:
  static void addRow(GtkWidget *grid, int row, const char *text,
                     GtkWidget *widget) {
    auto *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);
    atk_object_set_name(gtk_widget_get_accessible(widget), text);
    gtk_widget_set_hexpand(widget, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
  }

  static GtkWidget *addCheck(GtkWidget *box, const char *label, bool active) {
    auto *check = gtk_check_button_new_with_label(label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), active);
    gtk_box_pack_start(GTK_BOX(box), check, FALSE, FALSE, 0);
    return check;
  }

  void save() {
    Config::Settings settings;
    gtk_spin_button_update(GTK_SPIN_BUTTON(nbest_));
    gtk_spin_button_update(GTK_SPIN_BUTTON(pageSize_));
    settings.nbest = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(nbest_));
    settings.pageSize =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(pageSize_));
    settings.fuzzyFlags = gtk_entry_get_text(GTK_ENTRY(fuzzyFlags_));
    const char *level = gtk_combo_box_get_active_id(GTK_COMBO_BOX(logLevel_));
    settings.logLevel = level ? level : "";
    settings.englishCandidates =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(english_));
    settings.recordInputHistory =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(history_));
    settings.learnUserDictionary =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(learning_));
    GError *error = nullptr;
    if (!Config::getInstance().saveSettings(settings, &error)) {
      auto *dialog = gtk_message_dialog_new(
          GTK_WINDOW(window_),
          static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL |
                                      GTK_DIALOG_DESTROY_WITH_PARENT),
          GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "无法保存设置");
      gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                               error->message);
      g_error_free(error);
      g_signal_connect(dialog, "response",
                       G_CALLBACK(+[](GtkDialog *dialog, gint, gpointer) {
                         gtk_widget_destroy(GTK_WIDGET(dialog));
                       }),
                       nullptr);
      gtk_widget_show(dialog);
      return;
    }
    gtk_label_set_text(GTK_LABEL(status_),
                       "设置已保存。当前输入不受影响。\n"
                       "英文补全和日志级别修改需重启输入法后生效。");
  }

  GtkWidget *window_;
  GtkWidget *nbest_;
  GtkWidget *pageSize_;
  GtkWidget *fuzzyFlags_;
  GtkWidget *history_;
  GtkWidget *learning_;
  GtkWidget *english_;
  GtkWidget *logLevel_;
  GtkWidget *status_;
};
} // namespace

int main(int argc, char **argv) {
  GtkApplication *application = gtk_application_new(
      "org.freedesktop.IBus.LibIME.Setup", static_cast<GApplicationFlags>(0));
  g_signal_connect(application, "activate",
                   G_CALLBACK(+[](GtkApplication *app, gpointer) {
                     GList *windows = gtk_application_get_windows(app);
                     if (windows) {
                       gtk_window_present(GTK_WINDOW(windows->data));
                     } else {
                       new SettingsWindow(app);
                     }
                   }),
                   nullptr);
  const int result = g_application_run(G_APPLICATION(application), argc, argv);
  g_object_unref(application);
  return result;
}
