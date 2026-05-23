#!/usr/bin/env uv -S uv run --script
# /// script
# requires-python = ">=3.14"
# dependencies = [
#     "click>=8.4.0",
#     "gpt-image>=0.9.1",
# ]
# ///
import click
from pathlib import Path

KiB = 1024
MiB = 1024**2

LEAN_GUID = "bb5a91b0-977e-11db-b606-0800200c9a66"

OutFileType = click.Path(
    dir_okay = False,
    writable = True,
    path_type = Path)
RootDirType = click.Path(
    exists = True,
    file_okay = False,
    readable = True,
    path_type = Path)

@click.command()
@click.argument('output', type=OutFileType)
@click.argument('root', type=RootDirType)
def main(output: Path, root: Path) -> None:
    """Create a disk image with the contents of ROOT."""
    from fat import FATDir, make_fat16

    fatdir = FATDir(root, name="jsix_esp")
    fatpart = make_fat16(fatdir)

    from gpt_image.disk import Disk
    from gpt_image.partition import Partition, PartitionType

    # gpt-image doesn't like to overwrite existing images
    if output.exists():
        import os
        os.unlink(output)

    disk = Disk(output) # type: ignore
    disk.create(len(fatpart) + 5*MiB)

    esp = Partition("boot", len(fatpart), PartitionType.EFI_SYSTEM_PARTITION.value)
    disk.table.partitions.add(esp)

    j6p = Partition("jsix_os", disk.geometry.sector_size, LEAN_GUID)
    disk.table.partitions.add(j6p)

    j6p.size = (disk.geometry.last_usable_lba - j6p.first_lba_staged) \
            * disk.geometry.sector_size
    disk.commit()

    esp.write_data(disk, fatpart) 
    with open(str(output) + ".fat", 'wb') as f:
        f.write(fatpart)

if __name__ == "__main__":
    main()
