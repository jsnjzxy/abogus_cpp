#pragma once

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file abogus.h
 * @brief Douyin a_bogus signature generator
 *
 * This library provides a C interface for generating a_bogus signatures
 * required for Douyin (TikTok China) API requests.
 *
 * @usage
 *   char* sig = get_abogus("Mozilla/5.0...", "aid=6383&web_rid=xxx");
 *   if (sig) {
 *       // use signature...
 *       free_abogus(sig);
 *   }
 */

/**
 * @brief Generate a_bogus signature
 *
 * Calculates the a_bogus signature for Douyin API requests.
 * The signature is computed using the provided User-Agent and query parameters.
 *
 * @param userAgent HTTP User-Agent header value (must match actual request)
 * @param params URL query parameters string (e.g., "aid=6383&web_rid=xxx")
 * @return Signature string pointer on success, nullptr on failure.
 *         Caller must free the returned pointer using free_abogus().
 *
 * @note Thread-safe on macOS (automatically dispatches to main thread if needed)
 *
 * @example
 *   const char* ua = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)...";
 *   const char* params = "aid=6383&app_name=douyin_web&web_rid=123456";
 *   char* sig = get_abogus(ua, params);
 *   if (sig) {
 *       printf("a_bogus=%s\n", sig);
 *       free_abogus(sig);
 *   }
 */
char* get_abogus(const char* userAgent, const char* params);

/**
 * @brief Free signature string returned by get_abogus
 *
 * @param ptr Pointer returned by get_abogus()
 */
void free_abogus(char* ptr);

/**
 * @brief Get library version string
 * @return Version string (e.g., "1.0.0")
 */
const char* abogus_version(void);

#ifdef __cplusplus
}
#endif
