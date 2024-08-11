.. jsix syscall interface.
.. Automatically updated from the definition files using cog!

.. [[[cog code generation
.. from os.path import join
.. from memory import Layout, unit
..
.. layout = Layout(join(definitions_path, "memory_layout.yaml"))
.. l = max([len(r.name) for r in layout.regions])
.. ]]]
.. [[[end]]] (checksum: d41d8cd98f00b204e9800998ecf8427e)

Kernel memory
=============

While jsix probably should eventually use KASLR to randomize its memory layout,
currently the layout is mostly fixed. (Kernel code locations are not consistent
but aren't explicitly randomized.)

.. [[[cog code generation
.. line_size = 128 * 1024**3  # Each line represents up to 32 GiB
.. max_lines = 32
.. totals = sum([r.size for r in layout.regions])
.. remain = unit((128 * 1024**4) - totals)
..
.. def split(val):
..    return f"0x {val >> 48:04x} {(val >> 32) & 0xffff:04x} {(val >> 16) & 0xffff:04x} {val & 0xffff:04x}"
..
.. cog.outl()
.. cog.outl(f"+-+-----------------------------+----------+---------------------------------------+")
.. cog.outl(f"| | Address                     | Size     | Use                                   |")
.. cog.outl(f"+=+=============================+==========+=======================================+")
..
.. for region in layout.regions:
..     cog.outl(f"| | ``{split(region.start)}``  | {unit(region.size):>8} | {region.desc:37} |")
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
| | ``0x ffff be00 0000 0000``  |    1 TiB | Per-page state tracking structures    |
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
| | ``0x ffff bdf8 0000 0000``  |   32 GiB | Kernel heap accounting structures     |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bdf0 0000 0000``  |   32 GiB | Kernel heap                           |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bde8 0000 0000``  |   32 GiB | Capabilities accounting structures    |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bde0 0000 0000``  |   32 GiB | Capabilities                          |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bdd0 0000 0000``  |   64 GiB | Kernel thread stacks                  |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bdc0 0000 0000``  |   64 GiB | Kernel buffers                        |
+-+-----------------------------+----------+---------------------------------------+
| | ``0x ffff bdbf 8000 0000``  |    2 GiB | Kernel logs circular buffer           |
+-+-----------------------------+----------+---------------------------------------+
| |  ...                        |          |                                       |
+-+-----------------------------+----------+---------------------------------------+
| |  ``0x ffff 0000 0000 0000`` |          | Kernel code / headers                 |
+-+-----------------------------+----------+---------------------------------------+


Un-reserved virtual memory address space in the higher half: 61 TiB

.. [[[end]]] (checksum: 8c336cc8151beba1a79c8d3b653f1109)

* :ref:`genindex`
* :ref:`search`

