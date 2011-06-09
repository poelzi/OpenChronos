#!/usr/bin/env python2
# encoding: utf-8

import urwid
import urwid.raw_display
import sys


import re, sys, random
from sorteddict import SortedDict

# {0x79, 0x56, 0x34, 0x12}
def rand_hw():
    res = []
    for i in range(4):
        res.append(random.randint(1, 254))
    #res.insert(0, random.randint(1, 254))
    res.sort(reverse=True)
    return "{" + ",".join([hex(x) for x in res]) + "}"

DATA = SortedDict()

DATA["CONFIG_FREQUENCY"] = {
        "name": "Frequency",
        "depends": [],
        "default": 902,
        "type": "choices",
        "values": [902, 868, 433],
        "help": "Radio frequency for the clock"

}

DATA["OPTION_TIME_DISPLAY"] = {
        "name": "Time display options",
        "depends": [],
        "default": 0,
        "type": "choices",
        "values": [(0, "24hr"), (1, "12hr (AM/PM)"), (2, "selectable")],
        "help": "Select how time should be displayed, in order of code size options are- 0 = 24hr, 1=12hr (AM/PM) or 2=selectable"
}

DATA["CONFIG_METRIC_ONLY"] = {
        "name": "Metric only code (-286 bytes)",
        "depends": [],
        "default": False,
        "help": "Only add code for Metric units (meter/celsius) to reduce image size",
}

DATA["FIXEDPOINT"] = {
        "name": "Fixedpoint (-5078 bytes)",
        "depends": [],
        "default": False,
        "help": "Tries to use fix point aritmetric. If no module is using it, it reduces the code size dramaticly. EXPERIMENTAL",
}

DATA["THIS_DEVICE_ADDRESS"] = {
        "name": "Hardware address",
        "type": "text",
        "default": rand_hw(),
        "ifndef": True,
        "help": "Default Radio Hardware Address to use on the device",
}


DATA["USE_LCD_CHARGE_PUMP"] = {
        "name": "Use LCD Charge Pump (6 bytes)",
        "default": False,
        "help": "Use the internal charge pump to make the display contrast contstant through the whole battery lifetime. As a downside this increases currency and reduces battery lifetime.",
}

DATA["USE_WATCHDOG"] = {
        "name": "Use Watchdog (20 bytes)",
        "default": True,
        "help": "Protects the clock against deadlocks by rebooting it.",
}

# FIXME implement
# DATA["CONFIG_AUTOSYNC"] = {
#         "name": "Automaticly SYNC after reboot",
#         "default": False,
# 		"help": "Automaticly sync clock after reboot",
# }


DATA["DEBUG"] = {
        "name": "Debug",
        "default": False,
        "help": "Activates debug code",
        }

# modules

DATA["CONFIG_DAY_OF_WEEK"] = {
        "name": "Date: Day of Week (496 bytes)",
        "depends": [],
        "default": True,
        "help": "Shows day of week on the date module",
}

DATA["CONFIG_TEST"] = {
        "name": "Test Mode (614 bytes)",
        "depends": [],
        "default": True,
        "help": "Test module to test some functionalities when the clock started",
}

DATA["TEXT_MODULES"] = {
        "name": "Modules",
        "type": "info"
}

#### MODULES ####

DATA["CONFIG_EGGTIMER"] = {
        "name": "Eggtimer (1480 bytes)",
        "depends": [],
        "default": False,
        "help": "Countdown timer for intervals from seconds up to 20+ hours.",
}

DATA["CONFIG_PHASE_CLOCK"] = {
        "name": "Phase Clock (918 bytes)",
        "depends": [],
        "default": False,
        "help": "Measures sleep phase by recording body movement and sending the data to the accesspoint.\n"
                "Designed to be used with uberclock",
}

DATA["CONFIG_ALTITUDE"] = {
        "name": "Altitude (1202 bytes)",
        "depends": [],
        "default": True,
        "help": "Messures altitude"
        }


