#include <algorithm>
#include "kutil/assert.h"
#include "kutil/enum_bitfields.h"
#include "ahci/ata.h"
#include "ahci/hba.h"
#include "ahci/fis.h"
#include "ahci/port.h"
#include "console.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

namespace ahci {
	enum class cmd_list_flags : uint16_t;
}

IS_BITFIELD(ahci::port_cmd);
IS_BITFIELD(ahci::cmd_list_flags);

namespace ahci {

const unsigned max_prd_count = 16;


enum class cmd_list_flags : uint16_t
{
	atapi		= 0x0020,
	write		= 0x0040,
	prefetch	= 0x0080,
	reset		= 0x0100,
	bist		= 0x0200,
	clear_busy	= 0x0400
};

inline cmd_list_flags
cmd_list_fis_size(uint8_t size)
{
	return static_cast<cmd_list_flags>((size/4) & 0x1f);
}


struct cmd_list_entry
{
	cmd_list_flags flags;
	uint16_t prd_table_length;
	uint32_t prd_byte_count;
	uint32_t cmd_table_base_low;
	uint32_t cmd_table_base_high;
	uint32_t reserved[4];

} __attribute__ ((packed));


struct prdt_entry
{
	uint32_t data_base_low;
	uint32_t data_base_high;
	uint32_t reserved;
	uint32_t byte_count;
} __attribute__ ((packed));


struct cmd_table
{
	uint8_t cmd_fis[64];
	uint8_t atapi_cmd[16];
	uint8_t reserved[48];
	prdt_entry entries[0];
} __attribute__ ((packed));


enum class port_cmd : uint32_t
{
	start			= 0x00000001,
	spinup			= 0x00000002,
	poweron			= 0x00000004,
	clo				= 0x00000008,
	fis_recv		= 0x00000010,
	fisr_running	= 0x00004000,
	cmds_running	= 0x00008000,

	none			= 0x00000000
};


struct port_data
{
	uint32_t cmd_base_low;
	uint32_t cmd_base_high;
	uint32_t fis_base_low;
	uint32_t fis_base_high;

	uint32_t interrupt_status;
	uint32_t interrupt_enable;

	port_cmd command;

	uint32_t reserved0;

	uint8_t task_file_status;
	uint8_t task_file_error;
	uint16_t reserved1;

	sata_signature signature;

	uint32_t serial_status;
	uint32_t serial_control;
	uint32_t serial_error;
	uint32_t serial_active;
	uint32_t cmd_issue;
	uint32_t serial_notify;
	uint32_t fis_switching;
	uint32_t dev_sleep;

