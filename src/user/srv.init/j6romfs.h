#pragma once
/// \file j6romfs.h
/// Data structure for dealing with j6romfs images

#include <util/counted.h>
#include <util/enum_bitfields.h>

namespace j6romfs
{

inline constexpr unsigned max_path = 0xff;

enum class compressor : uint8_t { none, zstd };

struct superblock
{
    uint64_t magic;
    uint64_t inode_offset;

    uint32_t inode_count;
    uint32_t root_inode;

    compressor compressor;

    uint8_t reserved[7];
};

enum class inode_type : uint8_t { none, directory, file, symlink, };

struct inode
{
    uint32_t size;
    uint32_t compressed : 24;
    inode_type type : 8;
    uint64_t offset;
};

struct dirent
{
    uint32_t inode;
    uint16_t name_offset;
    inode_type type;
    uint8_t name_len;
    uint64_t name_hash;
};

class fs
{
public:
    fs(util::const_buffer data);

    util::const_buffer load_simple(char const *path) const;

private:
    size_t load_inode_data(const inode *in, util::buffer dest) const;
    const inode * lookup_inode_in_dir(const inode *in, const char *name) const;

    util::const_buffer m_data;
    inode const *m_inodes;
    inode const *m_root;
};

} // namespace j6romfs
