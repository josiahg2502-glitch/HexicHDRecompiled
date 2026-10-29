#include "stfs_extract.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace hexic {
namespace {

uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
uint32_t le24(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16; }

}  // namespace

bool IsStfsPackage(const std::filesystem::path& file) {
  char magic[4] = {};
  std::ifstream(file, std::ios::binary).read(magic, 4);
  return !memcmp(magic, "LIVE", 4) || !memcmp(magic, "PIRS", 4) || !memcmp(magic, "CON ", 4);
}

bool ExtractStfs(const std::filesystem::path& package, const std::filesystem::path& out_dir, std::string* error) {
  namespace fs = std::filesystem;
  std::ifstream in(package, std::ios::binary);
  std::vector<uint8_t> d((std::istreambuf_iterator<char>(in)), {});
  if (d.size() < 0x1000 || !IsStfsPackage(package)) return *error = "not an STFS package", false;

  uint32_t header_size = be32(&d[0x340]);
  const uint8_t* vd = &d[0x379];
  uint32_t shift = (vd[2] & 1) ? 0 : 1;  // read-only packages keep one hash table per level
  uint32_t table_blocks = vd[3] | vd[4] << 8;
  uint32_t table_start = le24(vd + 5);
  uint64_t base = (header_size + 0xFFF) & ~0xFFFull;

  // Data blocks are interleaved with hash tables every 170 blocks (and 170^2, 170^3).
  auto block_offset = [&](uint64_t i) {
    uint64_t block = i;
    for (uint64_t level = 0xAA, n = 0; n < 3; ++n, level *= 0xAA) {
      block += ((i + level) / level) << shift;
      if (i < level) break;
    }
    return base + (block << 12);
  };
  auto read_run = [&](uint32_t start, uint32_t count, std::vector<uint8_t>& out) {
    for (uint32_t k = 0; k < count; ++k) {
      uint64_t off = block_offset(start + k);
      if (off + 0x1000 > d.size()) return false;
      out.insert(out.end(), d.begin() + off, d.begin() + off + 0x1000);
    }
    return true;
  };

  std::vector<uint8_t> table;
  if (!read_run(table_start, table_blocks, table)) return *error = "file table out of range", false;

  struct Entry { fs::path name; bool dir; int parent; };
  std::vector<Entry> entries;
  for (size_t o = 0; o + 0x40 <= table.size(); o += 0x40) {
    const uint8_t* e = &table[o];
    uint32_t name_len = e[0x28] & 0x3F;
    if (!name_len) break;
    std::string name(reinterpret_cast<const char*>(e), name_len);
    if (name.find_first_of("/\\:") != std::string::npos || name == "." || name == "..")
      return *error = "bad file name in package", false;
    int parent = int16_t(e[0x32] << 8 | e[0x33]);
    if (parent >= int(entries.size())) return *error = "bad directory index in package", false;
    entries.push_back({name, (e[0x28] & 0x80) != 0, parent});

    fs::path path = name;
    for (int p = parent; p != -1; p = entries[p].parent) path = entries[p].name / path;
    path = out_dir / path;
    std::error_code ec;
    if (entries.back().dir) {
      fs::create_directories(path, ec);
      continue;
    }
    fs::create_directories(path.parent_path(), ec);
    uint32_t size = be32(e + 0x34);
    uint32_t blocks = (size + 0xFFF) >> 12;
    if (!(e[0x28] & 0x40) && blocks > 1) return *error = name + " is fragmented in the package", false;
    std::vector<uint8_t> data;
    if (!read_run(le24(e + 0x2F), blocks, data)) return *error = name + " is out of range", false;
    // Write under a temporary name, so an interrupted install never leaves a truncated file.
    fs::path tmp = path;
    tmp += ".part";
    if (!std::ofstream(tmp, std::ios::binary).write(reinterpret_cast<char*>(data.data()), size))
      return *error = "could not write " + tmp.string(), false;
    fs::rename(tmp, path, ec);
    if (ec) return *error = "could not write " + path.string() + ": " + ec.message(), false;
  }
  return true;
}

}  // namespace hexic
