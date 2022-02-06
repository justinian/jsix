#pragma once

#ifdef __cplusplus
#define CPP_CHECK_BEGIN extern "C" {
#define CPP_CHECK_END   }
#else
#define CPP_CHECK_BEGIN
#define CPP_CHECK_END
#endif