DATA["CONFIG_VARIO"] = {
        "name": "Combined with alti, gives vertical speed (478 bytes)",
        "depends": [],
        "default": False}

DATA["CONFIG_ALTI_ACCUMULATOR"] = {
	"name": "Altitude accumulator (1068 bytes)",
	"depends": [],
	"default": False,
	"help": "If active take altitude measurement once per minute and accumulate all ascending vertical meters."
	}

DATA["CONFIG_PROUT"] = {
        "name": "Simple example that displays a text (238 bytes)",
        "depends": [],
        "default": False}


DATA["CONFIG_SIDEREAL"] = {
        "name": "Sidereal Time Clock (3418 bytes)",
        "depends": [],
        "default": False,
        "help": "Calculate and show local sidereal time (accurate to ~5s).\n"
                "To work properly, the current time zone (that is set on the normal clock) and longitude have to be set on the watch. The clock does real sidereal second clock ticks. When desired the sidereal time can also be set manually.\n"
                "This does NOT replace the normal clock which is still available and working."
        }

DATA["CONFIG_DST"] = {
        "name": "Daylight Saving Time",
        "depends": [],
        "default": 0,
        "type": "choices",
        "values": [(0, "No DST"), (1, "US/Canada"), (2, "Mexico"), (3, "Brazil"), (4, "EU/UK/Eastern Europe"), (5, "Australia"), (6, "New Zealand")],
        "help": "Automatically adjust Clock for Daylight Saving Time"
        }


DATA["CONFIG_INFOMEM"] = {
        "name": "Information Memory Driver (2934 bytes, requires sidereal clock)",
        "depends": [],
        "default": False,
        "help": "Build driver for usage of the Information Memory.\n"
                "COMPILATION WILL LIKELY FAIL WITH mspgcc4 <20100829 !"
        }


DATA["CONFIG_ACCEL"] = {
        "name": "Acceleration (1232 bytes)",
        "depends": [],
        "default": True,
        "help": "Acceleration applications (display and transmission). When no other application uses the acceleration sensor, it is disabled completely"
        }

DATA["CONFIG_STRENGTH"] = {
    "name": "Strength training timer (380 bytes)",
    "depends": [],
    "default": False,
    "help": "Timer for strength training studios which have you perform repetitions for 90 seconds, e.g. Kieser Training"
}

# not yet working
DATA["CONFIG_ALARM"] = {
        "name": "Alarm (608 bytes)",
        "depends": [],
        "default": True}
DATA["CONFIG_BATTERY"] = {
        "name": "Battery (360 bytes)",
        "depends": [],
        "default": True}
DATA["CONFIG_CLOCK"] = {
        "name": "Clock",
        "depends": [],
        "default": True}
DATA["CONFIG_DATE"] = {
        "name": "Date",
        "depends": [],
        "default": True}
DATA["CONFIG_RFBSL"] = {
        "name": "Wireless Update",
        "depends": [],
        "default": True}
DATA["CONFIG_STOP_WATCH"] = {
        "name": "Stop Watch (1202 bytes)",
        "depends": [],
        "default": True}
DATA["CONFIG_TEMP"] = {
        "name": "Temperature",
        "depends": [],
        "default": True}


###IMPLEMENTED BY LeanChronos. gventosa 09.10.2010
DATA["CONFIG_USEPPT"] = {
        "name": "Use PPT",
        "depends": [],
        "default": True}

DATA["CONFIG_USE_SYNC_TOSET_TIME"] = {
	"name": "Sync is the only way to set clocks data/time",
	"depends": [],
	"default": False}

DATA["CONFIG_USE_DISCRET_RFBSL"] = {
	"name": "RFBSL is hidden behind battery",
	"depends": [],
	"default": False}

DATA["CONFIG_USE_GPS"] = {
	"name": "GPS Functions enabled (doorlock)",
	"depends": [],
	"default": False}

