// Unpacks an Xbox 360 STFS package (the XBLA download, a LIVE/PIRS/CON file) into a folder.

#pragma once

#include <filesystem>
#include <string>

namespace hexic {

// Writes every file of the package under out_dir. Returns false and sets error on failure.
// shortcut: reads files as contiguous block runs only (true for XBLA packages); add hash-chain
// walking if a package with fragmented files turns up.
bool ExtractStfs(const std::filesystem::path& package, const std::filesystem::path& out_dir, std::string* error);

// True if the file starts with an STFS magic.
bool IsStfsPackage(const std::filesystem::path& file);

}  // namespace hexic
