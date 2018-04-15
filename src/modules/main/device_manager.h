#pragma once

class device_manager
{
public:
	device_manager(const void *root_table);

	device_manager() = delete;
	device_manager(const device_manager &) = delete;
};
