#pragma once

#include <curl/curl.h> // libcurl global init and cleanup functions

#include <atomic>
#include <stdexcept>

// CurlGlobal : RAII wrapper for curl_global_init and curl_global_cleanup
//
// Example usage:
//   std::unique_ptr<j2::network::CurlGlobal> curlInit = nullptr;
//   try {
//      curlInit = std::make_unique<j2::network::CurlGlobal>();
//      // CurlGlobal will automatically clean up when going out of scope at the end of main
//   } catch (const std::exception& ex) {
//      console->critical("[Fatal] CurlGlobal initialization failed: {}", ex.what());
//      return 1;
//   }
//
// Note: CurlGlobal is non-copyable and non-movable to make ownership explicit.
// CurlGlobal maintains a static atomic reference count to allow multiple instances to be created safely; the first instance will perform initialization and the last instance will perform cleanup.
namespace j2::network {

struct CurlGlobal {
    CurlGlobal() {
        // Increment ref count; if transitioning 0->1, perform init.
        if (ref_.fetch_add(1, std::memory_order_acq_rel) == 0) {
            if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
                // revert ref count and report error
                ref_.fetch_sub(1, std::memory_order_acq_rel);
                throw std::runtime_error("curl_global_init failed");
            }
        }
    }

    ~CurlGlobal() {
        // Decrement ref count; if transitioning 1->0, perform cleanup.
        if (ref_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            curl_global_cleanup();
        }
    }

    // non-copyable, non-movable to make ownership explicit
    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
    CurlGlobal(CurlGlobal&&) = delete;
    CurlGlobal& operator=(CurlGlobal&&) = delete;

private:
    inline static std::atomic<int> ref_{0};
};

} // namespace j2::network