DATA["CONFIG_CW_TIME"] = {
        "name": "CW Time",
        "depends": [],
        "default": False,
	"help": "Send time in morse code"}

HEADER = """
#ifndef _CONFIG_H_
#define _CONFIG_H_

"""

FOOTER = """
#endif // _CONFIG_H_
"""


# debugging functions
#FP = open("/tmp/log", "w")
#def flog(*args):
#    FP.write(repr(args))
#    FP.write("\n")


class HelpListWalker(urwid.SimpleListWalker):
    def __init__(self, app, *args, **kwargs):
        self.app = app
        super(HelpListWalker, self).__init__(*args, **kwargs)


    def set_focus(self, focus):
        if hasattr(self[focus], "_datafield") and \
           "help" in self[focus]._datafield:
            self.app.help_widget.set_text(self[focus]._datafield["help"])
        else:
            self.app.help_widget.set_text("")
        return super(HelpListWalker, self).set_focus(focus)

class HelpGridFlow(urwid.GridFlow):
    def __init__(self, app, *args, **kwargs):
        self.app = app
        super(HelpGridFlow, self).__init__(*args, **kwargs)


    def set_focus(self, focus):
        if hasattr(focus, "_datafield") and \
           "help" in focus._datafield:
            self.app.help_widget.set_text(focus._datafield["help"])
        else:
            self.app.help_widget.set_text("")
        return super(HelpGridFlow, self).set_focus(focus)


