#pragma once

#include "aex/dev/input.hpp"
#include "aex/utility.hpp"

namespace AEX::Dev {
    class API InputDevice {
        public:
        enum led_flag_t {
            LED_NONE       = 0x00,
            LED_NUMLOCK    = 0x01,
            LED_CAPSLOCK   = 0x02,
            LED_SCROLLLOCK = 0x04,
        };

        virtual ~InputDevice();

        /**
         * Calling this method notifies of a key press.
         * @param key HID key code.
         **/
        void keyPress(Input::hid_keycode_t key);

        /**
         * Calling this method notifies of a key release.
         * @param key HID key code.
         **/
        void keyRelease(Input::hid_keycode_t key);

        /**
         * Called to update the status LEDs.
         * @param flags Flags.
         **/
        virtual void updateLEDs(led_flag_t flags);

        /**
         * Registers the device.
         * @returns True if registration has succeded.
         **/
        bool registerDevice();
    };
}