	uint8_t reserved2[40];
	uint8_t vendor[16];
} __attribute__ ((packed));


port::port(hba *device, uint8_t index, port_data *data, bool impl) :
	m_index(index),
	m_type(sata_signature::none),
	m_state(state::unimpl),
	m_hba(device),
	m_data(data),
	m_fis(nullptr),
	m_cmd_list(nullptr),
	m_cmd_table(nullptr)
{
	if (impl) {
		m_state = state::inactive;
		update();
	}
}

port::~port()
{
	if (m_cmd_list) {
		page_manager *pm = page_manager::get();
		pm->unmap_pages(m_cmd_list, 3);
	}
}

void
port::update()
{
	if (m_state == state::unimpl) return;

	uint32_t detected = m_data->serial_status & 0x0f;
	uint32_t power = (m_data->serial_status >> 8) & 0x0f;

	if (detected == 0x3 && power == 0x1) {
		m_state = state::active;
		m_type = m_data->signature;

		const char *name =
			m_type == sata_signature::sata_drive ? "SATA" :
			m_type == sata_signature::satapi_drive ? "SATAPI" :
			"Other";

		log::info(logs::driver, "Found device type %s at port %d", name, m_index);

		rebase();
		m_pending.set_size(32);
		for (auto &pend : m_pending) {
			pend.type = command_type::none;
		}

		// Clear any old pending interrupts and enable interrupts
		m_data->interrupt_status = m_data->interrupt_status;
		m_data->interrupt_enable = 0xffffffff;
	} else {
		m_state = state::inactive;
	}
}

bool
port::busy()
{
	return (m_data->task_file_status & 0x88) != 0;
}

void
port::start_commands()
{
	while (bitfield_has(m_data->command, port_cmd::cmds_running))
		io_wait();

	m_data->command |= port_cmd::fis_recv;
	m_data->command |= port_cmd::start;
}

void
port::stop_commands()
{
	m_data->command &= ~port_cmd::start;

	while (
		bitfield_has(m_data->command, port_cmd::cmds_running) ||
		bitfield_has(m_data->command, port_cmd::fisr_running))
		io_wait();

	m_data->command &= ~port_cmd::fis_recv;
}

int
port::make_command(size_t length, fis_register_h2d **fis)
{
	int slot = -1;
	uint32_t used_slots =
		m_data->serial_active |
		m_data->cmd_issue |
		m_data->interrupt_status;

	for (int i = 0; i < 32; ++i) {
		if (used_slots & (1 << i)) continue;

		if (m_pending[i].type == command_type::none) {
			slot = i;
			break;
		} else {
			log::debug(logs::driver, "Type is %d", m_pending[i].type);
		}
	}

	if (slot < 0) {
		log::error(logs::driver, "AHCI could not get a free command slot.");
		return -1;
	}

	page_manager *pm = page_manager::get();

	cmd_list_entry &ent = m_cmd_list[slot];
	cmd_table &cmdt = m_cmd_table[slot];

	kutil::memset(&cmdt, 0, sizeof(cmd_table) +
			max_prd_count * sizeof(prdt_entry));

	ent.flags = cmd_list_fis_size(sizeof(fis_register_h2d));

	fis_register_h2d *cfis = reinterpret_cast<fis_register_h2d *>(&cmdt.cmd_fis);
	kutil::memset(cfis, 0, sizeof(fis_register_h2d));
	cfis->type = fis_type::register_h2d;
	cfis->pm_port = 0x80; // set command register flag
	*fis = cfis;

	size_t remaining = length;
	for (int i = 0; i < max_prd_count; ++i) {
		size_t prd_len = std::min(remaining, 0x200000ul);
		remaining -= prd_len;

		void *mem = pm->map_offset_pages(page_count(prd_len));
		kutil::memset(mem, 0xaf, prd_len);

		addr_t phys = pm->offset_phys(mem);
		cmdt.entries[i].data_base_low = phys & 0xffffffff;
		cmdt.entries[i].data_base_high = phys >> 32;
		cmdt.entries[i].byte_count = prd_len - 1;
		if (remaining == 0 || i == max_prd_count - 1) {
			// If this is the last one, set the interrupt flag
			cmdt.entries[i].byte_count |= 0x80000000;
			ent.prd_table_length = i + 1;
			break;
		}
	}


	log::debug(logs::driver, "Created command, slot %d, %d PRD entries.",
			slot, ent.prd_table_length);
	return slot;
}

int
port::read_async(uint64_t offset, size_t length, void *dest)
{
	fis_register_h2d *fis;
	int slot = make_command(length, &fis);
	if (slot < 0)
		return 0;

	cmd_table &cmdt = m_cmd_table[slot];

	fis->command = ata_cmd::read_dma_ext;
	fis->device = 0x40; // ATA8-ACS p.175

	uint64_t sector = offset >> 9;
	fis->lba0 = (sector      ) & 0xff;
	fis->lba1 = (sector >>  8) & 0xff;
	fis->lba2 = (sector >> 16) & 0xff;
	fis->lba3 = (sector >> 24) & 0xff;
	fis->lba4 = (sector >> 32) & 0xff;
	fis->lba5 = (sector >> 40) & 0xff;

	size_t count = ((length - 1) >> 9) + 1; // count is in sectors
	fis->count0 = (count     ) & 0xff;
	fis->count1 = (count >> 8) & 0xff;

	log::debug(logs::driver, "Reading %d sectors, starting from %d (0x%lx)",
			count, sector, sector*512);

	m_pending[slot].type = command_type::read;
	m_pending[slot].offset = offset % 512;
	m_pending[slot].count = 0;
	m_pending[slot].data = dest;
	if(issue_command(slot))
		return slot;
	else
		return -1;
}

size_t
port::read(uint64_t offset, size_t length, void *dest)
{
	dump();
	int slot = read_async(offset, length, dest);
	dump();

	int timeout = 0;
	while (m_pending[slot].type == command_type::read) {
		if (timeout++ > 5) {
			m_hba->dump();
			dump();
			return 0;
		}
		asm("hlt");
	}
	kassert(m_pending[slot].type == command_type::finished,
			"Read got unexpected command type");

	size_t count = m_pending[slot].count;
	m_pending[slot].type = command_type::none;
	m_pending[slot].count = 0;

	return count;
}

int
port::identify_async()
{
	fis_register_h2d *fis;
	int slot = make_command(512, &fis);
	if (slot < 0)
		return 0;

	fis->command = ata_cmd::identify;

	m_pending[slot].type = command_type::identify;
	m_pending[slot].offset = 0;
	m_pending[slot].count = 0;
	m_pending[slot].data = 0;

	if(issue_command(slot))
		return slot;
	else
		return -1;
}

bool
port::issue_command(int slot)
{
	const int max_tries = 10;
	int tries = 0;
	while (busy()) {
		if (++tries == max_tries) {
			log::warn(logs::driver, "AHCI port was busy too long");
			free_command(slot);
			return false;
		}
		io_wait();
	}

	// Set bit in CI. Note that only new bits should be written, not
	// previous state.
	m_data->cmd_issue = (1 << slot);
	return true;
}

void
port::handle_interrupt()
{
	log::debug(logs::driver, "AHCI port %d got an interrupt");

	// TODO: handle other states in interrupt_status

	if (m_data->interrupt_status & 0x40000000) {
		log::error(logs::driver, "AHCI task file error");
		dump();
		kassert(0, "Task file error");
	}

	log::debug(logs::driver, "AHCI interrupt status: %08lx  %08lx",
			m_data->interrupt_status, m_data->serial_error);

	uint32_t ci = m_data->cmd_issue;
	for (int i = 0; i < 32; ++i) {
		if (ci & (1 << i)) continue;

		pending &p = m_pending[i];
		switch (p.type) {
		case command_type::read:
			finish_read(i);
			break;
		case command_type::identify:
			finish_identify(i);
			break;
		default:
			break;
		}
	}
	m_data->interrupt_status = m_data->interrupt_status;
}

void
port::finish_read(int slot)
{
	page_manager *pm = page_manager::get();
	cmd_table &cmdt = m_cmd_table[slot];
	cmd_list_entry &ent = m_cmd_list[slot];

	size_t count = 0;
	void *p = m_pending[slot].data;
	uint8_t offset = m_pending[slot].offset;
	for (int i = 0; i < ent.prd_table_length; ++i) {
		size_t prd_len = (cmdt.entries[i].byte_count & 0x7fffffff) + 1;

		addr_t phys = 
			static_cast<addr_t>(cmdt.entries[i].data_base_low) |
			static_cast<addr_t>(cmdt.entries[i].data_base_high) << 32;

		void *mem = kutil::offset_pointer(pm->offset_virt(phys), offset);

		log::debug(logs::driver, "Reading PRD %2d: %016lx->%016lx [%lxb]", i, mem, p, prd_len);

		kutil::memcpy(p, mem, prd_len);
		p = kutil::offset_pointer(p, prd_len - offset);
		count += (prd_len - offset);
		offset = 0;

		pm->unmap_pages(mem, page_count(prd_len));
	}

	m_pending[slot].count = count;
	m_pending[slot].type = command_type::finished;
	m_pending[slot].data = nullptr;
}

static void
ident_strcpy(uint16_t *from, int words, char *dest)
{
	for (int i = 0; i < words; ++i) {
		*dest++ = *from >> 8;
		*dest++ = *from & 0xff;
		from++;
	}
	*dest = 0;
}

void
port::finish_identify(int slot)
{
	page_manager *pm = page_manager::get();
	cmd_table &cmdt = m_cmd_table[slot];
	cmd_list_entry &ent = m_cmd_list[slot];

	kassert(ent.prd_table_length == 1, "AHCI identify used multiple PRDs");

	size_t prd_len = (cmdt.entries[0].byte_count & 0x7fffffff) + 1;

	addr_t phys =
		static_cast<addr_t>(cmdt.entries[0].data_base_low) |
		static_cast<addr_t>(cmdt.entries[0].data_base_high) << 32;

	log::debug(logs::driver, "Reading ident PRD:");

	uint16_t *mem = reinterpret_cast<uint16_t *>(pm->offset_virt(phys));
	char string[41];

	ident_strcpy(&mem[10], 10, &string[0]);
	log::debug(logs::driver, "    Device serial: %s", string);

	ident_strcpy(&mem[23], 4, &string[0]);
	log::debug(logs::driver, "   Device version: %s", string);

	ident_strcpy(&mem[27], 20, &string[0]);
	log::debug(logs::driver, "     Device model: %s", string);

	uint32_t sectors = mem[60] | (mem[61] << 16);
	log::debug(logs::driver, "      Max sectors: %xh", sectors);

	uint16_t lb_size = mem[106];
	log::debug(logs::driver, " lsects per psect: %d %s %s", 1 << (lb_size & 0xf),
			lb_size & 0x20 ? "multiple logical per physical" : "",
			lb_size & 0x10 ? "physical > 512b" : "");

	uint32_t b_per_ls = 2 * (mem[117] | (mem[118] << 16));
	log::debug(logs::driver, "      b per lsect: %d", b_per_ls);

	/*
	for (int i=0; i<256; i += 4)
		log::debug(logs::driver, "  %3d: %04x  %3d: %04x  %3d: %04x  %3d: %04x",
				i, mem[i], i+1, mem[i+1], i+2, mem[i+2], i+3, mem[i+3]);
	*/

	pm->unmap_pages(mem, page_count(prd_len));
	m_pending[slot].type = command_type::none;
}

void
port::free_command(int slot)
{
	page_manager *pm = page_manager::get();

	cmd_table &cmdt = m_cmd_table[slot];
	cmd_list_entry &ent = m_cmd_list[slot];

	for (int i = 0; i < ent.prd_table_length; ++i) {
		size_t prd_len = (cmdt.entries[i].byte_count & 0x7fffffff) + 1;

		addr_t phys =
			static_cast<addr_t>(cmdt.entries[i].data_base_low) |
			static_cast<addr_t>(cmdt.entries[i].data_base_high) << 32;
		void *mem = pm->offset_virt(phys);
		pm->unmap_pages(mem, page_count(prd_len));
	}

	ent.prd_table_length = max_prd_count;
}

void
port::rebase()
{
	kassert(!m_cmd_list, "AHCI port called rebase() twice");

	page_manager *pm = page_manager::get();
	size_t prd_size = sizeof(cmd_table) + (max_prd_count * sizeof(prdt_entry));

	// 1 for FIS + command list, N for PRD
	size_t pages = 1 + page_count(prd_size * 32);

	void *mem = pm->map_offset_pages(pages);
	addr_t phys = pm->offset_phys(mem);

	log::debug(logs::driver, "Rebasing address for AHCI port %d to %lx [%d]", m_index, mem, pages);

	stop_commands();

	// Command list
	m_cmd_list = reinterpret_cast<cmd_list_entry *>(mem);
	m_data->cmd_base_low = phys & 0xffffffff;
	m_data->cmd_base_high = phys >> 32;
	kutil::memset(mem, 0, 1024);

	mem = kutil::offset_pointer(mem, 32 * sizeof(cmd_list_entry));
	phys = pm->offset_phys(mem);

	// FIS
	m_fis = mem;
	m_data->fis_base_low = phys & 0xffffffff;
	m_data->fis_base_high = phys >> 32;
	kutil::memset(mem, 0, 256);

	mem = page_align(kutil::offset_pointer(mem, 256));
	phys = pm->offset_phys(mem);

	// Command table
	m_cmd_table = reinterpret_cast<cmd_table *>(mem);
	size_t cmdt_len = sizeof(cmd_table) +
			max_prd_count * sizeof(prdt_entry);

	kutil::memset(m_cmd_table, 0, cmdt_len * 32);

	// set up each entry in the command list to point to the
	// corresponding command table
	for (int i = 0; i < 32; ++i) {
		m_cmd_list[i].prd_table_length = max_prd_count;
		m_cmd_list[i].cmd_table_base_low = phys & 0xffffffff;
		m_cmd_list[i].cmd_table_base_high = phys >> 32;

		mem = kutil::offset_pointer(mem, cmdt_len);
		phys = pm->offset_phys(mem);
	}

	start_commands();
}

void
port::dump()
{
	console *cons = console::get();
	static const char *regs[] = {
		" CLB", "+CLB", "  FB", " +FB", "  IS", "  IE",
		" CMD", nullptr, " TFD", " SIG", "SSTS", "SCTL", "SERR",
		"SACT", "  CI", "SNTF", " FBS", "DEVS"
	};

	cons->printf("Port Registers:\n");
	uint32_t *data = reinterpret_cast<uint32_t *>(m_data);
	for (int i = 0; i < 18; ++i) {
		if (regs[i]) cons->printf("  %s: %08x\n", regs[i], data[i]);
	}
	cons->putc('\n');
}

} // namespace ahci
