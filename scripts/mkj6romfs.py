#!/usr/bin/env python3
#
# Generate a j6romfs image from a given directory.

from struct import calcsize, pack
from fnv import hash64

inode_type_dir = 1
inode_type_file = 2

uncompressed_limit = 0xffffffff
compressed_limit = 0xffffff

hasher = hash64

class fs_exception(Exception): pass

def compress_zstd(data):
    from pyzstd import compress
    return compress(data, 19)

def compress_none(data):
    return data

compressors = {
    'none': (0, compress_none),
    'zstd': (1, compress_zstd),
}

def align(stream, size):
    offset = stream.tell()
    extra = offset % size
    if extra != 0:
        stream.seek(size-extra, 1)
        offset = stream.tell()
    return offset

def add_file(path, inode_data, output, compress):
    offset = output.tell()
    comp_size = 0
    uncomp_size = 0

    with open(path, 'rb') as file:
        uncompressed = file.read()
        uncomp_size = len(uncompressed)
        compressed = compress(uncompressed)
        comp_size = len(compressed)

        # Don't use more room for compression than
        # the original file
        if comp_size >= uncomp_size:
            compressed = uncompressed
            comp_size = uncomp_size

        output.write(compressed)

        if uncomp_size > uncompressed_limit:
            raise fs_exception(f"File {path} too large: {uncomp_size} bytes.")

        if comp_size > compressed_limit:
            raise fs_exception(f"File {path} too large when compressed: {comp_size} bytes.")

    inode_data.append((inode_type_file, offset, comp_size, uncomp_size))


dirent_format = "<IHBBQ"
dirent_size = calcsize(dirent_format)

def make_dirent(inode, inode_type, name, strings, strings_offset):
    strings_offset += len(strings)
    name_utf = name.encode('utf-8')
    name_hash = hasher(name_utf)
    strings.extend(name_utf)
    strings.append(0)

    return pack(dirent_format, inode, strings_offset, inode_type, len(name_utf) + 1, name_hash)


def add_dir(path, files, dirs, inode_data, dir_inodes, output, compress):
    strings = bytearray()
    uncompressed = bytearray()
    offset = align(output, 0x10)
    comp_size = 0
    uncomp_size = 0

    strings_offset = (len(dirs) + len(files)) * dirent_size

    for name, realpath in dirs:
        inode = dir_inodes[realpath]
        uncompressed.extend(make_dirent(inode, inode_type_dir, name, strings, strings_offset))

    for name, inode in files:
        uncompressed.extend(make_dirent(inode, inode_type_file, name, strings, strings_offset))

    uncompressed.extend(strings)

    uncomp_size = len(uncompressed)
    compressed = compress(uncompressed)
    comp_size = len(compressed)

    # Don't use more room for compression than
    # the original file
    if comp_size >= uncomp_size:
        compressed = uncompressed
        comp_size = uncomp_size

    output.write(uncompressed)
    inode_data.append((inode_type_dir, offset, comp_size, uncomp_size))


def make_image(root, image, compressor):
    import os
    from os.path import dirname, join

    compressor_id, compress = compressor

    directories = []
    inode_data = []

    with open(image, 'wb') as output:
        def write_header(inode_offset, inode_count, root_inode):
            output.seek(0, 0)
            output.write(pack("<8s Q II B7x",
                b"j6romfs1", inode_offset, inode_count, root_inode, compressor_id))

        write_header(0, 0, 0)

        for (dirpath, dirs, files) in os.walk(root, topdown=False):
            #print(f"{dirpath}:\n\t{dirs}\n\t{files}")

            dir_inodes = []
            for file in files:
                dir_inodes.append((file, len(inode_data)))
                add_file(join(dirpath, file), inode_data, output, compress)

            parent = dirpath
            if dirpath != root:
                parent = dirname(dirpath)

            dir_directories = [('.', dirpath), ('..', parent)]
            dir_directories += [(d, join(dirpath, d)) for d in dirs]

            directories.append((dirpath, dir_inodes, dir_directories))

        dir_inodes = {directories[i][0]: len(inode_data) + i for i in range(len(directories))}
        for d in directories:
            add_dir(*d, inode_data, dir_inodes, output, compress)

        inode_offset = align(output, 0x10) # align to a 16 byte value

        for inode_type, offset, comp_size, uncomp_size in inode_data:
            comp_size_type = (comp_size & 0xffffff) | (inode_type << 24)
            output.write(pack("<IIQ", uncomp_size, comp_size_type, offset))

        write_header(inode_offset, len(inode_data), len(inode_data) - 1)


if __name__ == "__main__":
    import sys
    from argparse import ArgumentParser

    p = ArgumentParser(description="Generate a j6romfs image from a directory")

    p.add_argument("--compressor", "-c", metavar="NAME", default="zstd",
            help="Which compressor to use (currently: 'none' or 'zstd')")

    p.add_argument("--verbose", "-v", action="store_true",
            help="Output more information on progress")

    p.add_argument("input", metavar="DIR",
            help="The source directory to use as the root of the image")

    p.add_argument("output", metavar="FILE",
            help="The output file for the image")

    args = p.parse_args()

    import logging
    log_level = logging.ERROR
    if args.verbose:
        log_level = logging.INFO

    logging.basicConfig(
        format="%(levelname)s: %(message)s",
        level=log_level,
    )

    compressor = compressors.get(args.compressor)
    if compressor is None:
        logging.error(f"No such compressor: {args.compressor}")
        sys.exit(1)

    try:
        make_image(args.input, args.output, compressor)
    except fs_exception as fse:
        print(fse, file=sys.stderr)
        sys.exit(1)
