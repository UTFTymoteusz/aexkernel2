#include "aex/dev/input.hpp"

#include "aex/dev/inputdevice.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/proc.hpp"
#include "aex/spinlock.hpp"
#include "aex/tty.hpp"

namespace AEX::Dev::Input {
    Spinlock lock;

    Mem::SmartArray<InputDevice> devices;
    Mem::Vector<Handle*>         handles;

    bool num_lock  = true;
    bool caps_lock = false;

    bool pressed[256] = {};

    int shift = 0;
    int ctrl  = 0;
    int alt   = 0;
    int altgr = 0;

    void     update_leds();
    keymod_t get_mod();
    void     tty_input_thread();

    void init() {
        for (int i = 0; i < TTY_AMOUNT; i++) {
            auto tty = VTTYs[i];

            tty->inputReady();
        }

        auto thread = new Proc::Thread(nullptr, (void*) tty_input_thread, 8192, nullptr);
        thread->start();
    }

    void key_press(hid_keycode_t code) {
        keymod_t mod = get_mod();

        for (int i = 0; i < handles.count(); i++) {
            auto handle = handles[i];

            event m_event;

            m_event.keycode = code;
            m_event.mod     = mod;

            handle->writeEvent(m_event);
        }

        if (!pressed[code]) {
            switch (code) {
            case KEY_NUMLOCK:
                num_lock = !num_lock;
                update_leds();
                break;
            case KEY_CAPSLOCK:
                caps_lock = !caps_lock;
                update_leds();
                break;
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                Mem::atomic_add(&shift, 1);
                break;
            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                Mem::atomic_add(&ctrl, 1);
                break;
            case KEY_LEFTALT:
                Mem::atomic_add(&alt, 1);
                break;
            case KEY_RIGHTALT:
                Mem::atomic_add(&altgr, 1);
                break;
            default:
                break;
            }
        }

        pressed[code] = true;
    }

    void key_release(hid_keycode_t code) {
        keymod_t mod = get_mod();

        for (int i = 0; i < handles.count(); i++) {
            auto  handle = handles[i];
            event m_event;

            m_event.keycode = code;
            m_event.mod     = (keymod_t)(mod | KEYMOD_RELEASE);

            handle->writeEvent(m_event);
        }

        if (pressed[code]) {
            switch (code) {
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                Mem::atomic_sub(&shift, 1);
                break;
            case KEY_LEFTCTRL:
            case KEY_RIGHTCTRL:
                Mem::atomic_sub(&ctrl, 1);
                break;
            case KEY_LEFTALT:
                Mem::atomic_sub(&alt, 1);
                break;
            case KEY_RIGHTALT:
                Mem::atomic_sub(&altgr, 1);
                break;
            default:
                break;
            }
        }

        pressed[code] = false;
    }

    void register_handle(Handle* handle) {
        handles.pushBack(handle);
    }

    void unregister_handle(Handle* m_handle) {
        for (int i = 0; i < handles.count(); i++) {
            if (handles[i] != m_handle)
                continue;

            handles.erase(i);
            i--;
        }
    }

    void register_device(InputDevice* device) {
        devices.addRef(device);
        update_leds();
    }

    void update_leds() {
        auto flags = InputDevice::LED_NONE;

        if (num_lock)
            flags = (InputDevice::led_flag_t)(flags | InputDevice::LED_NUMLOCK);

        if (caps_lock)
            flags = (InputDevice::led_flag_t)(flags | InputDevice::LED_CAPSLOCK);

        for (auto iterator = devices.getIterator(); auto device = iterator.next();)
            device->updateLEDs(flags);
    }

    keymod_t get_mod() {
        auto mod = KEYMOD_NONE;

        if (num_lock)
            mod = (keymod_t)(mod | KEYMOD_NUMLOCK);

        if (caps_lock)
            mod = (keymod_t)(mod | KEYMOD_CAPSLOCK);

        if (shift)
            mod = (keymod_t)(mod | KEYMOD_SHIFT);

        if (ctrl)
            mod = (keymod_t)(mod | KEYMOD_CTRL);

        if (alt)
            mod = (keymod_t)(mod | KEYMOD_ALT);

        if (altgr)
            mod = (keymod_t)(mod | KEYMOD_ALTGR);

        return mod;
    }

    char translateEvent(keymap* m_keymap, event& m_event) {
        auto& key  = m_keymap->keys[m_event.keycode];
        bool  caps = m_event.mod & KEYMOD_CAPSLOCK;

        if (m_event.mod & KEYMOD_ALTGR)
            return caps ? key.capslock_ctrl_alt : key.ctrl_alt;
        else if (m_event.mod & KEYMOD_CTRL)
            return caps ? key.capslock_ctrl : key.ctrl;
        else if (m_event.mod & KEYMOD_SHIFT)
            return caps ? key.capslock_shift : key.shift;

        return caps ? key.capslock : key.normal;
    }

    void tty_input_thread() {
        auto handle = Handle::getHandle(1024);
        handle.begin();

        while (true) {
            auto event = handle.readEvent();
            if (event.mod & KEYMOD_RELEASE)
                continue;

            VTTYs[ROOT_TTY]->inputKeyPress(event);
        }
    }
}