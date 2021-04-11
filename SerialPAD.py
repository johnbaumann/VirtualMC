"""Simple gamepad/joystick test example."""
# https://raw.githubusercontent.com/zeth/inputs/master/examples/jstest.py

from __future__ import print_function


import inputs
import serial
from serial.serialutil import EIGHTBITS


# Constants
# Pad Constants
PADUP = (1 << 12)
PADDOWN = (1 << 14)
PADLEFT = (1 << 15)
PADRIGHT = (1 << 13)
PADL1 = (1 << 2)
PADL2 = (1 << 0)
PADR1 = (1 << 3)
PADR2 = (1 << 1)
PADTRI = (1 << 4)
PADSQR = (1 << 7)
PADCRC = (1 << 5)
PADX = (1 << 6)
PADSTART = (1 << 11)
PADSELECT = (1 << 8)
# Serial Constants
SER_GET_ID_ = b'\xA0'
SER_GET_VERSION = b'\xA1'
SER_MC_READ = b'\xA2'
SER_MC_WRITE = b'\xA3'
SER_PAD_ON = b'\xB0'
SER_PAD_OFF = b'\xB1'
SER_MC_ON = b'\xB2'
SER_MC_OFF = b'\xB3'
SER_PAD_SETALL = b'\xC0'
# /Constants

PAD_DigitalSwitches = 0xFFFF

ser = serial.Serial(
    port=None,
    baudrate=38400,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=3,
    xonxoff=False,
    rtscts=False,
    dsrdtr=False
)

ser.port = 'COM12'
ser.rts = False

waiting_for_last_event = False


def pad_press(button_mask):
    global PAD_DigitalSwitches
    PAD_DigitalSwitches &= ~button_mask


def pad_release(button_mask):
    global PAD_DigitalSwitches
    PAD_DigitalSwitches |= button_mask


EVENT_ABB = (
    # D-PAD, aka HAT
    ('Absolute-ABS_HAT0X', 'HX'),
    ('Absolute-ABS_HAT0Y', 'HY'),

    # Face Buttons
    ('Key-BTN_NORTH', 'N'),
    ('Key-BTN_EAST', 'E'),
    ('Key-BTN_SOUTH', 'S'),
    ('Key-BTN_WEST', 'W'),

    # Other buttons
    ('Key-BTN_THUMBL', 'THL'),
    ('Key-BTN_THUMBR', 'THR'),
    ('Key-BTN_TL', 'TL'),
    ('Key-BTN_TR', 'TR'),
    ('Key-BTN_TL2', 'TL2'),
    ('Key-BTN_TR2', 'TR3'),
    ('Key-BTN_MODE', 'M'),
    # Xbox 360 controller these are backwards
    ('Key-BTN_SELECT', 'ST'),
    ('Key-BTN_START', 'SEL'),



    # PiHUT SNES style controller buttons
    # ('Key-BTN_TRIGGER', 'N'),
    # ('Key-BTN_THUMB', 'E'),
    # ('Key-BTN_THUMB2', 'S'),
    # ('Key-BTN_TOP', 'W'),
    # ('Key-BTN_BASE3', 'SL'),
    # ('Key-BTN_BASE4', 'ST'),
    # ('Key-BTN_TOP2', 'TL'),
    # ('Key-BTN_PINKIE', 'TR')
)


# This is to reduce noise from the PlayStation controllers
# For the Xbox controller, you can set this to 0
MIN_ABS_DIFFERENCE = 5


def send_pad_serial():
    digital_switches = PAD_DigitalSwitches.to_bytes(2, 'little')
    serial_dataOut = [SER_PAD_SETALL, digital_switches,
                      b'\xFF', b'\xFF', b'\xFF', b'\xFF']
    for x in serial_dataOut:
        ser.write(x)


