/*
 * Copyright (c) 2026 mentaldesk
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_os_selector

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <oskey/os_state.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct os_selector_config {
    uint8_t default_os;
};

static int on_os_selector_binding_pressed(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    zmk_oskey_set_os((uint8_t)binding->param1);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_os_selector_binding_released(struct zmk_behavior_binding *binding,
                                           struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_os_selector_driver_api = {
    .binding_pressed  = on_os_selector_binding_pressed,
    .binding_released = on_os_selector_binding_released,
};

static int behavior_os_selector_init(const struct device *dev) {
    const struct os_selector_config* cfg = dev->config;
    if (cfg->default_os != -1) {
        zmk_oskey_set_os(cfg->default_os);
    }
    return 0;
}

#define OS_SELECTOR_INST(n)                                                                        \
    static const struct os_selector_config os_selector_config_##n = {                              \
        .default_os = DT_INST_PROP_OR(n, default_os, -1),                                          \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_os_selector_init, NULL, NULL, &os_selector_config_##n,     \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_os_selector_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OS_SELECTOR_INST)
