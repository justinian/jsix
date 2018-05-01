#!/usr/bin/env python3
#
# parse_version.py - Create a NASM version definition file given
# version inputs. Usage:
#
#     parse_version.py <git-describe version> <git short sha>


def split_version(version_string):
    major, minor, patch_dirty = version_string.split(".")
    patch_dirty = patch_dirty.split("-")

    return major, minor, patch_dirty[0], len(patch_dirty) > 1


def make_nasm(major, minor, patch, dirty, sha):
    if dirty:
        dirty = "1"
    else:
        dirty = "0"

    lines = [
        "%define VERSION_MAJOR  {}".format(major),
        "%define VERSION_MINOR  {}".format(minor),
        "%define VERSION_PATCH  {}".format(patch),
        "%define VERSION_GITSHA 0x{}{}".format(dirty, sha),
    ]
    return "\n".join(lines)


if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: {} <desc version> <git sha>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)
    print(make_nasm(*split_version(sys.argv[1]), sys.argv[2]))
