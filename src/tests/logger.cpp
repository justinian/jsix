#include "kutil/constexpr_hash.h"
#include "kutil/logger.h"
#include "catch.hpp"

using namespace kutil;

uint8_t test_log_buffer[0x400];

const char *name1 = "test_area1";
log::area_t hash1 = "test_area1"_h;

const char *name2 = "test_area2";
log::area_t hash2 = "test_area2"_h;

TEST_CASE( "logger writing", "[logger]" )
{
    log::logger logger(test_log_buffer, sizeof(test_log_buffer));
    logger.register_area(hash1, name1, log::level::verbose);
    logger.register_area(hash2, name2, log::level::verbose);
    CHECK( hash1 != hash2 );

    const char *check1 = logger.area_name(hash1);
    const char *check2 = logger.area_name(hash2);

    CHECK( check1 == name1 );
    CHECK( check2 == name2 );

    log::verbose(hash1, "This is a thing %016lx", 35);
    log::info(hash2, "This is a string %s", "foo");
    log::warn(hash1, "This is a thing %016lx", 682);
    log::error(hash2, "This is a string %s", "bar");
}
