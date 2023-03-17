//---------------------------------------------------------------------------//
// ==>/[_]\   fxlink: A community communication tool for CASIO calculators.  //
//    |:::|   Made by Lephe' as part of the fxSDK.                           //
//    \___/   License: MIT <https://opensource.org/licenses/MIT>             //
//---------------------------------------------------------------------------//
// fxlink.tui.tui: Global data and main functions for the interactive TUI

#pragma once
#include <fxlink/tui/layout.h>
#include <fxlink/tui/render.h>
#include <fxlink/tui/input.h>
#include <fxlink/devices.h>
#include <fxlink/logging.h>

#include <libusb.h>
#include <ncurses.h>

struct TUIData {
    /* SIGWINCH flag */
    bool resize_needed;
    /* ncurses window panels */
    WINDOW *wStatus;
    WINDOW *wLogs;
    WINDOW *wTransfers;
    WINDOW *wTextOutput;
    WINDOW *wConsole;
    /* Root box */
    struct fxlink_TUI_box *bRoot;
    /* Application data */
    struct fxlink_pollfds polled_fds;
    struct fxlink_device_list devices;
};

extern struct TUIData TUI;

void TUI_execute_command(char const *command);
