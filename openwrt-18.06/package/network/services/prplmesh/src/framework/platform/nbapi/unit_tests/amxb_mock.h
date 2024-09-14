/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#ifndef _AMXB_MOCK_H_
#define _AMXB_MOCK_H_

// Ambiorix
#include <amxc/amxc.h>
#include <amxp/amxp.h>

#include <amxd/amxd_dm.h>

#include <amxb/amxb.h>
#include <amxb/amxb_register.h>

#include "amxb_wrapper.h"

class AmbxMock : public c_wrappers::AmbxWrapper {
public:
    MOCK_METHOD(int, amxb_be_load, (const char *path_name), (override));
    MOCK_METHOD(int, amxb_connect, (amxb_bus_ctx_t * *ctx, const char *uri), (override));
    MOCK_METHOD(int, amxb_register, (amxb_bus_ctx_t *const ctx, amxd_dm_t *const dm), (override));
    MOCK_METHOD(int, amxb_read, (const amxb_bus_ctx_t *const ctx), (override));
    MOCK_METHOD(int, amxb_get_fd, (const amxb_bus_ctx_t *const ctx), (override));
    MOCK_METHOD(void, amxb_free, (amxb_bus_ctx_t * *ctx), (override));
    MOCK_METHOD(void, amxb_be_remove_all, (), (override));
};

#endif //_AMXB_MOCK_H_
