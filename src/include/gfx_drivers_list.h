#ifndef _GFX_DRIVERS_LIST_H_
#define _GFX_DRIVERS_LIST_H_

#include <gfx_driver.h>

extern gfx_driver_t gfx_driver_ggi;

static gfx_driver_t *gfx_drivers[] = {
  &gfx_driver_ggi,
  NULL
};

#endif
