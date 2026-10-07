// boom_boom_rocket - ReXGlue Recompiled Project

#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/cvar.h>
#include <rex/ui/window.h>
#include <rex/ui/window_sdl.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <rex/logging.h>
#include "title_screen_version.h"
#include "keyboard_driver.h"
#include "runtime_shortcuts.h"
#include "local_multiplayer.h"
#include <rex/input/input_system.h>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")

class BoomBoomRocketApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  std::string GetWindowTitle() const override {
    return "Boom Boom Rocket";
  }

  inline static BoomBoomRocketApp* current_instance_ = nullptr;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    auto app = std::unique_ptr<BoomBoomRocketApp>(
        new BoomBoomRocketApp(
            ctx,
            "boom_boom_rocket",
            PPCImageConfig));

    current_instance_ = app.get();
    return app;
  }

  static void SavePcSettings() {
    BoomBoomRocketApp* app = current_instance_;
    if (!app || app->config_path().empty()) {
      return;
    }

    // Use the exact same config file as the F4 settings overlay.
    rex::cvar::SaveConfig(app->config_path());
  }

  static void SetPcResolution(
      int32_t width,
      int32_t height,
      int32_t scale) {
    BoomBoomRocketApp* app = current_instance_;
    if (!app) {
      return;
    }

    bool changed = false;

    changed |= rex::cvar::SetFlagByName(
        "video_mode_width",
        std::to_string(width));

    changed |= rex::cvar::SetFlagByName(
        "video_mode_height",
        std::to_string(height));

    bool scale_changed = rex::cvar::SetFlagByName(
        "resolution_scale",
        std::to_string(scale));
    changed |= scale_changed;

    if (changed) {
      SavePcSettings();
    }

    // Rebuild scale-dependent graphics resources immediately.
    if (scale_changed && app->runtime() &&
        app->runtime()->graphics_system()) {
      app->runtime()->graphics_system()->ReinitializeContext();
    }

    // Apply the output/window resolution immediately.
    app->app_context().CallInUIThreadDeferred(
        [app, width, height]() {
          if (app->window()) {
            auto* sdl_window =
                static_cast<rex::ui::WindowSDL*>(app->window());

            sdl_window->SetLogicalSize(width, height);
          }
        });
  }

  static int32_t GetPcResolutionIndex() {
    std::string width =
        rex::cvar::GetFlagByName("video_mode_width");
    std::string height =
        rex::cvar::GetFlagByName("video_mode_height");

    if (width == "1280" && height == "720") {
      return 0;
    }
    if (width == "1920" && height == "1080") {
      return 1;
    }
    if (width == "2560" && height == "1440") {
      return 2;
    }
    if (width == "3840" && height == "2160") {
      return 3;
    }

    return 1;
  }
  static bool GetPcVSync() {
    std::string value = rex::cvar::GetFlagByName("vsync");
    return value == "true" || value == "1";
  }

  static void SetPcVSync(bool enabled) {
    if (rex::cvar::SetFlagByName(
            "vsync",
            enabled ? "true" : "false")) {
      SavePcSettings();
    }
  }
  static bool GetPcShowFps() {
    std::string value = rex::cvar::GetFlagByName("show_fps");
    return value == "true" || value == "1";
  }

  static void SetPcShowFps(bool enabled) {
    if (rex::cvar::SetFlagByName(
            "show_fps",
            enabled ? "true" : "false")) {
      SavePcSettings();
    }
  }

  static bool GetPcFullscreen() {
    std::string value = rex::cvar::GetFlagByName("fullscreen");
    return value == "true" || value == "1";
  }
  static void SetPcFullscreen(bool fullscreen) {
    BoomBoomRocketApp* app = current_instance_;
    if (!app) {
      return;
    }

    // Keep the ReXGlue CVar/F4 setting synchronized.
    if (rex::cvar::SetFlagByName(
            "fullscreen",
            fullscreen ? "true" : "false")) {
      SavePcSettings();
    }

    // Window state changes belong on the UI thread.
    app->app_context().CallInUIThreadDeferred(
        [app, fullscreen]() {
          if (app->window()) {
            app->window()->SetFullscreen(fullscreen);
          }
        });
  }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    wchar_t exe_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

    const auto app_directory =
        std::filesystem::path(exe_path).parent_path();

    // Keep only the five most recent runtime logs. Portable installs should
    // not accumulate an unbounded collection of diagnostic files.
    {
      const auto logs_path = app_directory / "logs";
      std::error_code iterator_error;
      std::vector<std::pair<
          std::filesystem::file_time_type,
          std::filesystem::path>> log_files;

      for (std::filesystem::directory_iterator iterator(
               logs_path, iterator_error), end;
           !iterator_error && iterator != end;
           iterator.increment(iterator_error)) {
        std::error_code entry_error;
        if (!iterator->is_regular_file(entry_error)) {
          continue;
        }

        const auto filename = iterator->path().filename().wstring();
        if (!filename.starts_with(L"boom_boom_rocket_") ||
            iterator->path().extension() != L".log") {
          continue;
        }

        const auto write_time =
            iterator->last_write_time(entry_error);
        if (!entry_error) {
          log_files.emplace_back(write_time, iterator->path());
        }
      }

      std::sort(
          log_files.begin(), log_files.end(),
          [](const auto& left, const auto& right) {
            return left.first > right.first;
          });

      constexpr std::size_t kRetainedLogCount = 5;
      for (std::size_t index = kRetainedLogCount;
           index < log_files.size(); ++index) {
        std::error_code remove_error;
        std::filesystem::remove(log_files[index].second, remove_error);
      }
    }

    // Keep saves, achievements, installed DLC and runtime caches alongside
    // the game so the entire installation can be moved as one portable folder.
    const auto user_data_path =
        app_directory / "userdata";

    paths.user_data_root = user_data_path;
    paths.cache_root = user_data_path / "cache";

    // ReXGlue preserves the Xbox 360 storage layout internally. Leave a
    // human-readable map beside it so portable installs are self-explanatory.
    {
      std::error_code error;
      std::filesystem::create_directories(user_data_path, error);

      std::ofstream readme(
          user_data_path / "README - SAVE AND DLC LOCATIONS.txt",
          std::ios::out | std::ios::trunc);

      if (readme) {
        readme
            << "Boom Boom Rocket - Portable User Data\n"
            << "=======================================\n\n"
            << "PLAYER 1 SAVE DATA\n"
            << "  5841086A\\profile\\User\\\n\n"
            << "PLAYER 2 SAVE DATA (local multiplayer)\n"
            << "  5841086A\\profile\\Player2\\\n\n"
            << "BOOM BOOM ROCK PACK DLC\n"
            << "  0000000000000000\\5841086A\\00000002\\\n\n"
            << "ACHIEVEMENT PROGRESS\n"
            << "  achievements\\5841086A.toml\n\n"
            << "RUNTIME/SHADER CACHE (safe to delete)\n"
            << "  cache\\\n\n"
            << "The numbered folders use the Xbox 360 storage layout.\n"
            << "Do not rename or move them individually. To back up or move\n"
            << "the game, copy the entire Boom Boom Rocket folder.\n";
      }
    }

    const auto assets_path =
        app_directory / "assets";

    const auto temp_path =
        app_directory / "assets_import_tmp";

    const auto extractor_path =
        app_directory / "tools" / "Extract-STFS.ps1";

    install_base_mode_ =
        HasCommandLineArgument(L"--install-base");

    install_dlc_mode_ =
        HasCommandLineArgument(L"--install-dlc");

    // Do not allow both installer modes at once.
    if (install_base_mode_ && install_dlc_mode_) {
      MessageBoxW(
          nullptr,
          L"Only one installer can be run at a time.",
          L"Boom Boom Rocket Setup",
          MB_OK | MB_ICONERROR);

      ExitProcess(1);
    }

    bool base_game_installed =
        std::filesystem::exists(
            assets_path / "default.xex");

    if (base_game_installed) {
      EnsurePcSettingStrings(assets_path);
    }

    // ------------------------------------------------------------
    // BASE GAME INSTALLER MODE
    // ------------------------------------------------------------
    if (install_base_mode_) {
      if (base_game_installed) {
        MessageBoxW(
            nullptr,
            L"Boom Boom Rocket is already installed.",
            L"Boom Boom Rocket Setup",
            MB_OK | MB_ICONINFORMATION);

        ExitProcess(0);
      }

      if (!InstallBaseGame(
              app_directory,
              assets_path,
              temp_path,
              extractor_path)) {
        ExitProcess(0);
      }

      MessageBoxW(
          nullptr,
          L"Boom Boom Rocket was installed successfully.\n\n"
          L"You can now run boom_boom_rocket.exe,\n"
          L"or install the optional Boom Boom Rock Pack DLC.",
          L"Installation Complete",
          MB_OK | MB_ICONINFORMATION);

      ExitProcess(0);
    }

    // ------------------------------------------------------------
    // DLC INSTALLER MODE
    // ------------------------------------------------------------
    if (install_dlc_mode_) {
      if (!base_game_installed) {
        MessageBoxW(
            nullptr,
            L"Boom Boom Rocket must be installed before the "
            L"Boom Boom Rock Pack DLC.\n\n"
            L"Please run:\n"
            L"Setup Boom Boom Rocket.exe in your portable folder",
            L"Base Game Required",
            MB_OK | MB_ICONINFORMATION);

        ExitProcess(0);
      }

      if (!SelectAndValidatePackage(
              true,
              pending_dlc_path_)) {
        ExitProcess(0);
      }

      dlc_install_only_mode_ = true;

      paths.game_data_root = assets_path;
      return;
    }

    // ------------------------------------------------------------
    // NORMAL GAME LAUNCH
    // ------------------------------------------------------------
    if (!base_game_installed) {
      int result = MessageBoxW(
          nullptr,
          L"Boom Boom Rocket game data has not been installed.\n\n"
          L"Would you like to install it now?\n\n"
          L"You can also close this window and run:\n"
          L"Setup Boom Boom Rocket.exe in your portable folder",
          L"Boom Boom Rocket Setup",
          MB_YESNO | MB_ICONINFORMATION);

      if (result != IDYES) {
        ExitProcess(0);
      }

      if (!InstallBaseGame(
              app_directory,
              assets_path,
              temp_path,
              extractor_path)) {
        ExitProcess(0);
      }

      MessageBoxW(
          nullptr,
          L"Boom Boom Rocket was installed successfully.\n\n"
          L"The game will now start.",
          L"Installation Complete",
          MB_OK | MB_ICONINFORMATION);
    }

    UpdateMainMenuText(
        assets_path,
        paths.user_data_root);
    paths.game_data_root = assets_path;
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    title_version_ = std::make_unique<BbrTitleVersion>(drawer, version_font_);
  }

  void OnConfigureFonts(ImFontAtlas* atlas) override {
    // Use Windows' installed font, not a redistributed font asset. Baking at
    // twice the display size gives the small label smooth proportional glyphs.
    char windows_dir[MAX_PATH]{};
    if (!GetWindowsDirectoryA(windows_dir, MAX_PATH)) return;
    const auto font = std::filesystem::path(windows_dir) / "Fonts" / "segoeuib.ttf";
    if (!std::filesystem::is_regular_file(font)) return;
    ImFontConfig config{};
    config.OversampleH = 3; config.OversampleV = 2;
    version_font_ = atlas->AddFontFromFileTTF(font.string().c_str(), 36.f, &config, atlas->GetGlyphRangesDefault());
  }

  void OnPostSetup() override {
    // Disable only the runtime bind dispatcher. Native game input has its own
    // listener; keeping ImGui preserves the title label and native Show FPS.
    runtime_shortcuts_enabled_ = rex::cvar::GetFlagByName("bbr_runtime_shortcuts") == "true";
    auto keyboard = std::make_unique<BbrKeyboardDriver>();
    keyboard->Setup();
    keyboard->set_is_active_callback([this] {
      return !bbr::overlay_captures_input.load(std::memory_order_relaxed) &&
             (!imgui_drawer() || !imgui_drawer()->HasOpenModalPopup());
    });
    keyboard->OnWindowAvailable(window());
    static_cast<rex::input::InputSystem*>(runtime()->input_system())->AddDriver(std::move(keyboard));
#if defined(BBR_MULTIPLAYER_TEST)
    auto second_test_pad = std::make_unique<BbrKeyboardDriver>(1, true);
    second_test_pad->Setup();
    second_test_pad->set_is_active_callback([this] {
      return !bbr::overlay_captures_input.load(std::memory_order_relaxed) &&
             (!imgui_drawer() || !imgui_drawer()->HasOpenModalPopup());
    });
    second_test_pad->OnWindowAvailable(window());
    static_cast<rex::input::InputSystem*>(runtime()->input_system())->AddDriver(std::move(second_test_pad));
#endif
    bbr::ConfigureLocalMultiplayer(runtime()->user_data_root(), [this] {
      rex::input::X_INPUT_CAPABILITIES caps{};
      return static_cast<rex::input::InputSystem*>(runtime()->input_system())->GetCapabilities(1, 0, &caps) == 0;
    });
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    config.gpu_plugin = "xenos";
    // The title-local driver supplies the requested mappings. Do not also run
    // the runtime's generic shooter bindings/cursor-capture driver.
    rex::cvar::SetFlagByName("mnk_mode", "false");
  }

  void OnKeyDown(rex::ui::KeyEvent& event) override {
    bbr::DispatchRuntimeShortcut(runtime_shortcuts_enabled_, event);
  }

  // ReXGlue creates its normal game window before constructing Runtime.
  // In DLC installer mode, immediately hide that temporary window so the
  // installer does not appear to launch the game.
  bool SetupPresentation() override {
    if (!rex::ReXApp::SetupPresentation()) {
      return false;
    }

    // Use the same embedded artwork for Explorer, the title bar and Alt-Tab.
    const auto module = GetModuleHandleW(nullptr);
    const auto resource = FindResourceW(module, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(10));
    if (resource && window()) {
      const auto data = LoadResource(module, resource);
      if (data) window()->SetIcon(LockResource(data), SizeofResource(module, resource));
    }

    if (install_dlc_mode_ && window()) {
      void* native_handle =
          window()->GetNativeWindowHandle();

      if (native_handle) {
        ShowWindow(
            static_cast<HWND>(native_handle),
            SW_HIDE);
      }
    }

    return true;
  }

  void OnPreLaunchModule() override {
    REXLOG_CAT_INFO(
        rex::log::core(),
        "BBR: OnPreLaunchModule");

    // Normal game launch.
    if (!dlc_install_only_mode_) {
      return;
    }

    // DLC installer mode.
    if (pending_dlc_path_.empty()) {
      MessageBoxW(
          nullptr,
          L"No DLC package was selected.",
          L"DLC Installation",
          MB_OK | MB_ICONERROR);

      ExitProcess(1);
    }

    auto* kernel_state =
        runtime()->kernel_state();

    if (!kernel_state ||
        !kernel_state->content_manager()) {
      MessageBoxW(
          nullptr,
          L"The Boom Boom Rock Pack could not be installed "
          L"because ReXGlue's content manager is unavailable.",
          L"DLC Installation Failed",
          MB_OK | MB_ICONERROR);

      ExitProcess(1);
    }

    auto result =
        kernel_state
            ->content_manager()
            ->InstallContent(pending_dlc_path_);

    pending_dlc_path_.clear();

    if (result != 0) {
      MessageBoxW(
          nullptr,
          L"The Boom Boom Rock Pack could not be installed.",
          L"DLC Installation Failed",
          MB_OK | MB_ICONERROR);

      ExitProcess(1);
    }

    MessageBoxW(
        nullptr,
        L"Boom Boom Rock Pack was installed successfully.\n\n"
        L"You can now run boom_boom_rocket.exe to play.",
        L"DLC Installation Complete",
        MB_OK | MB_ICONINFORMATION);

    // Installer mode ends here. Do not launch the game.
    ExitProcess(0);
  }

  void OnPostLaunchModule(
      rex::system::XThread* thread) override {
    REXLOG_CAT_INFO(
        rex::log::core(),
        "BBR: OnPostLaunchModule");
  }

  void OnGuestThreadExit(
      rex::system::XThread* thread) override {
    REXLOG_CAT_INFO(
        rex::log::core(),
        "BBR: OnGuestThreadExit");
  }

  void OnShutdown() override {
    REXLOG_CAT_INFO(
        rex::log::core(),
        "BBR: OnShutdown");

    // Guest-initiated shutdown reaches ReXGlue's subsystem teardown after the
    // guest thread has already completed. That teardown can intermittently
    // deadlock, leaving the process behind. Match ReXGlue's window-close path:
    // preserve the final log messages and let the OS reclaim runtime resources.
    REXLOG_CAT_INFO(
        rex::log::core(),
        "BBR: Guest shutdown complete; hard-exiting process");
    rex::FlushLogging();
    std::_Exit(0);
  }

 private:
  enum class PackageValidationResult {
    kValid,
    kCouldNotOpen,
    kNotLive,
    kMetadataError,
    kWrongPackage,
  };

  static bool HasCommandLineArgument(
      const wchar_t* argument) {
    int argument_count = 0;

    LPWSTR* arguments =
        CommandLineToArgvW(
            GetCommandLineW(),
            &argument_count);

    if (!arguments) {
      return false;
    }

    bool found = false;

    for (int i = 1; i < argument_count; ++i) {
      if (_wcsicmp(arguments[i], argument) == 0) {
        found = true;
        break;
      }
    }

    LocalFree(arguments);
    return found;
  }

  static PackageValidationResult ValidatePackage(
      const std::filesystem::path& package_path,
      std::uint32_t expected_content_type) {
    std::ifstream package_file(
        package_path,
        std::ios::binary);

    if (!package_file) {
      return PackageValidationResult::kCouldNotOpen;
    }

    // Microsoft LIVE STFS package signature.
    char signature[4] = {};
    package_file.read(signature, 4);

    if (!package_file ||
        signature[0] != 'L' ||
        signature[1] != 'I' ||
        signature[2] != 'V' ||
        signature[3] != 'E') {
      return PackageValidationResult::kNotLive;
    }

    // STFS Content Type at offset 0x344.
    package_file.seekg(0x344);

    unsigned char content_type_bytes[4] = {};
    package_file.read(
        reinterpret_cast<char*>(
            content_type_bytes),
        4);

    if (!package_file) {
      return PackageValidationResult::kMetadataError;
    }

    std::uint32_t content_type =
        (static_cast<std::uint32_t>(
             content_type_bytes[0]) << 24) |
        (static_cast<std::uint32_t>(
             content_type_bytes[1]) << 16) |
        (static_cast<std::uint32_t>(
             content_type_bytes[2]) << 8) |
        static_cast<std::uint32_t>(
            content_type_bytes[3]);

    // STFS Title ID at offset 0x360.
    package_file.seekg(0x360);

    unsigned char title_id_bytes[4] = {};
    package_file.read(
        reinterpret_cast<char*>(
            title_id_bytes),
        4);

    if (!package_file) {
      return PackageValidationResult::kMetadataError;
    }

    std::uint32_t title_id =
        (static_cast<std::uint32_t>(
             title_id_bytes[0]) << 24) |
        (static_cast<std::uint32_t>(
             title_id_bytes[1]) << 16) |
        (static_cast<std::uint32_t>(
             title_id_bytes[2]) << 8) |
        static_cast<std::uint32_t>(
            title_id_bytes[3]);

    // Boom Boom Rocket Title ID = 5841086A
    if (title_id != 0x5841086A ||
        content_type != expected_content_type) {
      return PackageValidationResult::kWrongPackage;
    }

    return PackageValidationResult::kValid;
  }

  static bool SelectAndValidatePackage(
      bool dlc,
      std::filesystem::path& selected_path) {
    while (true) {
      wchar_t selected_file[MAX_PATH] = {};

      OPENFILENAMEW dialog = {};
      dialog.lStructSize = sizeof(dialog);
      dialog.hwndOwner = nullptr;
      dialog.lpstrFile = selected_file;
      dialog.nMaxFile = MAX_PATH;

      if (dlc) {
        dialog.lpstrFilter =
            L"Xbox 360 DLC Package\0*.*\0"
            L"All Files\0*.*\0";

        dialog.lpstrTitle =
            L"Select your Boom Boom Rock Pack DLC package";
      } else {
        dialog.lpstrFilter =
            L"Xbox 360 XBLA Package\0*.*\0"
            L"All Files\0*.*\0";

        dialog.lpstrTitle =
            L"Select your original Boom Boom Rocket XBLA package";
      }

      dialog.nFilterIndex = 1;
      dialog.Flags =
          OFN_FILEMUSTEXIST |
          OFN_PATHMUSTEXIST |
          OFN_NOCHANGEDIR;

      if (!GetOpenFileNameW(&dialog)) {
        return false;
      }

      std::filesystem::path package_path(
          selected_file);

      const std::uint32_t expected_content_type =
          dlc ? 0x00000002 : 0x000D0000;

      PackageValidationResult validation =
          ValidatePackage(
              package_path,
              expected_content_type);

      if (validation ==
          PackageValidationResult::kValid) {
        selected_path = package_path;
        return true;
      }

      const wchar_t* message = nullptr;
      const wchar_t* title = nullptr;

      switch (validation) {
        case PackageValidationResult::kCouldNotOpen:
          message =
              dlc
                  ? L"The selected DLC package could not "
                    L"be opened.\n\n"
                    L"Would you like to select another file?"
                  : L"The selected game package could not "
                    L"be opened.\n\n"
                    L"Would you like to select another file?";
          title = L"Invalid Package";
          break;

        case PackageValidationResult::kNotLive:
          message =
              L"The selected file is not a valid Xbox 360 "
              L"LIVE package.\n\n"
              L"Would you like to select another file?";
          title = L"Invalid Package";
          break;

        case PackageValidationResult::kMetadataError:
          message =
              L"The Xbox 360 package metadata could not "
              L"be read.\n\n"
              L"Would you like to select another file?";
          title = L"Invalid Package";
          break;

        case PackageValidationResult::kWrongPackage:
          message =
              dlc
                  ? L"The selected package is not the "
                    L"Boom Boom Rock Pack DLC.\n\n"
                    L"Would you like to select another file?"
                  : L"The selected package is not "
                    L"Boom Boom Rocket.\n\n"
                    L"Would you like to select another file?";
          title =
              dlc
                  ? L"Incorrect DLC"
                  : L"Incorrect Game";
          break;

        default:
          return false;
      }

      int retry = MessageBoxW(
          nullptr,
          message,
          title,
          MB_YESNO | MB_ICONERROR);

      if (retry != IDYES) {
        return false;
      }
    }
  }

  static void EnsurePcSettingStrings(
      const std::filesystem::path& assets_path) {
    const wchar_t* language_files[] = {
        L"English.ini",
        L"French.ini",
        L"german.ini",
        L"Italian.ini",
        L"spanish.ini",
    };

    const std::wstring pc_strings =
        L"IDS_PC_DISPLAY_MODE = Display Mode\r\n"
        L"IDS_PC_RESOLUTION = Resolution\r\n"
        L"IDS_PC_VSYNC = VSync\r\n"
        L"IDS_PC_FRAME_LIMIT = Show FPS\r\n"
        L"IDS_PC_FULLSCREEN = Fullscreen\r\n"
        L"IDS_PC_WINDOWED = Windowed\r\n"
        L"IDS_PC_RES_720P = 1280x720\r\n"
        L"IDS_PC_RES_1080P = 1920x1080\r\n"
        L"IDS_PC_RES_1440P = 2560x1440\r\n"
        L"IDS_PC_RES_2160P = 3840x2160\r\n"
        L"IDS_PC_FRAME_UNLIMITED = Unlimited\r\n"
        L"IDS_PC_FRAME_60 = 60 FPS\r\n"
        L"IDS_PC_FRAME_120 = 120 FPS\r\n"
        L"IDS_PC_FRAME_144 = 144 FPS\r\n"
        L"IDS_PC_FRAME_165 = 165 FPS\r\n"
        L"IDS_PC_FRAME_240 = 240 FPS\r\n"
        L"IDS_PC_KEYBOARD_CONTROLS = Keyboard Controls\r\n"
        L"IDS_PC_KEYS_MOVE = Move: W A S D or Arrow keys\r\n"
        L"IDS_PC_KEYS_START = Start: Enter or Space\r\n"
        L"IDS_PC_KEYS_PAUSE = Pause / Resume: Esc during gameplay\r\n"
        L"IDS_PC_KEYS_A = A: E or Left mouse button\r\n"
        L"IDS_PC_KEYS_B = B: B, Backspace or Right mouse button\r\n"
        L"IDS_PC_KEYS_X = X: X key\r\n"
        L"IDS_PC_KEYS_Y = Y: Y key\r\n"
        L"IDS_PC_KEYS_MENU_ESC = Esc: Back in menus\r\n"
        L"IDS_PC_KEYS_PLAYER = Keyboard controls Player 1\r\n"
        L"IDS_PC_TIMING_CALIBRATION = Timing Calibration\r\n"
        L"IDS_PC_TIMING_OFFSET = Timing Offset\r\n"
        L"IDS_PC_TIMING_INFO = Adjust note judgement for your display / audio.\r\n"
        L"IDS_PC_TIMING_LATE = Positive offset: accept later hits.\r\n"
        L"IDS_PC_TIMING_EARLY = Negative offset: accept earlier hits.\r\n"
        L"IDS_PC_TIMING_KEYS = Left / Right: adjust   X: reset   A: save\r\n"
        L"IDS_PC_TIMING_NEXT_SONG = Changes apply from your next song. B cancels.\r\n"
        L"IDS_PC_TIMING_VALUE_0 = -200 ms\r\n"
        L"IDS_PC_TIMING_VALUE_1 = -190 ms\r\n"
        L"IDS_PC_TIMING_VALUE_2 = -180 ms\r\n"
        L"IDS_PC_TIMING_VALUE_3 = -170 ms\r\n"
        L"IDS_PC_TIMING_VALUE_4 = -160 ms\r\n"
        L"IDS_PC_TIMING_VALUE_5 = -150 ms\r\n"
        L"IDS_PC_TIMING_VALUE_6 = -140 ms\r\n"
        L"IDS_PC_TIMING_VALUE_7 = -130 ms\r\n"
        L"IDS_PC_TIMING_VALUE_8 = -120 ms\r\n"
        L"IDS_PC_TIMING_VALUE_9 = -110 ms\r\n"
        L"IDS_PC_TIMING_VALUE_10 = -100 ms\r\n"
        L"IDS_PC_TIMING_VALUE_11 = -90 ms\r\n"
        L"IDS_PC_TIMING_VALUE_12 = -80 ms\r\n"
        L"IDS_PC_TIMING_VALUE_13 = -70 ms\r\n"
        L"IDS_PC_TIMING_VALUE_14 = -60 ms\r\n"
        L"IDS_PC_TIMING_VALUE_15 = -50 ms\r\n"
        L"IDS_PC_TIMING_VALUE_16 = -40 ms\r\n"
        L"IDS_PC_TIMING_VALUE_17 = -30 ms\r\n"
        L"IDS_PC_TIMING_VALUE_18 = -20 ms\r\n"
        L"IDS_PC_TIMING_VALUE_19 = -10 ms\r\n"
        L"IDS_PC_TIMING_VALUE_20 = 0 ms\r\n"
        L"IDS_PC_TIMING_VALUE_21 = +10 ms\r\n"
        L"IDS_PC_TIMING_VALUE_22 = +20 ms\r\n"
        L"IDS_PC_TIMING_VALUE_23 = +30 ms\r\n"
        L"IDS_PC_TIMING_VALUE_24 = +40 ms\r\n"
        L"IDS_PC_TIMING_VALUE_25 = +50 ms\r\n"
        L"IDS_PC_TIMING_VALUE_26 = +60 ms\r\n"
        L"IDS_PC_TIMING_VALUE_27 = +70 ms\r\n"
        L"IDS_PC_TIMING_VALUE_28 = +80 ms\r\n"
        L"IDS_PC_TIMING_VALUE_29 = +90 ms\r\n"
        L"IDS_PC_TIMING_VALUE_30 = +100 ms\r\n"
        L"IDS_PC_TIMING_VALUE_31 = +110 ms\r\n"
        L"IDS_PC_TIMING_VALUE_32 = +120 ms\r\n"
        L"IDS_PC_TIMING_VALUE_33 = +130 ms\r\n"
        L"IDS_PC_TIMING_VALUE_34 = +140 ms\r\n"
        L"IDS_PC_TIMING_VALUE_35 = +150 ms\r\n"
        L"IDS_PC_TIMING_VALUE_36 = +160 ms\r\n"
        L"IDS_PC_TIMING_VALUE_37 = +170 ms\r\n"
        L"IDS_PC_TIMING_VALUE_38 = +180 ms\r\n"
        L"IDS_PC_TIMING_VALUE_39 = +190 ms\r\n"
        L"IDS_PC_TIMING_VALUE_40 = +200 ms\r\n"
        L"IDS_PC_RESET_DISPLAY = Reset Display Settings\r\n"
        L"IDS_PC_PLAYER_ONE_NAME = Player 1 Name\r\n"
        L"IDS_PC_PLAYER_TWO_NAME = Player 2 Name\r\n"
        L"IDS_PC_PRESS_A = Press A\r\n"
        L"IDS_PC_PRESS_ANY_KEY = Press Any Key";

    for (const wchar_t* language_file : language_files) {
      const auto ini_path =
          assets_path / "UI" / "Text" / language_file;

      std::ifstream input(
          ini_path,
          std::ios::binary | std::ios::ate);

      if (!input) {
        continue;
      }

      const auto file_size =
          static_cast<std::size_t>(input.tellg());

      if (file_size < sizeof(wchar_t) ||
          ((file_size - sizeof(wchar_t)) % sizeof(wchar_t)) != 0) {
        continue;
      }

      input.seekg(0);

      wchar_t bom = 0;
      input.read(
          reinterpret_cast<char*>(&bom),
          sizeof(bom));

      if (bom != 0xFEFF) {
        continue;
      }

      std::wstring text(
          (file_size - sizeof(wchar_t)) / sizeof(wchar_t),
          L'\0');

      input.read(
          reinterpret_cast<char*>(text.data()),
          static_cast<std::streamsize>(
              text.size() * sizeof(wchar_t)));
      input.close();

      // Upgrade older portable installations without duplicating or shifting
      // their existing string IDs. Append new IDs in the authored order only.
      bool changed = false;
      for (size_t start=0; start<pc_strings.size();) {
        const auto end = pc_strings.find(L"\r\n", start);
        const auto line = pc_strings.substr(start, end == std::wstring::npos
            ? std::wstring::npos : end-start);
        const auto separator = line.find(L" =");
        if (separator != std::wstring::npos &&
            text.find(line.substr(0,separator)+L" =") == std::wstring::npos) {
          if (!text.empty() && text.back()!=L'\n') text+=L"\r\n";
          text += line+L"\r\n";
          changed = true;
        }
        if (end == std::wstring::npos) break;
        start = end+2;
      }
      if (!changed) continue;

      std::ofstream output(
          ini_path,
          std::ios::binary | std::ios::trunc);

      if (!output) {
        continue;
      }

      output.write(
          reinterpret_cast<const char*>(&bom),
          sizeof(bom));
      output.write(
          reinterpret_cast<const char*>(text.data()),
          static_cast<std::streamsize>(
              text.size() * sizeof(wchar_t)));
    }
  }

  static bool InstallBaseGame(
      const std::filesystem::path& app_directory,
      const std::filesystem::path& assets_path,
      const std::filesystem::path& temp_path,
      const std::filesystem::path& extractor_path) {
    if (!std::filesystem::exists(
            extractor_path)) {
      MessageBoxW(
          nullptr,
          L"The game data extraction tool could not be found.\n\n"
          L"Please reinstall the Boom Boom Rocket package.",
          L"Setup Error",
          MB_OK | MB_ICONERROR);

      return false;
    }

    std::filesystem::path package_path;

    if (!SelectAndValidatePackage(
            false,
            package_path)) {
      return false;
    }

    std::error_code error;

    // Always extract into a temporary directory.
    std::filesystem::remove_all(
        temp_path,
        error);

    error.clear();

    std::filesystem::create_directories(
        temp_path,
        error);

    if (error) {
      MessageBoxW(
          nullptr,
          L"Could not create the temporary extraction folder.",
          L"Installation Failed",
          MB_OK | MB_ICONERROR);

      return false;
    }

    std::wstring command_line =
        L"powershell.exe -NoProfile "
        L"-ExecutionPolicy Bypass "
        L"-File \"" +
        extractor_path.wstring() +
        L"\" -Path \"" +
        package_path.wstring() +
        L"\" -OutputDir \"" +
        temp_path.wstring() +
        L"\"";

    std::vector<wchar_t> command_buffer(
        command_line.begin(),
        command_line.end());

    command_buffer.push_back(L'\0');

    STARTUPINFOW startup_info = {};
    startup_info.cb = sizeof(startup_info);

    PROCESS_INFORMATION process_info = {};

    BOOL process_created =
        CreateProcessW(
            nullptr,
            command_buffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            app_directory.c_str(),
            &startup_info,
            &process_info);

    if (!process_created) {
      std::filesystem::remove_all(
          temp_path,
          error);

      MessageBoxW(
          nullptr,
          L"The Xbox 360 package extractor could not be started.",
          L"Installation Failed",
          MB_OK | MB_ICONERROR);

      return false;
    }

    WaitForSingleObject(
        process_info.hProcess,
        INFINITE);

    DWORD exit_code = 1;

    GetExitCodeProcess(
        process_info.hProcess,
        &exit_code);

    CloseHandle(
        process_info.hThread);

    CloseHandle(
        process_info.hProcess);

    if (exit_code != 0) {
      std::filesystem::remove_all(
          temp_path,
          error);

      MessageBoxW(
          nullptr,
          L"The Xbox 360 package could not be extracted.",
          L"Installation Failed",
          MB_OK | MB_ICONERROR);

      return false;
    }

    // Confirm expected Boom Boom Rocket files exist.
    bool valid_game =
        std::filesystem::exists(
            temp_path / "default.xex") &&
        std::filesystem::exists(
            temp_path /
            "Content" /
            "contents.txt") &&
        std::filesystem::exists(
            temp_path /
            "UI" /
            "Text" /
            "English.ini") &&
        std::filesystem::exists(
            temp_path /
            "Textures" /
            "CityTextures.xpr");

    if (!valid_game) {
      std::filesystem::remove_all(
          temp_path,
          error);

      MessageBoxW(
          nullptr,
          L"The package was extracted, but it does not "
          L"contain the expected Boom Boom Rocket game data.",
          L"Incorrect Game",
          MB_OK | MB_ICONERROR);

      return false;
    }

    // Install extracted game data.
    std::filesystem::remove_all(
        assets_path,
        error);

    error.clear();

    std::filesystem::rename(
        temp_path,
        assets_path,
        error);

    if (error) {
      MessageBoxW(
          nullptr,
          L"The extracted Boom Boom Rocket game data "
          L"could not be installed.",
          L"Installation Failed",
          MB_OK | MB_ICONERROR);

      return false;
    }

    EnsurePcSettingStrings(assets_path);

    return true;
  }

  void UpdateMainMenuText(
      const std::filesystem::path& assets_path,
      const std::filesystem::path& user_data_root) {
    const auto dlc_root =
        user_data_root /
        "0000000000000000" /
        "5841086A" /
        "00000002";

    bool dlc_installed = false;
    std::error_code error;

    if (std::filesystem::exists(dlc_root, error)) {
      std::filesystem::recursive_directory_iterator it(
          dlc_root,
          std::filesystem::directory_options::skip_permission_denied,
          error);

      const std::filesystem::recursive_directory_iterator end;

      while (it != end) {
        if (!error &&
            it->is_regular_file(error) &&
            it->path().filename() == L"CanonInDMajor.xma") {
          dlc_installed = true;
          break;
        }

        error.clear();
        it.increment(error);
      }
    }

    const auto ini_path =
        assets_path /
        "UI" /
        "Text" /
        "English.ini";

    std::ifstream input(
        ini_path,
        std::ios::binary | std::ios::ate);

    if (!input) {
      return;
    }

    const auto file_size =
        static_cast<std::size_t>(input.tellg());

    if (file_size < sizeof(wchar_t) ||
        ((file_size - sizeof(wchar_t)) % sizeof(wchar_t)) != 0) {
      return;
    }

    input.seekg(0);

    wchar_t bom = 0;
    input.read(
        reinterpret_cast<char*>(&bom),
        sizeof(bom));

    if (bom != 0xFEFF) {
      return;
    }

    std::wstring text(
        (file_size - sizeof(wchar_t)) / sizeof(wchar_t),
        L'\0');

    input.read(
        reinterpret_cast<char*>(text.data()),
        static_cast<std::streamsize>(
            text.size() * sizeof(wchar_t)));

    input.close();

    const std::wstring key =
        L"IDS_DOWNLOADABLE_CONTENT_INFO = ";

    const auto start = text.find(key);

    if (start == std::wstring::npos) {
      return;
    }

    auto end_of_line =
        text.find_first_of(L"\r\n", start);

    if (end_of_line == std::wstring::npos) {
      end_of_line = text.size();
    }

    const std::wstring message =
        dlc_installed
            ? L"Boom Boom Rock Pack DLC is installed."
            : L"To install the Boom Boom Rocket Rock Pack DLC please exit the game and run Setup Boom Boom Rocket.exe.";

    text.replace(
        start,
        end_of_line - start,
        key + message);

    // Replace values in place so the game's string table IDs remain unchanged.
    const auto replace_menu_label = [&text](
        const std::wstring& label_key,
        const std::wstring& value) {
      const auto label_start = text.find(label_key);
      if (label_start == std::wstring::npos) {
        return;
      }
      const auto label_end = text.find_first_of(L"\r\n", label_start);
      text.replace(
          label_start,
          (label_end == std::wstring::npos ? text.size() : label_end) - label_start,
          label_key + value);
    };
    replace_menu_label(L"IDS_RETURN_TO_ARCADE = ", L"Exit Game");
    replace_menu_label(L"IDS_CONTROLS = ", L"Gamepad Controls");
    replace_menu_label(L"IDS_MULTI_PLAYER_INFO = ", L"Battle against a friend locally. Connect two controllers.");
    replace_menu_label(L"IDS_NOT_ENOUGH_USERS = ",
        L"Local multiplayer requires two controllers. Connect both controllers, then restart Boom Boom Rocket if Player 2 is not detected.");
    replace_menu_label(L"IDS_PROFILES_CHANGED = ",
        L"A controller was disconnected. Reconnect both controllers, then press Start to reload the local profiles.");
    replace_menu_label(
        L"IDS_RETURN_TO_ARCADE_INFO = ",
        L"Quit Boom Boom Rocket and return to desktop.");

    std::ofstream output(
        ini_path,
        std::ios::binary | std::ios::trunc);

    if (!output) {
      return;
    }

    bom = 0xFEFF;

    output.write(
        reinterpret_cast<const char*>(&bom),
        sizeof(bom));

    output.write(
        reinterpret_cast<const char*>(text.data()),
        static_cast<std::streamsize>(
            text.size() * sizeof(wchar_t)));
  }
  bool install_base_mode_ = false;
  bool runtime_shortcuts_enabled_ = false;
  std::unique_ptr<BbrTitleVersion> title_version_;
  ImFont* version_font_ = nullptr;
  bool install_dlc_mode_ = false;
  bool dlc_install_only_mode_ = false;

  std::filesystem::path pending_dlc_path_;
};
