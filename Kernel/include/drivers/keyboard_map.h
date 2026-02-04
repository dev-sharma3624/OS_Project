#pragma once

char scan_code_for_lookup_table[128] = {
    // --- Row 1: System & Numbers ---
    0,      // 0x00: Null / Error
    0x1B,   // 0x01: Esc
    '1',    // 0x02: 1
    '2',    // 0x03: 2
    '3',    // 0x04: 3
    '4',    // 0x05: 4
    '5',    // 0x06: 5
    '6',    // 0x07: 6
    '7',    // 0x08: 7
    '8',    // 0x09: 8
    '9',    // 0x0A: 9
    '0',    // 0x0B: 0
    '-',    // 0x0C: - (Minus)
    '=',    // 0x0D: = (Equals)
    '\b',   // 0x0E: Backspace (Changed from 0)

    // --- Row 2: Tab & QWERTY ---
    '\t',   // 0x0F: Tab (Changed from 0)
    'q',    // 0x10: q
    'w',    // 0x11: w
    'e',    // 0x12: e
    'r',    // 0x13: r
    't',    // 0x14: t
    'y',    // 0x15: y
    'u',    // 0x16: u
    'i',    // 0x17: i
    'o',    // 0x18: o
    'p',    // 0x19: p
    '[',    // 0x1A: [
    ']',    // 0x1B: ]
    '\n',   // 0x1C: Enter

    // --- Row 3: Control & ASDF ---
    0,      // 0x1D: Left Control
    'a',    // 0x1E: a
    's',    // 0x1F: s
    'd',    // 0x20: d
    'f',    // 0x21: f
    'g',    // 0x22: g
    'h',    // 0x23: h
    'j',    // 0x24: j
    'k',    // 0x25: k
    'l',    // 0x26: l
    ';',    // 0x27: ;
    '\'',   // 0x28: ' (Single Quote)
    '`',    // 0x29: ` (Backtick)

    // --- Row 4: Shift & ZXCV ---
    0,      // 0x2A: Left Shift
    '\\',   // 0x2B: \ (Backslash)
    'z',    // 0x2C: z
    'x',    // 0x2D: x
    'c',    // 0x2E: c
    'v',    // 0x2F: v
    'b',    // 0x30: b
    'n',    // 0x31: n
    'm',    // 0x32: m
    ',',    // 0x33: ,
    '.',    // 0x34: .
    '/',    // 0x35: /
    0,      // 0x36: Right Shift

    // --- Row 5: Keypad & Space ---
    '*',    // 0x37: Keypad *
    0,      // 0x38: Left Alt
    ' ',    // 0x39: Space
    0,      // 0x3A: Caps Lock

    // --- Function Keys ---
    0,      // 0x3B: F1
    0,      // 0x3C: F2
    0,      // 0x3D: F3
    0,      // 0x3E: F4
    0,      // 0x3F: F5
    0,      // 0x40: F6
    0,      // 0x41: F7
    0,      // 0x42: F8
    0,      // 0x43: F9
    0,      // 0x44: F10

    // --- Keypad & Locks ---
    0,      // 0x45: Num Lock
    0,      // 0x46: Scroll Lock
    '7',    // 0x47: Keypad 7
    '8',    // 0x48: Keypad 8
    '9',    // 0x49: Keypad 9
    '-',    // 0x4A: Keypad -
    '4',    // 0x4B: Keypad 4
    '5',    // 0x4C: Keypad 5
    '6',    // 0x4D: Keypad 6
    '+',    // 0x4E: Keypad +
    '1',    // 0x4F: Keypad 1
    '2',    // 0x50: Keypad 2
    '3',    // 0x51: Keypad 3
    '0',    // 0x52: Keypad 0
    '.',    // 0x53: Keypad .
    
    // --- Extended (Rarely used in simple shells) ---
    0,      // 0x54: Unknown
    0,      // 0x55: Unknown
    0,      // 0x56: Unknown
    0,      // 0x57: F11
    0       // 0x58: F12
};

