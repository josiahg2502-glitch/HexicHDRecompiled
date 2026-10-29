// hexic - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/input/input_system.h>
#include <rex/logging.h>
#include <rex/rex_app.h>

#include "script_input.h"
#include "stfs_extract.h"

REXCVAR_DECLARE(std::string, input_script);

class HexicApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<HexicApp>(new HexicApp(ctx, "hexic",
        PPCImageConfig));
  }

  // Seed the user's shader storage from the precompiled cache shipped next to the exe, so every
  // known shader is translated and its pipeline built at startup, not on first use.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path exe_dir = rex::filesystem::GetExecutableFolder();
    // Without --game_data_root, look for the game in "game" next to the exe, unpacking the
    // XBLA package there on first run.
    if (paths.game_data_root.empty()) {
      fs::path game = exe_dir / "game";
      if (!fs::is_regular_file(game / "default.xex", ec)) InstallFromPackage(game);
      if (fs::is_regular_file(game / "default.xex", ec)) paths.game_data_root = game;
    }
    fs::path shipped = exe_dir / "shader_cache";
    for (auto& entry : fs::recursive_directory_iterator(shipped, ec)) {
      if (!entry.is_regular_file()) continue;
      fs::path dest = paths.cache_root / fs::relative(entry.path(), shipped);
      // Storage files only grow, so a smaller user copy predates the shipped one.
      if (fs::exists(dest) && fs::file_size(dest, ec) >= entry.file_size()) continue;
      fs::create_directories(dest.parent_path(), ec);
      if (!fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing, ec))
        REXLOG_WARN("shader cache: could not seed {}: {}", dest.string(), ec.message());
    }
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    // The game needs the Xenos GPU plugin; don't require --gpu_plugin on every launch.
    if (config.gpu_plugin.empty()) config.gpu_plugin = "xenos";
  }

  // Unpacks the first STFS package found in game/ (the 584107D1... file from the console) into
  // game/. Files land in a staging folder first and default.xex moves last, so an interrupted
  // install is redone on the next start.
  static void InstallFromPackage(const std::filesystem::path& game) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path staging = game / ".install", package;
    fs::remove_all(staging, ec);
    for (auto& entry : fs::recursive_directory_iterator(game, ec)) {
      if (entry.is_regular_file() && entry.file_size() >= 0x10000 && hexic::IsStfsPackage(entry.path())) {
        package = entry.path();
        break;
      }
    }
    if (package.empty()) return;
    std::string error;
    if (!hexic::ExtractStfs(package, staging, &error)) {
      REXLOG_ERROR("install: {}: {}", package.string(), error);
#ifdef _WIN32
      MessageBoxA(nullptr, ("Could not unpack " + package.filename().string() + ":\n" + error).c_str(),
                  "Hexic HD", MB_ICONERROR);
#endif
      return;
    }
    for (auto& f : fs::directory_iterator(staging, ec))
      if (f.path().filename() != "default.xex") fs::rename(f.path(), game / f.path().filename(), ec);
    fs::rename(staging / "default.xex", game / "default.xex", ec);
    fs::remove_all(staging, ec);
    REXLOG_INFO("install: unpacked {}", package.string());
  }

  void OnPostSetup() override {
    auto* input = static_cast<rex::input::InputSystem*>(runtime()->input_system());
    if (input && !REXCVAR_GET(input_script).empty()) {
      if (auto driver = hexic::ScriptInputDriver::Load(REXCVAR_GET(input_script)))
        input->AddDriver(std::move(driver));  // InputSystem re-enumerates drivers on every poll
    }
  }
};
