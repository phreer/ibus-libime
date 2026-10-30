#include "configs.h"

#include <gtk/gtk.h>

#include <array>
#include <climits>
#include <string>
#include <utility>

namespace {
struct FuzzyOption {
  const char *key;
  const char *label;
  const char *hint;
};

constexpr std::array<FuzzyOption, 21> fuzzyOptions = {{
    {"CommonTypo", "纠正常见拼写错误", "例如将误写的 gn 识别为 ng。"},
    {"V_U", "ü 与 u 不分", "允许用 u 代替通常以 v 输入的 ü。"},
    {"AN_ANG", "an / ang 不分", "兼容前后鼻音：an 和 ang。"},
    {"EN_ENG", "en / eng 不分", "兼容前后鼻音：en 和 eng。"},
    {"IAN_IANG", "ian / iang 不分", "兼容前后鼻音：ian 和 iang。"},
    {"IN_ING", "in / ing 不分", "兼容前后鼻音：in 和 ing。"},
    {"U_OU", "u / ou 不分", "兼容韵母 u 和 ou。"},
    {"UAN_UANG", "uan / uang 不分", "兼容前后鼻音：uan 和 uang。"},
    {"C_CH", "c / ch 不分", "兼容平翘舌声母 c 和 ch。"},
    {"F_H", "f / h 不分", "兼容声母 f 和 h。"},
    {"L_N", "l / n 不分", "兼容声母 l 和 n。"},
    {"S_SH", "s / sh 不分", "兼容平翘舌声母 s 和 sh。"},
    {"Z_ZH", "z / zh 不分", "兼容平翘舌声母 z 和 zh。"},
    {"VE_UE", "üe 的两种拼写兼容", "兼容以 ve 或 ue 输入 üe。"},
    {"Inner", "自动识别音节内部的切分", "启用引擎内置的音节内部模糊切分规则。"},
    {"InnerShort", "允许更短的音节内部切分", "放宽音节内部的简短切分匹配。"},
    {"PartialFinal", "允许不完整的韵母", "韵母尚未输入完整时也尝试匹配。"},
    {"PartialSp", "允许不完整的双拼",
     "仅在双拼解析时使用；当前全拼输入通常无需启用。"},
    {"AdvancedTypo", "纠正可能有歧义的拼写错误",
     "扩大拼写纠错范围，可能增加不相关候选。"},
    {"Correction", "按键盘布局纠错",
     "依赖引擎的键盘布局纠错配置，当前界面不提供布局配置。"},
    {"L_R", "l / r 不分", "兼容声母 l 和 r。"},
}};

class SettingsWindow {
public:
  explicit SettingsWindow(GtkApplication *application) {
    auto &config = Config::getInstance();
    const auto settings = config.getSettings();
    window_ = gtk_application_window_new(application);
    gtk_window_set_title(GTK_WINDOW(window_), "LibIME 拼音设置");
    gtk_window_set_icon_name(GTK_WINDOW(window_), "ibus-pinyin");
    gtk_window_set_default_size(GTK_WINDOW(window_), 620, 560);
    gtk_container_set_border_width(GTK_CONTAINER(window_), 20);

    auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_add(GTK_CONTAINER(window_), box);
    auto *notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);
    auto *interface = addPage(notebook, "界面");
    auto *engine = addPage(notebook, "引擎");
    auto *fuzzy = addPage(notebook, "模糊音");
    auto *privacy = addPage(notebook, "隐私与学习");
    auto *debug = addPage(notebook, "调试");
    auto *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 18);
    gtk_box_pack_start(GTK_BOX(engine), grid, FALSE, FALSE, 0);
    auto *interfaceGrid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(interfaceGrid), 18);
    gtk_box_pack_start(GTK_BOX(interface), interfaceGrid, FALSE, FALSE, 0);

    nbest_ = gtk_spin_button_new_with_range(1, INT_MAX, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(nbest_), settings.nbest);
    addRow(grid, 0, "候选词数量", nbest_,
           "生成的候选词数量，影响选词准确度。默认：3。\n"
           "保存后从下一次输入开始生效，不打断当前输入。");
    pageSize_ = gtk_spin_button_new_with_range(1, 16, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(pageSize_), settings.pageSize);
    addRow(interfaceGrid, 0, "每页候选词数量", pageSize_,
           "候选窗口每页显示的词条数量，范围：1–16。默认：9。\n"
           "保存后从下一次输入开始生效，不打断当前输入。");
    fuzzyDefault_ = addCheck(
        fuzzy, "使用推荐规则",
        config.getFuzzyFlags() == 0 && config.getEffectiveFuzzyFlags() != 0,
        "使用内置的常见拼写纠错和音节内部切分规则。\n"
        "取消勾选可自定义；自定义时全部不选表示关闭所有模糊音规则。\n"
        "保存后从下一次输入开始生效。");
    fuzzyChoices_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(fuzzy), fuzzyChoices_, FALSE, FALSE, 0);
    auto *initials = addGroup(fuzzyChoices_, "声母（包括平翘舌）");
    auto *finals = addGroup(fuzzyChoices_, "韵母（包括前后鼻音）");
    auto *corrections = addGroup(fuzzyChoices_, "拼写纠错与音节匹配");
    const int flags = config.getEffectiveFuzzyFlags();
    for (size_t i = 0; i < fuzzyOptions.size(); ++i) {
      const auto &option = fuzzyOptions[i];
      GtkWidget *group = (i >= 8 && i <= 12) || i == 20  ? initials
                         : (i >= 1 && i <= 7) || i == 13 ? finals
                                                         : corrections;
      const std::string hint =
          std::string(option.hint) + "\n保存后从下一次输入开始生效。";
      fuzzyChecks_[i] =
          addCheck(group, option.label, (flags & (1 << i)) != 0, hint.c_str());
    }
    gtk_widget_set_sensitive(
        fuzzyChoices_,
        !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fuzzyDefault_)));
    g_signal_connect(fuzzyDefault_, "toggled",
                     G_CALLBACK(+[](GtkToggleButton *button, gpointer choices) {
                       gtk_widget_set_sensitive(
                           GTK_WIDGET(choices),
                           !gtk_toggle_button_get_active(button));
                     }),
                     fuzzyChoices_);

    history_ =
        addCheck(privacy, "记录中文输入历史", settings.recordInputHistory,
                 "将确认提交的中文拼音和文本追加到本机 input-history.tsv。\n"
                 "保存后直接生效；关闭后停止追加，不删除已有记录。");
    learning_ = addCheck(
        privacy, "学习用户词库和候选排序", settings.learnUserDictionary,
        "通过已选择的中文候选学习用户词库并调整候选排序。\n"
        "保存后直接生效；关闭后停止新增学习，仍使用已有用户数据。");
    english_ = addCheck(engine, "中文模式英文补全（重启生效）",
                        settings.englishCandidates,
                        "在中文模式下提供英文单词前缀补全候选。\n"
                        "修改后需重启输入法才能生效。");

    auto *logRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
    auto *logLabel = gtk_label_new("日志级别（重启生效）");
    gtk_box_pack_start(GTK_BOX(logRow), logLabel, FALSE, FALSE, 0);
    logLevel_ = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logLevel_), "", "默认");
    for (const auto &[level, label] :
         std::array<std::pair<const char *, const char *>, 4>{
             {{"DEBUG", "调试（详细）"},
              {"INFO", "常规信息"},
              {"WARN", "仅警告和错误"},
              {"ERROR", "仅错误"}}}) {
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logLevel_), level, label);
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
    const char *logHint = "控制日志详细程度：DEBUG、INFO、WARN、ERROR。\n"
                          "修改后需重启输入法才能生效。\n"
                          "IBUS_LIBIME_LOG_LEVEL 环境变量优先于此配置。";
    gtk_widget_set_tooltip_text(logLevel_, logHint);
    gtk_widget_set_tooltip_text(logLabel, logHint);
    gtk_box_pack_start(GTK_BOX(logRow), logLevel_, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(debug), logRow, FALSE, FALSE, 0);

    auto *path = gtk_label_new(config.getConfigPath().c_str());
    gtk_label_set_selectable(GTK_LABEL(path), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(path), TRUE);
    gtk_label_set_xalign(GTK_LABEL(path), 0);
    gtk_box_pack_start(GTK_BOX(debug), path, FALSE, FALSE, 0);
    status_ =
        gtk_label_new("将鼠标停留在选项上可查看说明；点击“保存”应用修改。");
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
  static GtkWidget *addPage(GtkWidget *notebook, const char *title) {
    auto *scroll = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_set_border_width(GTK_CONTAINER(box), 16);
    gtk_container_add(GTK_CONTAINER(scroll), box);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scroll,
                             gtk_label_new(title));
    return box;
  }

  static GtkWidget *addGroup(GtkWidget *parent, const char *title) {
    auto *frame = gtk_frame_new(title);
    auto *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 10);
    gtk_container_add(GTK_CONTAINER(frame), box);
    gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0);
    return box;
  }

  static void addRow(GtkWidget *grid, int row, const char *text,
                     GtkWidget *widget, const char *hint) {
    auto *label = gtk_label_new(text);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), widget);
    atk_object_set_name(gtk_widget_get_accessible(widget), text);
    gtk_widget_set_tooltip_text(label, hint);
    gtk_widget_set_tooltip_text(widget, hint);
    gtk_widget_set_hexpand(widget, TRUE);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
  }

  static GtkWidget *addCheck(GtkWidget *box, const char *label, bool active,
                             const char *hint) {
    auto *check = gtk_check_button_new_with_label(label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), active);
    gtk_widget_set_tooltip_text(check, hint);
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
    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fuzzyDefault_))) {
      for (size_t i = 0; i < fuzzyOptions.size(); ++i) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fuzzyChecks_[i]))) {
          if (!settings.fuzzyFlags.empty()) {
            settings.fuzzyFlags += ',';
          }
          settings.fuzzyFlags += fuzzyOptions[i].key;
        }
      }
      if (settings.fuzzyFlags.empty()) {
        settings.fuzzyFlags = "None";
      }
    }
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
                       "设置已保存。各选项的生效时机请查看选项提示。");
  }

  GtkWidget *window_;
  GtkWidget *nbest_;
  GtkWidget *pageSize_;
  GtkWidget *fuzzyDefault_;
  GtkWidget *fuzzyChoices_;
  std::array<GtkWidget *, fuzzyOptions.size()> fuzzyChecks_{};
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
