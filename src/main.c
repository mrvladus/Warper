#include <adwaita.h>
#include <libportal/portal.h>

const char *app_id = "io.github.mrvladus.Warper";
const char *styles = "switch{min-width:120px;min-height:60px;border-radius:60px;}"
                     ".warp-label{font-size:64px;font-weight:1000;color:@orange_4;}";
bool flatpak;
GtkWidget *toggle;
GtkWidget *label;
GtkWidget *not_installed_label;
GtkWidget *refresh_btn;

static char *run_cmd(const char *cmd) {
  g_autofree gchar *command = g_strdup_printf("%s%s", flatpak ? "flatpak-spawn --host " : "", cmd);
  printf("[Warper] Run command: %s\n", command);
  GSubprocess *subprocess =
      g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE, NULL,
                       "/bin/sh", "-c", command, NULL);
  g_autoptr(GInputStream) stdout_stream = g_subprocess_get_stdout_pipe(subprocess);
  g_autoptr(GDataInputStream) data_stream = g_data_input_stream_new(stdout_stream);
  gsize length = 0;
  char *output = g_data_input_stream_read_upto(data_stream, "\0", 0, &length, NULL, NULL);
  if (output)
    printf("%s", output);
  return output;
}

static bool get_status() {
  char *res = run_cmd("warp-cli status | grep 'Status update:'");
  bool out = false;
  if (!res)
    return out;
  if (strstr(res, "Connect"))
    out = true;
  free(res);
  return out;
}

static bool get_warp_cli_installed() {
  char *res = run_cmd("warp-cli --version");
  bool out = false;
  if (!res)
    return out;
  if (strstr(res, "warp-cli 20"))
    out = true;
  free(res);
  return out;
}

static void update_ui() {
  bool installed = get_warp_cli_installed();
  gtk_widget_set_visible(not_installed_label, !installed);
  gtk_widget_set_visible(refresh_btn, !installed);
  gtk_widget_set_visible(toggle, installed);
  gtk_widget_set_visible(label, installed);
}

static void about_cb(GtkButton *btn) {
  adw_show_about_dialog(GTK_WIDGET(btn), "application-name", "Warper", "application-icon", app_id,
                        "version", "1.0", "copyright", "© 2025 Vlad Krupinskii", "website",
                        "https://github.com/mrvladus/Warper", "issue-url",
                        "https://github.com/mrvladus/Warper/issues/", "license-type",
                        GTK_LICENSE_MIT_X11, NULL);
}

static void toggle_cb(GtkSwitch *btn, bool active) {
  char *res = active ? run_cmd("warp-cli connect") : run_cmd("warp-cli disconnect");
  if (res && strstr(res, "Success")) {
    gtk_label_set_label(GTK_LABEL(label), active ? "Connected" : "Disconnected");
    gtk_widget_remove_css_class(label, active ? "error" : "success");
    gtk_widget_add_css_class(label, active ? "success" : "error");
    free(res);
  }
}

static void activate_cb(GtkApplication *app) {
  // Window
  GtkWidget *window = adw_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Warper");
  gtk_window_set_resizable(GTK_WINDOW(window), false);
  // Toolbar View
  GtkWidget *tb = adw_toolbar_view_new();
  adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), tb);
  // Main Box
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
  g_object_set(box, "valign", GTK_ALIGN_CENTER, "margin-bottom", 36, NULL);
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(tb), box);
  // Header Bar
  GtkWidget *hb = adw_header_bar_new();
  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(tb), hb);
  // About button
  GtkWidget *about_btn = gtk_button_new_from_icon_name("help-about-symbolic");
  g_signal_connect(about_btn, "clicked", G_CALLBACK(about_cb), NULL);
  adw_header_bar_pack_start(ADW_HEADER_BAR(hb), about_btn);
  // WARP logo
  GtkWidget *warp_label = gtk_label_new("WARP");
  gtk_widget_add_css_class(warp_label, "warp-label");
  gtk_box_append(GTK_BOX(box), warp_label);
  // Status
  not_installed_label = gtk_label_new(
      "<a href='https://developers.cloudflare.com/warp-client/get-started/linux'>warp-cli</a> is "
      "not installed");
  g_object_set(not_installed_label, "use-markup", true, "wrap", true, "wrap-mode", 2, NULL);
  gtk_widget_add_css_class(not_installed_label, "title-1");
  gtk_box_append(GTK_BOX(box), not_installed_label);
  // Refresh btn
  refresh_btn = gtk_button_new_with_label("Refresh");
  g_object_set(refresh_btn, "halign", GTK_ALIGN_CENTER, NULL);
  g_signal_connect(refresh_btn, "clicked", G_CALLBACK(update_ui), NULL);
  gtk_widget_add_css_class(refresh_btn, "pill");
  gtk_box_append(GTK_BOX(box), refresh_btn);
  // Get first status
  bool connected = get_status();
  // Toggle
  toggle = gtk_switch_new();
  g_object_set(toggle, "halign", GTK_ALIGN_CENTER, "active", connected, NULL);
  g_signal_connect(toggle, "state-set", G_CALLBACK(toggle_cb), NULL);
  gtk_box_append(GTK_BOX(box), toggle);
  // Status label
  label = gtk_label_new(connected ? "Connected" : "Disconnected");
  gtk_widget_add_css_class(label, "title-1");
  gtk_widget_add_css_class(label, connected ? "success" : "error");
  gtk_box_append(GTK_BOX(box), label);
  // Add custom CSS
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(provider, styles);
  gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  // Show window
  update_ui();
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char *argv[]) {
  printf("[Warper] Start Warper\n");
  flatpak = xdp_portal_running_under_flatpak();
  printf("[Warper] Flatpak: %s\n", flatpak ? "true" : "false");
  g_autoptr(AdwApplication) app = adw_application_new(app_id, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate_cb), NULL);
  return g_application_run(G_APPLICATION(app), argc, argv);
}
