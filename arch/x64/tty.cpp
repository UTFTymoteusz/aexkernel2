#include "aex/tty.hpp"

#include "aex/arch/sys/cpu.hpp"
#include "aex/mem.hpp"
#include "aex/printk.hpp"
#include "aex/string.hpp"

#include "boot/mboot.h"
#include "grtty.hpp"
#include "txtty.hpp"

#include <stddef.h>
#include <stdint.h>

using namespace AEX::Dev::Input;

namespace AEX {
    VTTY::VTTY() {}
    VTTY::VTTY(int width, int height) {
        this->width  = width;
        this->height = height;
    }

    void VTTY::_writeChar(char) {
        // kpanic("VTTY::_writeChar() not implemented");
    }

    void VTTY::scrollDown(int) {
        // kpanic("VTTY::scrollDown() not implemented");
    }

    VTTY& VTTY::setColorANSI(int) {
        // kpanic("VTTY::setColorANSI() not implemented");
        return *this;
    }

    char VTTY::readChar() {
        char c;
        m_inputBuffer->read(&c, 1);

        return c;
    }

    void VTTY::writeChar(char c) {
        m_lock.acquire();
        // Sys::CPU::outportb(0xE9, c);
        _writeChar(c);
        m_lock.release();
    }

    void VTTY::write(const char* str) {
        m_lock.acquire();

        while (*str != '\0') {
            // Sys::CPU::outportb(0xE9, *str);
            _writeChar(*str++);
        }

        m_lock.release();
    }

    void VTTY::inputReady() {
        m_inputBuffer = new Mem::CircularBuffer(2048);
    }

    void VTTY::inputKeyPress(event m_event) {
        if (!m_inputBuffer->writeAvailable())
            return;

        if (m_event.mod & KEYMOD_CTRL && m_event.keycode >= KEY_A && m_event.keycode <= KEY_Z) {
            char cc = 1 + (m_event.keycode - KEY_A);
            m_inputBuffer->write(&cc, 1);

            return;
        }

        char c = translateEvent(&m_keymap, m_event);
        if (!c)
            return;

        m_inputBuffer->write(&c, 1);
    }

    VTTY* VTTYs[TTY_AMOUNT];
    TxTTY tx_init_tty;
    GrTTY gr_init_tty;

    uint16_t buffer[80 * 25 * 2];

    void VTTY::init(multiboot_info_t* mbinfo) {
        TxTTY::clear();

        if ((mbinfo->flags & (1 << 2)) &&
            mbinfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
            new (&tx_init_tty) TxTTY((void*) 0xFFFFFFFF800B8000);
        }
        else {
            memset64(buffer, '\0', sizeof(buffer) / sizeof(uint64_t));
            new (&tx_init_tty) TxTTY((void*) &buffer);
        }

        VTTYs[0] = &tx_init_tty;
    }

    void VTTY::init_mem(multiboot_info_t* mbinfo) {
        if ((mbinfo->flags & (1 << 2)) &&
            mbinfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT) {
            for (int i = 1; i < TTY_AMOUNT; i++)
                VTTYs[i] = new TxTTY((void*) 0xFFFFFFFF800B8000);

            return;
        }

        new (&gr_init_tty) GrTTY(mbinfo);

        gr_init_tty.fillFromEGA((GrTTY::vga_char*) tx_init_tty.getOutputPointer());
        VTTYs[0] = &gr_init_tty;

        gr_init_tty.setCursorX(tx_init_tty.getCursorX());
        gr_init_tty.setCursorY(tx_init_tty.getCursorY());

        for (int i = 1; i < TTY_AMOUNT; i++)
            VTTYs[i] = new GrTTY(mbinfo);
    }
}