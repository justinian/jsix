#include <assert.h>
#include <string.h>
#include <util/hash.h>

#include <zstd.h>
#include <zstd_errors.h>

#include "j6romfs.h"

namespace j6romfs {
namespace {

static constexpr unsigned buf_size = max_path + 1;

const char *
copy_path_element(const char *path, char buf[buf_size])
{
    if (*path == '/') path++;

    unsigned i = 0;
    while (*path && *path != '/' && i < max_path)
        buf[i++] = *path++;

    buf[i] = 0;
    return path;
}

} // anon namespace

fs::fs(util::const_buffer data) :
    m_data {data}
{
    const superblock *sb = reinterpret_cast<const superblock*>(data.pointer);
    m_inodes = reinterpret_cast<const inode*>(
            util::offset_pointer(data.pointer, sb->inode_offset));
    m_root = &m_inodes[sb->root_inode];
}

util::const_buffer
fs::load_simple(char const *path) const
{
    const inode *in = lookup_inode(path);
    if (!in || in->type != inode_type::file)
        return {0, 0};

    uint8_t *data = new uint8_t [in->size];
    size_t total = load_inode_data(in, util::buffer::from(data, in->size));
    return util::buffer::from(data, total);
}

size_t
fs::load_inode_data(const inode *in, util::buffer dest) const
{
    util::const_buffer src = m_data + in->offset;

    assert(dest.count >= in->size && "Dest buffer not big enough");
    assert(src.count >= in->compressed && "Source buffer not big enough");

    if (in->size == in->compressed) {
        // If the sizes are equal, no compression happened
        memcpy(dest.pointer, src.pointer, in->size);
        return in->size;
    }

    size_t decom = ZSTD_decompress(dest.pointer, dest.count,
            src.pointer, in->compressed);
    if (ZSTD_isError(decom)) {
        ZSTD_ErrorCode err = ZSTD_getErrorCode(decom);
        const char *name = ZSTD_getErrorString(err);
        assert(!name);
    }

    return decom;
}

const inode *
fs::lookup_inode(const char *path) const
{
    if (!path)
        return nullptr;

    char element [buf_size];
    inode const *in = m_root;

    while (*path) {
        path = copy_path_element(path, element);
        in = lookup_inode_in_dir(in, element);

        if (!in) {
            // entry was not found
            break;
        } else if (!*path) {
            // found it
            return in;
        } else if (*path && in->type == inode_type::file) {
            // a directory was expected
            break;
        }
    }

    return nullptr;
}

const inode *
fs::lookup_inode_in_dir(const inode *dir, const char *name) const
{
    uint8_t *dirdata = new uint8_t [dir->size];
    load_inode_data(dir, util::buffer::from(dirdata, dir->size));

    const dirent *entries = reinterpret_cast<const dirent*>(dirdata);

    inode const *found = nullptr;
    uint64_t hash = util::fnv1a::hash64_string(name);

    unsigned max = dir->size / sizeof(dirent);
    for (unsigned i = 0; i < max; ++i) {
        const dirent &e = entries[i];
        if (e.name_hash == hash) {
            found = &m_inodes[e.inode];
            break;
        }

        unsigned new_max = e.name_offset / sizeof(dirent);
        if (new_max < max)
            max = new_max;
    }

    delete [] dirdata;
    return found;
}

} // namespace j6romfs
