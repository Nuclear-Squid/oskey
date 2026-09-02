/*
 * Copyright (c) 2026 mentaldesk
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

/* OS identifiers — match the dt-bindings values in oskey.h */
typedef enum {
    OSKEY_OS_UNSURE,
    OSKEY_OS_LINUX,
    OSKEY_OS_WINDOWS,
    OSKEY_OS_MACOS,
    OSKEY_OS_IOS,
} os_variant_t;

/**
 * @brief Get the currently selected operating system.
 * @return os_variant_t
 */
os_variant_t zmk_oskey_get_os(void);

/**
 * @brief Set the currently selected operating system.
 * @param os_variant_t
 */
void zmk_oskey_set_os(os_variant_t os);
