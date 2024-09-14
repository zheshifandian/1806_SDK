/* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * SPDX-FileCopyrightText: 2020 the prplMesh contributors (see AUTHORS.md)
 *
 * This code is subject to the terms of the BSD+Patent license.
 * See LICENSE file for more details.
 */

#include "amxb_wrapper.h"

#include <gtest/gtest.h>

namespace c_wrappers {

static AmbxWrapper *amxb_wrapper_singleton = nullptr;

AmbxWrapper::AmbxWrapper() { amxb_wrapper_singleton = this; }

AmbxWrapper::~AmbxWrapper() { amxb_wrapper_singleton = nullptr; }

} //namespace c_wrappers

extern "C" {

int amxb_be_load(const char *path_name)
{
    if (c_wrappers::amxb_wrapper_singleton)
        return c_wrappers::amxb_wrapper_singleton->amxb_be_load(path_name);
    else
        ADD_FAILURE();
    return -1;
}

int amxb_connect(amxb_bus_ctx_t **ctx, const char *uri)
{
    if (c_wrappers::amxb_wrapper_singleton)
        return c_wrappers::amxb_wrapper_singleton->amxb_connect(ctx, uri);
    else
        ADD_FAILURE();
    return -1;
}

int amxb_register(amxb_bus_ctx_t *const ctx, amxd_dm_t *const dm)
{
    if (c_wrappers::amxb_wrapper_singleton)
        return c_wrappers::amxb_wrapper_singleton->amxb_register(ctx, dm);
    else
        ADD_FAILURE();
    return -1;
}

int amxb_read(const amxb_bus_ctx_t *const ctx)
{
    if (c_wrappers::amxb_wrapper_singleton)
        return c_wrappers::amxb_wrapper_singleton->amxb_read(ctx);
    else
        ADD_FAILURE();
    return -1;
}

int amxb_get_fd(const amxb_bus_ctx_t *const ctx)
{
    if (c_wrappers::amxb_wrapper_singleton)
        return c_wrappers::amxb_wrapper_singleton->amxb_get_fd(ctx);
    else
        ADD_FAILURE();
    return -1;
}

void amxb_free(amxb_bus_ctx_t **ctx)
{
    if (c_wrappers::amxb_wrapper_singleton)
        c_wrappers::amxb_wrapper_singleton->amxb_free(ctx);
    else
        ADD_FAILURE();
}

void amxb_be_remove_all(void)
{
    if (c_wrappers::amxb_wrapper_singleton)
        c_wrappers::amxb_wrapper_singleton->amxb_be_remove_all();
    else
        ADD_FAILURE();
}

} //extern "C"
