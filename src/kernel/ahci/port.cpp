#include <algorithm>
#include "kutil/assert.h"
#include "kutil/enum_bitfields.h"
#include "ahci/ata.h"
#include "ahci/fis.h"
#include "ahci/port.h"
#include "io.h"
#include "log.h"
#include "page_manager.h"

IS_BITFIELD(ahci::port_cmd);

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

	uint32_t signature;
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


port::port(port_data *data, bool impl) :
	m_state(state::unimpl),
	m_data(data),
	m_fis(nullptr),
	m_cmd_list(nullptr),
	m_cmd_table(nullptr)
{
	if (impl) {
		m_state = state::inactive;
		update();
		if (m_state == state::active)
			rebase();
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

	if (detected == 0x3 && power == 0x1)
		m_state = state::active;
	else
		m_state = state::inactive;
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

bool
port::read(uint64_t sector, size_t length)
{
	m_data->interrupt_status = ~0u;

	int slot = get_cmd_slot();
	if (slot < 0) {
		log::info(logs::driver, "AHCI could not get a free command slot.");
		return false;
	}

	page_manager *pm = page_manager::get();

	cmd_list_entry &ent = m_cmd_list[slot];
	cmd_table &cmdt = m_cmd_table[slot];

	kutil::memset(&cmdt, 0, sizeof(cmd_table) +
			max_prd_count * sizeof(prdt_entry));

	ent.flags = cmd_list_fis_size(sizeof(fis_register_h2d));

	void *buffers[32];

	size_t remaining = length;
	for (int i = 0; i < max_prd_count; ++i) {
		size_t prd_len = std::min(remaining, 0x200000ul);
		remaining -= prd_len;

		void *mem = pm->map_offset_pages(page_count(prd_len));
		buffers[i] = mem;
		addr_t phys = pm->offset_phys(mem);
		cmdt.entries[i].data_base_low = phys & 0xffffffff;
		cmdt.entries[i].data_base_high = phys >> 32;
		cmdt.entries[i].byte_count = prd_len - 1;
		if (remaining == 0 || i == max_prd_count - 1) {
			// If this is the last one, set the interrupt flag
			cmdt.entries[i].byte_count |= 0x80000000;
			ent.prd_table_length = i;
			break;
		}
	}

	fis_register_h2d *fis = reinterpret_cast<fis_register_h2d *>(&cmdt.cmd_fis);
	fis->type = fis_type::register_h2d;
	fis->pm_port = 0x80; // set command register flag
	fis->command = ata_cmd::read_dma_ext;

	fis->lba0 = (sector      ) & 0xff;
	fis->lba1 = (sector >>  8) & 0xff;
	fis->lba2 = (sector >> 16) & 0xff;
	fis->lba3 = (sector >> 24) & 0xff;
	fis->lba4 = (sector >> 32) & 0xff;
	fis->lba5 = (sector >> 40) & 0xff;

	size_t count = length >> 9; // count is in sectors
	fis->count0 = (count     ) & 0xff;
	fis->count1 = (count >> 8) & 0xff;

	const int max_tries = 10;
	int tries = 0;
	while (busy()) {
		if (++tries == max_tries) {
			log::warn(logs::driver, "AHCI port was busy too long");
			return false;
		}
		io_wait();
	}

	if (tries == max_tries) {
		// TODO: clean up!!!
		return false;
	}

	m_data->cmd_issue |= (1 << slot);

	// TODO: interrupt-based
	while (true) {
		if ((m_data->cmd_issue & (1 << slot)) == 0) break;
		if (m_data->interrupt_status & 0x40000000) {
			log::error(logs::driver, "AHCI task file error");
			// TODO: clean up!
			return false;
		}
		io_wait();
	}

	if (m_data->interrupt_status & 0x40000000) {
		log::error(logs::driver, "AHCI task file error");
		// TODO: clean up!
		return false;
	}

	log::warn(logs::driver, "AHCI read success!? '%s'", buffers[0]);

	return true;
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

	log::debug(logs::driver, "Rebasing address for AHCI port to %lx [%d]", mem, pages);

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

int
port::get_cmd_slot()
{
	uint32_t used = (m_data->serial_active | m_data->cmd_issue);
	for (int i = 0; i < 32; ++i)
		if (used & (1 << i)) return i;

	return -1;
}

} // namespace ahci
