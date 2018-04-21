#include "console.h"
#include "memory_pages.h"

page_block *
page_block::list_consolidate()
{
	page_block *cur = this;
	page_block *freed_head = nullptr, **freed = &freed_head;

	while (cur) {
		page_block *next = cur->next;

		if (next && cur->flags == next->flags &&
			cur->end() == next->physical_address)
		{
			cur->count += next->count;
			cur->next = next->next;

			next->next = 0;
			*freed = next;
			freed = &next->next;
			continue;
		}

		cur = next;
	}

	return freed_head;
}

void
page_block::list_dump(const char *name)
{
	console *cons = console::get();
	cons->puts("Block list");
	if (name) {
		cons->puts(" ");
		cons->puts(name);
	}
	cons->puts(":\n");

	int count = 0;
	for (page_block *cur = this; cur; cur = cur->next) {
		cons->puts("  ");
		cons->put_hex(cur->physical_address);
		cons->puts(" ");
		cons->put_hex((uint32_t)cur->flags);
		if (cur->virtual_address) {
			cons->puts(" ");
			cons->put_hex(cur->virtual_address);
		}
		cons->puts(" [");
		cons->put_dec(cur->count);
		cons->puts("]\n");

		count += 1;
	}

	cons->puts("  Total: ");
	cons->put_dec(count);
	cons->puts("\n");
}

void
page_table_indices::dump()
{
	console *cons = console::get();
	cons->puts("{");
	for (int i = 0; i < 4; ++i) {
		if (i) cons->puts(", ");
		cons->put_dec(index[i]);
	}
	cons->puts("}");
}