class JSTest(object):
    """Simple joystick test class."""

    def __init__(self, gamepad=None, abbrevs=EVENT_ABB):
        self.btn_state = {}
        self.old_btn_state = {}
        self.abs_state = {}
        self.old_abs_state = {}
        self.abbrevs = dict(abbrevs)
        for key, value in self.abbrevs.items():
            if key.startswith('Absolute'):
                self.abs_state[value] = 0
                self.old_abs_state[value] = 0
            if key.startswith('Key'):
                self.btn_state[value] = 0
                self.old_btn_state[value] = 0
        self._other = 0
        self.gamepad = gamepad
        if not gamepad:
            self._get_gamepad()

    def _get_gamepad(self):
        """Get a gamepad object."""
        try:
            self.gamepad = inputs.devices.gamepads[0]
        except IndexError:
            raise inputs.UnpluggedError("No gamepad found.")

    def handle_unknown_event(self, event, key):
        """(dont)Deal with unknown events."""

        return None

        self.abbrevs[key] = new_abbv
        self._other += 1

        return self.abbrevs[key]

    def process_event(self, event):
        """Process the event into a state."""
        if event.ev_type == 'Sync':
            return
        if event.ev_type == 'Misc':
            return
        key = event.ev_type + '-' + event.code
        try:
            abbv = self.abbrevs[key]
        except KeyError:
            abbv = self.handle_unknown_event(event, key)
            if not abbv:
                return
        if event.ev_type == 'Key':
            self.old_btn_state[abbv] = self.btn_state[abbv]
            self.btn_state[abbv] = event.state
        if event.ev_type == 'Absolute':
            self.old_abs_state[abbv] = self.abs_state[abbv]
            self.abs_state[abbv] = event.state
        self.output_state(event.ev_type, abbv)

    def format_state(self):
        """Format the state."""
        output_string = ""
        for key, value in self.abs_state.items():
            output_string += key + ':' + '{:>4}'.format(str(value) + ' ')

        for key, value in self.btn_state.items():
            output_string += key + ':' + str(value) + ' '
            if key == 'S':
                if(value == 1):
                    pad_press(PADX)
                elif(value == 0):
                    pad_release(PADX)

            if key == 'HY':
                # print("hy")
                if(value == -1):
                    pad_release(PADDOWN)
                    pad_press(PADUP)
                elif(value == 0):
                    pad_release(PADUP)
                    pad_release(PADDOWN)
                if(value == -1):
                    pad_release(PADUP)
                    pad_press(PADDOWN)

        return output_string

    def set_pad_mask(self):
        for key, value in self.abs_state.items():
            if key == 'HY':
                if(value == -1):
                    pad_release(PADDOWN)
                    pad_press(PADUP)
                elif(value == 0):
                    pad_release(PADUP)
                    pad_release(PADDOWN)
                if(value == 1):
                    pad_release(PADUP)
                    pad_press(PADDOWN)

            elif key == 'HX':
                if(value == -1):
                    pad_release(PADRIGHT)
                    pad_press(PADLEFT)
                elif(value == 0):
                    pad_release(PADLEFT)
                    pad_release(PADRIGHT)
                if(value == 1):
                    pad_release(PADLEFT)
                    pad_press(PADRIGHT)

        for key, value in self.btn_state.items():

            if key == 'N':
                if(value == 1):
                    pad_press(PADTRI)
                elif(value == 0):
                    pad_release(PADTRI)

            elif key == 'E':
                if(value == 1):
                    pad_press(PADCRC)
                elif(value == 0):
                    pad_release(PADCRC)

            elif key == 'S':
                if(value == 1):
                    pad_press(PADX)
                elif(value == 0):
                    pad_release(PADX)

            elif key == 'W':
                if(value == 1):
                    pad_press(PADSQR)
                elif(value == 0):
                    pad_release(PADSQR)

            elif key == 'ST':
                if(value == 1):
                    pad_press(PADSTART)
                elif(value == 0):
                    pad_release(PADSTART)

            elif key == 'SEL':
                if(value == 1):
                    pad_press(PADSELECT)
                elif(value == 0):
                    pad_release(PADSELECT)

            elif key == 'TL':
                if(value == 1):
                    pad_press(PADL1)
                elif(value == 0):
                    pad_release(PADL1)

            elif key == 'TR':
                if(value == 1):
                    pad_press(PADR1)
                elif(value == 0):
                    pad_release(PADR1)

            elif value > 0:
                print(key)

    def output_state(self, ev_type, abbv):
        """Print out the output state."""
        if ev_type == 'Key':
            if self.btn_state[abbv] != self.old_btn_state[abbv]:
                self.set_pad_mask()
                send_pad_serial()
                # print(hex(PAD_DigitalSwitches))
                return

        if abbv[0] == 'H':
            self.set_pad_mask()
            send_pad_serial()
            # print(hex(PAD_DigitalSwitches))
            return

        difference = self.abs_state[abbv] - self.old_abs_state[abbv]
        if (abs(difference)) > MIN_ABS_DIFFERENCE:
            print(self.format_state())

    def process_events(self):
        global waiting_for_last_event
        """Process available events."""
        try:
            events = self.gamepad.read()
        except EOFError:
            events = []

            if waiting_for_last_event:
                if ser.cts == False:
                    waiting_for_last_event = False
                else:
                    return

        for event in events:
            self.process_event(event)


def main():
    """Process all events forever."""
    jstest = JSTest()
    if not ser.isOpen():
        ser.open()
    if ser.isOpen():
        while 1:
            jstest.process_events()


if __name__ == "__main__":
    main()
