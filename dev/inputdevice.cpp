#include "aex/dev/inputdevice.hpp"

#include "dev/input.hpp"

namespace AEX::Dev {
    InputDevice::~InputDevice() {}

    void InputDevice::keyPress(Input::hid_keycode_t code) {
        Input::key_press(code);
    }

    void InputDevice::keyRelease(Input::hid_keycode_t code) {
        Input::key_release(code);
    }

    void InputDevice::updateLEDs(led_flag_t) {}

    bool InputDevice::registerDevice() {
        Input::register_device(this);
        return true;
    }
}