/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _AMXB_WRAPPER_H_
#define _AMXB_WRAPPER_H_

// Ambiorix
#include <amxc/amxc.h>
#include <amxp/amxp.h>

#include <amxd/amxd_dm.h>

#include <amxb/amxb.h>
#include <amxb/amxb_register.h>

namespace c_wrappers {

class AmbxWrapper {
public:
    AmbxWrapper();
    virtual ~AmbxWrapper();

    virtual int amxb_be_load(const char *path_name)                           = 0;
    virtual int amxb_connect(amxb_bus_ctx_t **ctx, const char *uri)           = 0;
    virtual int amxb_register(amxb_bus_ctx_t *const ctx, amxd_dm_t *const dm) = 0;
    virtual int amxb_read(const amxb_bus_ctx_t *const ctx)                    = 0;
    virtual int amxb_get_fd(const amxb_bus_ctx_t *const ctx)                  = 0;
    virtual void amxb_free(amxb_bus_ctx_t **ctx)                              = 0;
    virtual void amxb_be_remove_all(void)                                     = 0;
};

} //namespace c_wrappers

#endif // _AMXB_WRAPPER_H_
