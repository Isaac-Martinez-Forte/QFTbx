/**
 * @file
 * @brief Functions that compile for the card and for the processor.
 *
 * Under nvcc QFTBX_HD makes a function callable from both; under the host
 * compiler it is nothing. The GPU tests call such functions on the
 * processor, so their logic is tested without a card.
 */

#ifndef QFTBX_GPU_HOST_DEVICE_H
#define QFTBX_GPU_HOST_DEVICE_H

#ifdef __CUDACC__
#define QFTBX_HD __host__ __device__
#else
#define QFTBX_HD
#endif

#endif