char scan_code_for_shift_lookup_table[128] = {
    // --- Row 1: System & Symbols (!@#) ---
    0,      // 0x00: Null
    0x1B,   // 0x01: Esc (Shift+Esc is usually just Esc)
    '!',    // 0x02: 1 -> !
    '@',    // 0x03: 2 -> @
    '#',    // 0x04: 3 -> #
    '$',    // 0x05: 4 -> $
    '%',    // 0x06: 5 -> %
    '^',    // 0x07: 6 -> ^
    '&',    // 0x08: 7 -> &
    '*',    // 0x09: 8 -> *
    '(',    // 0x0A: 9 -> (
    ')',    // 0x0B: 0 -> )
    '_',    // 0x0C: - -> _ (Underscore)
    '+',    // 0x0D: = -> + (Plus)
    '\b',   // 0x0E: Backspace (Shift+Backspace is usually Backspace)

    // --- Row 2: Tab & Uppercase QWERTY ---
    '\t',   // 0x0F: Tab (Shift+Tab is usually Tab)
    'Q',    // 0x10: q -> Q
    'W',    // 0x11: w -> W
    'E',    // 0x12: e -> E
    'R',    // 0x13: r -> R
    'T',    // 0x14: t -> T
    'Y',    // 0x15: y -> Y
    'U',    // 0x16: u -> U
    'I',    // 0x17: i -> I
    'O',    // 0x18: o -> O
    'P',    // 0x19: p -> P
    '{',    // 0x1A: [ -> {
    '}',    // 0x1B: ] -> }
    '\n',   // 0x1C: Enter

    // --- Row 3: Control & Uppercase ASDF ---
    0,      // 0x1D: Left Control
    'A',    // 0x1E: a -> A
    'S',    // 0x1F: s -> S
    'D',    // 0x20: d -> D
    'F',    // 0x21: f -> F
    'G',    // 0x22: g -> G
    'H',    // 0x23: h -> H
    'J',    // 0x24: j -> J
    'K',    // 0x25: k -> K
    'L',    // 0x26: l -> L
    ':',    // 0x27: ; -> : (Colon)
    '"',    // 0x28: ' -> " (Double Quote)
    '~',    // 0x29: ` -> ~ (Tilde)

    // --- Row 4: Shift & Uppercase ZXCV ---
    0,      // 0x2A: Left Shift
    '|',    // 0x2B: \ -> | (Pipe)
    'Z',    // 0x2C: z -> Z
    'X',    // 0x2D: x -> X
    'C',    // 0x2E: c -> C
    'V',    // 0x2F: v -> V
    'B',    // 0x30: b -> B
    'N',    // 0x31: n -> N
    'M',    // 0x32: m -> M
    '<',    // 0x33: , -> < (Less Than)
    '>',    // 0x34: . -> > (Greater Than)
    '?',    // 0x35: / -> ? (Question Mark)
    0,      // 0x36: Right Shift

    // --- Row 5: Keypad & Space ---
    '*',    // 0x37: Keypad * (Shift usually doesn't change this)
    0,      // 0x38: Left Alt
    ' ',    // 0x39: Space (Shift+Space is Space)
    0,      // 0x3A: Caps Lock

    // --- Function Keys (F1-F10 usually don't shift in simple terminals) ---
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x3B - 0x44

    // --- Keypad (With Shift, these act like NumLock is OFF) ---
    0,      // 0x45: Num Lock
    0,      // 0x46: Scroll Lock
    '7',    // 0x47: Keypad 7 -> Home (Depending on implementation, 0 or specific code)
    '8',    // 0x48: Keypad 8 -> Up
    '9',    // 0x49: Keypad 9 -> PgUp
    '-',    // 0x4A: Keypad -
    '4',    // 0x4B: Keypad 4 -> Left
    '5',    // 0x4C: Keypad 5 -> Center
    '6',    // 0x4D: Keypad 6 -> Right
    '+',    // 0x4E: Keypad +
    '1',    // 0x4F: Keypad 1 -> End
    '2',    // 0x50: Keypad 2 -> Down
    '3',    // 0x51: Keypad 3 -> PgDn
    '0',    // 0x52: Keypad 0 -> Ins
    '.',    // 0x53: Keypad . -> Del
};