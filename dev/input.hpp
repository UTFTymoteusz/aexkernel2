#pragma once

#include "aex/dev/input.hpp"
#include "aex/dev/inputdevice.hpp"

namespace AEX::Dev::Input {
    void init();

    void key_press(hid_keycode_t code);
    void key_release(hid_keycode_t code);

    void register_handle(Handle* handle);
    void unregister_handle(Handle* handle);

    void register_device(InputDevice* handle);
    void unregister_device(InputDevice* handle);
}