/*
 * Copyright (c) 2026 mentaldesk
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <oskey/os_state.h>
#include <dt-bindings/zmk/oskey.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_os_key_config {
    struct zmk_behavior_binding uns_binding;
    struct zmk_behavior_binding lin_binding;
    struct zmk_behavior_binding win_binding;
    struct zmk_behavior_binding mac_binding;
    struct zmk_behavior_binding ios_binding;
};

/*
 * Track active (pressed but not yet released) os-key presses so that the
 * release always fires on the same binding that was activated at press time,
 * even if the OS selection changes while the key is held.
 */
#define ZMK_BHV_MAX_ACTIVE_OS_KEYS 10

struct active_os_key {
    bool active;
    uint32_t position;
    struct zmk_behavior_binding binding;
};

static struct active_os_key active_os_keys[ZMK_BHV_MAX_ACTIVE_OS_KEYS];

static struct active_os_key *find_active_os_key(uint32_t position) {
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_OS_KEYS; i++) {
        if (active_os_keys[i].active && active_os_keys[i].position == position) {
            return &active_os_keys[i];
        }
    }
    return NULL;
}

static struct active_os_key *alloc_active_os_key(uint32_t position,
                                                  const struct zmk_behavior_binding *binding) {
    for (int i = 0; i < ZMK_BHV_MAX_ACTIVE_OS_KEYS; i++) {
        if (!active_os_keys[i].active) {
            active_os_keys[i].active   = true;
            active_os_keys[i].position = position;
            active_os_keys[i].binding  = *binding;
            return &active_os_keys[i];
        }
    }
    return NULL;
}

static void release_active_os_key(struct active_os_key *key) { key->active = false; }

static const struct zmk_behavior_binding *
select_binding(const struct behavior_os_key_config *cfg) {
    switch (zmk_oskey_get_os()) {
    case OSKEY_OS_IOS:
        return &cfg->ios_binding;
    case OSKEY_OS_MACOS:
        return &cfg->mac_binding;
    case OSKEY_OS_LINUX:
        return &cfg->lin_binding;
    case OSKEY_OS_WINDOWS:
        return &cfg->win_binding;
    default:
        return &cfg->uns_binding;
    }
}

static void replace_placeholder_params(struct zmk_behavior_binding *binding,
                                       uint32_t param1, uint32_t param2) {
    if (binding->param1 == OSKEY_1ST_PARAM) {
        binding->param1 = param1;
    }
    else if (binding->param1 == OSKEY_2ND_PARAM) {
        binding->param1 = param2;
    }

    if (binding->param2 == OSKEY_1ST_PARAM) {
        binding->param2 = param1;
    }
    else if (binding->param2 == OSKEY_2ND_PARAM) {
        binding->param1 = param2;
    }
}

static int on_os_key_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_os_key_config *cfg = dev->config;
    const struct zmk_behavior_binding *target = select_binding(cfg);

    struct active_os_key *key = alloc_active_os_key(event.position, target);
    if (!key) {
        LOG_ERR("oskey: no free active-key slots");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    replace_placeholder_params(&key->binding, binding->param1, binding->param2);
    return zmk_behavior_invoke_binding(&key->binding, event, true);
}

static int on_os_key_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    struct active_os_key *key = find_active_os_key(event.position);
    if (!key) {
        LOG_ERR("oskey: release with no matching press at position %d", event.position);
        return ZMK_BEHAVIOR_OPAQUE;
    }

    int ret = zmk_behavior_invoke_binding(&key->binding, event, false);
    release_active_os_key(key);
    return ret;
}

static const struct behavior_driver_api behavior_os_key_driver_api = {
    .binding_pressed  = on_os_key_binding_pressed,
    .binding_released = on_os_key_binding_released,
};

static int behavior_os_key_init(const struct device *dev) { return 0; }

#define _OS_KEY_BINDING(n, idx) {                                            \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(n, bindings, idx)), \
        .param1 = DT_PHA_BY_IDX_OR(n, bindings, idx, param1, 0),             \
        .param2 = DT_PHA_BY_IDX_OR(n, bindings, idx, param2, 0),             \
    }

#define OS_KEY_BINDING(inst, idx)                                            \
    COND_CODE_1(DT_PROP_HAS_IDX(inst, bindings, idx),                        \
        (_OS_KEY_BINDING(inst, idx)),                                        \
        (_OS_KEY_BINDING(inst, 0)))

#define OS_KEY_INST(inst)                                                    \
    static struct behavior_os_key_config behavior_os_key_config_##inst = {   \
        .uns_binding = OS_KEY_BINDING(inst, 0),                              \
        .lin_binding = OS_KEY_BINDING(inst, 1),                              \
        .win_binding = OS_KEY_BINDING(inst, 2),                              \
        .mac_binding = OS_KEY_BINDING(inst, 3),                              \
        .ios_binding = OS_KEY_BINDING(inst, 4),                              \
    };                                                                       \
    BEHAVIOR_DT_DEFINE(inst, behavior_os_key_init, NULL, NULL,               \
                            &behavior_os_key_config_##inst, POST_KERNEL,     \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,             \
                            &behavior_os_key_driver_api);

DT_FOREACH_STATUS_OKAY(zmk_behavior_os_key, OS_KEY_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_os_key_one_param, OS_KEY_INST)
DT_FOREACH_STATUS_OKAY(zmk_behavior_os_key_two_param, OS_KEY_INST)
