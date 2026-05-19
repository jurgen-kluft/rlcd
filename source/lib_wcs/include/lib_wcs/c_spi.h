#ifndef __RLCD_WCS_SPI_H__
#define __RLCD_WCS_SPI_H__
#include "rcore/c_target.h"
#ifdef USE_PRAGMA_ONCE
    #pragma once
#endif

namespace ncore
{
    namespace nlcd
    {
        bool spi_is_initialized();
        bool spi_initialize();
    }  // namespace nlcd
}  // namespace ncore

#endif  // __RLCD_WCS_SPI_H__
