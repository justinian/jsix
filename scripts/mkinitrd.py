#!/usr/bin/env python3

compress_level = 19

def write_image(image, files):
    from pathlib import Path
    from cdblib import Writer, djb_hash
    from pyzstd import compress

    with open(image, 'wb') as db:
        with Writer(db) as writer:
            for f in files:
                key = Path(f).name.encode('utf-8')
                with open(f, 'rb') as input_file:
                    writer.put(key, compress(input_file.read(), compress_level))

if __name__ == "__main__":
    from argparse import ArgumentParser

    p = ArgumentParser(description="Generate a jsix initrd image")

    p.add_argument("image", metavar="INITRD", 
            help="initrd image file to generate")

    p.add_argument("files", metavar="FILE", nargs='+',
            help="files to add to the image")

    args = p.parse_args()
    write_image(args.image, args.files)
