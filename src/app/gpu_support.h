/**
 * @file
 * @brief Installs the GPU engines when the build has them.
 *
 * Every program of the toolbox calls installGpuSupport() once at start; in
 * a build without CUDA it does nothing, and the core then has no GPU
 * provider.
 */

#ifndef QFTBX_GPU_SUPPORT_H
#define QFTBX_GPU_SUPPORT_H

namespace qftbx {

void installGpuSupport();

}

#endif
