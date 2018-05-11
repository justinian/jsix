#pragma once
/// \file port.h
/// Definition for AHCI ports
#include <stddef.h>
#include <stdint.h>

namespace ahci {

struct cmd_list_entry;
struct cmd_table;
enum class port_cmd : uint32_t;
struct port_data;


/// A port on an AHCI HBA
class port
{
public:
	/// Constructor.
	/// \arg data  Pointer to the device's registers for this port
	/// \arg impl  Whether this port is marked as implemented in the HBA
	port(port_data *data, bool impl);

	/// Destructor
	~port();

	enum class state { unimpl, inactive, active };

	/// Get the current state of this device
	/// \returns  An enum representing the state
	state get_state() const { return m_state; }

	/// Update the state of this object from the register data
	void update();

	/// Return whether the port is currently busy
	bool busy();

	/// Start command processing from this port
	void start_commands();

	/// Stop command processing from this port
	void stop_commands();

	/// Read data from the drive.
	/// \arg sector  Starting sector to read
	/// \arg length  Number of bytes to read
	/// \returns     True if the command succeeded
	bool read(uint64_t sector, size_t length); 

private:
	/// Rebase the port command structures to a new location in system
	/// memory, to be allocated from the page manager.
	void rebase();

	/// Get a free command slot
	/// \returns  The index of the command slot, or -1 if none available
	int get_cmd_slot();

	state m_state;
	port_data *m_data;

	void *m_fis;
	cmd_list_entry *m_cmd_list;
	cmd_table *m_cmd_table;
};

} // namespace ahci
