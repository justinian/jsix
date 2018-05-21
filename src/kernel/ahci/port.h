#pragma once
/// \file port.h
/// Definition for AHCI ports
#include <stddef.h>
#include <stdint.h>
#include "kutil/vector.h"
#include "block_device.h"

namespace ahci {

struct cmd_list_entry;
struct cmd_table;
struct fis_register_h2d;
class hba;
enum class sata_signature : uint32_t;
enum class port_cmd : uint32_t;
struct port_data;


/// A port on an AHCI HBA
class port :
	public block_device
{
public:
	/// Constructor.
	/// \arg device The HBA device this port belongs to
	/// \arg index  Index of the port on its HBA
	/// \arg data   Pointer to the device's registers for this port
	/// \arg impl   Whether this port is marked as implemented in the HBA
	port(hba *device, uint8_t index, port_data *data, bool impl);

	/// Destructor
	~port();

	/// Get the index of this port on the HBA
	/// \returns  The port index
	inline uint8_t index() const { return m_index; }

	enum class state : uint8_t { unimpl, inactive, active };

	/// Get the current state of this device
	/// \returns  An enum representing the state
	inline state get_state() const { return m_state; }

	/// Check if this device is active
	/// \returns  True if the device state is active
	inline bool active() const { return m_state == state::active; }

	/// Get the type signature of this device
	/// \returns  An enum representing the type of device
	inline sata_signature get_type() const { return m_type; }

	/// Update the state of this object from the register data
	void update();

	/// Return whether the port is currently busy
	bool busy();

	/// Start command processing from this port
	void start_commands();

	/// Stop command processing from this port
	void stop_commands();

	/// Start a read operation from the drive.
	/// \arg offset  Offset to start from
	/// \arg length  Number of bytes to read
	/// \arg dest    A buffer where the data will be placed
	/// \returns     A handle to the read operation, or -1 on error
	int read_async(uint64_t offset, size_t length, void *dest);

	/// Read from the drive, blocking until finished.
	/// \arg offset  Offset to start from
	/// \arg length  Number of bytes to read
	/// \arg dest    A buffer where the data will be placed
	/// \returns     The number of bytes read
	virtual size_t read(uint64_t offset, size_t length, void *dest);

	/// Start an identify operation for the drive.
	/// \returns     A handle to the read operation, or -1 on error
	int identify_async();

	/// Handle an incoming interrupt
	void handle_interrupt();

	/// Dump the port registers to the console
	void dump();

private:
	/// Rebase the port command structures to a new location in system
	/// memory, to be allocated from the page manager.
	void rebase();

	/// Initialize a command structure
	/// \arg length  The number of bytes of data needed in the PRDs
	/// \arg fis     [out] The FIS for this command
	/// \returns     The index of the command slot, or -1 if none available
	int make_command(size_t length, fis_register_h2d **fis);

	/// Send a constructed command to the hardware
	/// \arg slot   The index of the command slot used
	/// \returns    True if the command was successfully sent
	bool issue_command(int slot);

	/// Free the data structures allocated by command creation
	/// \arg slot   The index of the command slot used
	void free_command(int slot);

	/// Finish a read command started by `read()`. This will
	/// free the structures allocated, so `free_command()` is
	/// not necessary.
	/// \arg slot  The command slot that the read command used
	void finish_read(int slot);

	/// Finish an identify command started by `identify_async()`.
	/// This will free the structures allocated, so `free_command()` is
	/// not necessary.
	/// \arg slot  The command slot that the read command used
	void finish_identify(int slot);

	sata_signature m_type;
	uint8_t m_index;
	state m_state;

	hba *m_hba;
	port_data *m_data;
	void *m_fis;
	cmd_list_entry *m_cmd_list;
	cmd_table *m_cmd_table;

	enum class command_type : uint8_t { none, read, write, identify, finished };

	struct pending
	{
		command_type type;
		uint8_t offset;
		size_t count;
		void *data;
	};

	kutil::vector<pending> m_pending;
};

} // namespace ahci
