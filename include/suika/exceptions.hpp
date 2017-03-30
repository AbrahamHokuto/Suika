#ifndef SKA_EXCEPTIONS_HPP
#define SKA_EXCEPTIONS_HPP

#include <exception>

namespace suika {
        class deadlock_would_occur: std::exception {};
        class invalid_handle: std::exception {};
};

#endif  // SKA_EXCEPTIONS_HPP
