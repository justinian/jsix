#pragma once
/// \file loader.h
/// Data structure for a ramdisk archive, based on djb's CDB format

#include <unordered_map>
#include <vector>

#include <j6/types.h>
#include <util/counted.h>

class ramdisk
{
public:
    ramdisk(util::buffer data);

    util::buffer load_file(const char *name);

private:
    util::buffer m_data;
};

class manifest
{
public:
    manifest(util::buffer data);

private:
    std::vector<const char*> m_services;
    std::unordered_map<const char*, const char*> m_drivers;
};