class OpenChronosApp(object):
    def main(self):
        self.fields = {}
        text_header = (u"OpenChronos config  \u2503  "
                       u"UP / DOWN / PAGE UP / PAGE DOWN scroll.  F8 aborts.")

        self.list_content = list_content = []

        for key,field in DATA.iteritems():
            # generate gui forms depending on type
            if field.get("type", "bool") == "bool":
                f = urwid.AttrWrap(urwid.CheckBox(field["name"], state=field["value"]),'buttn','buttnf')
                f._datafield = field
                self.fields[key] = f
                list_content.append(f)

            elif field["type"] == "choices":
                try:
                    value = field["values"].index(field["value"])
                except ValueError:
                    value = field["default"]
                field["radio_button_group"] = []
                f = urwid.Text(field["name"])
                f._datafield = field
                choice_items = [f]
                for dat in field["values"]:
                    txt = value = dat
                    if isinstance(dat, tuple):
                        value, txt = dat
                    f = urwid.AttrWrap(urwid.RadioButton(field["radio_button_group"],
                        unicode(txt), state=value==field["value"]), 'buttn','buttnf')
                    f._datafield = field
                    f.value = value
                    choice_items.append(f)
                hgf = HelpGridFlow(self, choice_items, 20, 3, 1, 'left')
                self.fields[key] = choice_items
                hgf.focus_cell = hgf.cells[1]
                list_content.append(hgf)

            elif field["type"] == "text":
                f = urwid.AttrWrap(urwid.Edit("%s: "%field["name"], field["value"]), 
                                   'editbx', 'editfc')
                f._datafield = field
                self.fields[key] = f
                list_content.append(f)

            elif field["type"] == "info":
                f = urwid.Text(field["name"])
                f._datafield = field
                self.fields[key] = f
                list_content.append(f)

        def ok_pressed(*args, **kwargs):
            raise urwid.ExitMainLoop()

        def abort_pressed(*args, **kwargs):
            sys.exit(0)

        list_content.append(urwid.Divider(div_char=u"\u2550", top=1, bottom=1))
        list_content.append(
        urwid.Padding(urwid.GridFlow(
            [urwid.AttrWrap(urwid.Button("Save", ok_pressed), 'buttn','buttnf'),
             urwid.AttrWrap(urwid.Button("Abort", abort_pressed), 'buttn','buttnf')],
            15, 4, 4, 'center'),
            ('fixed left',4), ('fixed right',3)))


        header = urwid.AttrWrap(urwid.Text(text_header), 'header')
        #header = urwid.Padding(urwid.BigText("OpenChronos", urwid.HalfBlock5x4Font()))
        walker = HelpListWalker(self, list_content)
        listbox = urwid.ListBox(walker)
        self.help_widget = urwid.Text("")
        footer = urwid.AttrWrap(self.help_widget, 'footer')
        frame = urwid.Frame(urwid.AttrWrap(listbox, 'body'), header=header, footer=footer)

        screen = urwid.raw_display.Screen()
        palette = [
            ('body','black','light gray', 'standout'),
            ('reverse','light gray','black'),
            ('header','white','dark red', 'bold'),
            ('important','dark blue','light gray',('standout','underline')),
            ('editfc','white', 'dark blue', 'bold'),
            ('editbx','light gray', 'dark blue'),
            ('editcp','black','light gray', 'standout'),
            ('bright','dark gray','light gray', ('bold','standout')),
            ('buttn','black','dark cyan'),
            ('buttnf','white','dark blue','bold'),
            ]


        def unhandled(key):
            if key == 'f8':
                #raise urwid.ExitMainLoop()
                sys.exit(0)

        urwid.MainLoop(frame, palette, screen,
            unhandled_input=unhandled).run()


    def save_config(self):
        for key,field in self.fields.iteritems():
            if isinstance(field, (tuple, list)):
                for item in field:
                    if hasattr(item, "get_state"):
                        if item.get_state():
                            # found the set radio button
                            DATA[key]["value"] = item.value
                                # look up the 
            elif isinstance(field, urwid.Text):
                pass
            elif isinstance(field, urwid.AttrMap):
                wid = field.original_widget
                if isinstance(wid, urwid.Edit):
                    DATA[key]["value"] = wid.get_edit_text()
                else:
                    DATA[key]["value"] = wid.get_state()
            else:
                raise ValueError, "Unhandled type"

        fp = open("config.h", "w")
        fp.write("// !!!! DO NOT EDIT !!!, use: make config\n")
        fp.write(HEADER)
        for key,dat in DATA.iteritems():
            if not "value" in dat:
                continue
            if "type" in dat and dat["type"] == "info":
                continue
            if DATA[key].get("ifndef", False):
                fp.write("#ifndef %s\n" %key)
            if isinstance(dat["value"], bool):
                if dat["value"]:
                    fp.write("#define %s\n" %key)
                else:
                    fp.write("// %s is not set\n" %key)
            else:
                fp.write("#define %s %s\n" %(key, dat["value"]))
            if DATA[key].get("ifndef", False):
                fp.write("#endif // %s\n" %key)
        fp.write(FOOTER)


    def load_config(self):
        def set_default():
            for key,dat in DATA.iteritems():
                #print dat
                if not "value" in dat and "default" in dat:
                    dat["value"] = dat["default"]

        try:
            fp = open("config.h")
        except (OSError, IOError):
            set_default()
            return
        match = re.compile('^[\t ]*#[\t ]*define[\t ]+([a-zA-Z0-9_]+)[\t ]*(.*)$')
        match2 = re.compile('^// ([a-zA-Z0-9_]+) is not set$')
        for line in fp:
            m = match.search(line)
            if m:
                m = m.groups()
                if not m[0] in DATA:
                    continue
                if m[1] == "":
                    DATA[m[0]]["value"] = True
                else:
                    try:
                        value = int(m[1])
                    except ValueError:
                        value = m[1]
                    DATA[m[0]]["value"] = value
            else:
                m = match2.search(line)
                if m and m.groups()[0] in DATA:
                    m = m.groups()
                    DATA[m[0]]["value"] = False

        set_default()

if __name__ == "__main__":
    App = OpenChronosApp()
    App.load_config()
    App.main()
    App.save_config()

