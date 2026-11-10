/**
 * @file
 * @brief The exception every failure of a CUDA call becomes.
 *
 * Plain C++, so the glue that turns it into a message of the core includes
 * it without the CUDA toolkit. code() is CUDA's own error number.
 */

#ifndef QFTBX_GPU_CUDA_ERROR_H
#define QFTBX_GPU_CUDA_ERROR_H

#include <stdexcept>
#include <string>

namespace qftbx::gpu {

class CudaError : public std::runtime_error
{
public:
    CudaError(const std::string & what, int code) : std::runtime_error(what), m_code(code) {}

    int code() const noexcept { return m_code; }

private:
    int m_code = 0;
};

}

#endif
