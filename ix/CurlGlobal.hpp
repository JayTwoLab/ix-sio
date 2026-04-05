#pragma once

#include <curl/curl.h>
#include <atomic>
#include <stdexcept>

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
