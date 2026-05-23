.. jsix syscall interface.
.. Automatically updated from the definition files using cog!

.. [[[cog code generation
.. from pathlib import Path
.. from jsix.memory import layout, to_units
..
.. regions = layout(Path(definitions_path) / "memory_layout.toml")
.. l = max([len(r.name) for r in regions])
.. ]]]
.. [[[end]]] (sum: 1B2M2Y8Asg)

Kernel memory
=============

While jsix probably should eventually use KASLR to randomize its memory layout,
currently the layout is mostly fixed. (Kernel code locations are not consistent
but aren't explicitly randomized.)

.. [[[cog code generation
.. line_size = 128 * 1024**3  # Each line represents up to 32 GiB
.. max_lines = 32
.. totals = sum([r.size for r in regions])
.. remain = to_units((128 * 1024**4) - totals)
..
.. def split(val):
..    return f"0x {val >> 48:04x} {(val >> 32) & 0xffff:04x} {(val >> 16) & 0xffff:04x} {val & 0xffff:04x}"
..
.. cog.outl()
.. cog.outl(f"+-+-----------------------------+----------+---------------------------------------+")
.. cog.outl(f"| | Address                     | Size     | Use                                   |")
.. cog.outl(f"+=+=============================+==========+=======================================+")
..
.. for region in regions:
..     cog.outl(f"| | ``{split(region.start)}``  | {to_units(region.size):>8} | {region.desc:37} |")
..     lines = min(max_lines, region.size // line_size)
..     for i in range(1, lines):
..         cog.outl(f"+-+                             |          |                                       |")
..         cog.outl(f"| |                             |          |                                       |")
..     cog.outl(f"+-+-----------------------------+----------+---------------------------------------+")
..
.. cog.outl(f"| |  ...                        |          |                                       |")
.. cog.outl(f"+-+-----------------------------+----------+---------------------------------------+")
.. cog.outl(f"| |  ``0x ffff 0000 0000 0000`` |          | Kernel code / headers                 |")
.. cog.outl(f"+-+-----------------------------+----------+---------------------------------------+")
.. cog.outl("")
.. cog.outl("")
.. cog.outl(f"Un-reserved virtual memory address space in the higher half: {remain}")
.. cog.outl("")
..
.. ]]]

+-+-----------------------------+----------+---------------------------------------+
| | Address                     | Size     | Use                                   |
+=+=============================+==========+=======================================+
| | ``0x ffff c000 0000 0000``  |   64 TiB | Linearly-mapped physical memory       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bf00 0000 0000``  |    1 TiB | Used/free page tracking bitmap        |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+                             |          |                                       |
| |                             |          |                                       |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bef8 0000 0000``  |   32 GiB | Kernel heap accounting structures     |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bef0 0000 0000``  |   32 GiB | Kernel heap                           |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bee8 0000 0000``  |   32 GiB | Capabilities accounting structures    |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bee0 0000 0000``  |   32 GiB | Capabilities                          |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bed0 0000 0000``  |   64 GiB | Kernel thread stacks                  |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bec0 0000 0000``  |   64 GiB | Kernel buffers                        |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bebf 8000 0000``  |    2 GiB | Kernel logs circular buffer           |
+-+-----------------------------+----------+---------------------------------------+
| |  ...                        |          |                                       |
+-+-----------------------------+----------+---------------------------------------+
| |  ``0x ffff 0000 0000 0000`` |          | Kernel code / headers                 |
+-+-----------------------------+----------+---------------------------------------+


Un-reserved virtual memory address space in the higher half: 62.7 TiB

.. [[[end]]] (sum: bCC5/j6vqL)

* :ref:`genindex`
* :ref:`search`

