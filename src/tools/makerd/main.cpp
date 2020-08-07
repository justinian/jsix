#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>
#include "cpptoml.h"
#include "initrd/headers.h"
#include "entry.h"

int main(int argc, char **argv)
{
	if (argc < 3 || argc > 4) {
		std::cerr << "Usage: " << argv[0] << " <manifest> <output> [sourceroot]" << std::endl;
		return 1;
	}

	if (argc == 4 && chdir(argv[3]) != 0) {
		std::cerr
			<< "Failed to change to " << argv[3] << ": "
			<< std::strerror(errno) << std::endl;
		return 1;
	}

	std::vector<entry> entries;
	auto manifest = cpptoml::parse_file(argv[1]);

	size_t files_size = 0;
	size_t names_size = 0;

	auto files = manifest->get_table_array("files");
	for (const auto &file : *files) {
		auto dest = file->get_as<std::string>("dest");
		if (!dest) {
			std::cerr << "File has no destination!" << std::endl;
			return 1;
		}

		auto source = file->get_as<std::string>("source");
		if (!source) {
			std::cerr << "File " << *dest << " has no source!" << std::endl;
			return 1;
		}

		auto exec = file->get_as<bool>("executable").value_or(false);

		entries.emplace_back(*source, *dest, exec);
		const entry &e = entries.back();

		if (!e.good()) {
			std::cerr << "Error reading file: " << *source << std::endl;
			return 1;
		}

		files_size += e.size();
		names_size += dest->length() + 1;
	}

	std::fstream out(argv[2],
		std::ios_base::binary |
		std::ios_base::trunc |
		std::ios_base::out |
		std::ios_base::in);

	if (!out.good()) {
		std::cerr << "Error opening file: " << argv[2] << std::endl;
		return 1;
	}

	uint32_t total_length =
		sizeof(initrd::disk_header) +
		sizeof(initrd::file_header) * entries.size() +
		names_size + files_size;

	initrd::disk_header dheader;
	std::memset(&dheader, 0, sizeof(dheader));

	dheader.file_count = entries.size();
	dheader.length = total_length;
	out.write(reinterpret_cast<const char *>(&dheader), sizeof(dheader));

	size_t name_offset =
		sizeof(initrd::disk_header) +
		sizeof(initrd::file_header) * entries.size();

	size_t file_offset = name_offset + names_size;

	for (auto &e : entries) {
		initrd::file_header fheader;
		std::memset(&fheader, 0, sizeof(fheader));
		fheader.offset = file_offset;
		fheader.length = e.size();
		fheader.name_offset = name_offset;

		if (e.executable())
			fheader.flags |= initrd::file_flags::executable;

		out.write(
			reinterpret_cast<const char *>(&fheader),
			sizeof(fheader));

		name_offset += e.out().length() + 1;
		file_offset += e.size();
	}

	for (auto &e : entries) {
		out.write(e.out().c_str(), e.out().length() + 1);
	}

	for (auto &e : entries) {
		out << e.file().rdbuf();
	}

	char buffer[1024];
	uint8_t checksum = 0;

	out.seekg(0);
	while (true) {
		size_t n = out.readsome(&buffer[0], 1024);
		if (n == 0 || out.eof()) break;

		for (size_t i=0; i<n; ++i)
			checksum += static_cast<uint8_t>(buffer[i]);
	}

	dheader.checksum = static_cast<uint8_t>(0 - checksum);
	out.seekp(0);
	out.write(reinterpret_cast<const char *>(&dheader), sizeof(dheader));

	out.close();
	return 0;
}
