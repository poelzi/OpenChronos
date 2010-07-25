#!/usr/bin/env python
# encoding: utf-8

import npyscreen
import re, sys, random
#npyscreen.disableColor()
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
        "values": [902, 869, 433]}

DATA["CONFIG_METRIC_ONLY"] = {
        "name": "Metric only code",
        "depends": [],
        "default": False,
        "help": "Only add code for Metric units (24 hours/meter/celsius) to reduce image size",
}

DATA["THIS_DEVICE_ADDRESS"] = {
        "name": "Hardware address",
        "type": "text",
        "default": rand_hw(),
        "ifndef": True,
        "help": "Default Radio Hardware Address to use on the device",
}


DATA["USE_LCD_CHARGE_PUMP"] = {
        "name": "Use LCD Charge Pump",
        "default": False,
        "help": "Use the internal charge pump to make the display contrast same tho whole battery lifetime. But this increases currency and reduces battery lifetime.",
}

DATA["USE_WATCHDOG"] = {
        "name": "Use Watchdog",
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
        "name": "Date: Day of Week",
        "depends": [],
        "default": True,
        "help": "Shows day of week on the date module",
}

DATA["CONFIG_TEST"] = {
        "name": "Test Mode",
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
        "name": "Eggtimer",
        "depends": [],
        "default": False,
        "help": "Countdown timer to count down from 1 minute - 20 hours to 0 and start an alarm",
}

DATA["CONFIG_PHASE_CLOCK"] = {
        "name": "Phase Clock",
        "depends": [],
        "default": False,
        "help": "Messures sleep phase by recording body movement and sends the data to the accesspoint.\n"
                "Designed to be used with uberclock",
}

DATA["CONFIG_ALTITUDE"] = {
        "name": "Altitude",
        "depends": [],
        "default": True,
        "help": "Messures altitude"
        }

# not yet working

DATA["CONFIG_ACCEL"] = {
        "name": "Accleration",
        "depends": [],
        "default": True}
DATA["CONFIG_ALARM"] = {
        "name": "Alarm",
        "depends": [],
        "default": True}
DATA["CONFIG_BATTERY"] = {
        "name": "Battery",
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
        "name": "Stop Watch",
        "depends": [],
        "default": True}
DATA["CONFIG_TEMP"] = {
        "name": "Temperature",
        "depends": [],
        "default": True}



HEADER = """
#ifndef _CONFIG_H_
#define _CONFIG_H_

"""

FOOTER = """
#endif // _CONFIG_H_
"""


#load_config()

#print DATA
#sys.exit(1)

import curses, weakref

def wrap(text, width):
    """
    A word-wrap function that preserves existing line breaks
    and most spaces in the text. Expects that existing line
    breaks are posix newlines (\n).
    """
    return reduce(lambda line, word, width=width: '%s%s%s' %
                  (line,
                   ' \n'[(len(line)-line.rfind('\n')-1
                         + len(word.split('\n',1)[0]
                              ) >= width)],
                   word),
                  text.split(' ')
                 )

class ConfigForm(npyscreen.TitleForm):
    BLANK_LINES_BASE     = 0
    BLANK_COLUMNS_RIGHT  = 0
    DEFAULT_X_OFFSET = 2
    FRAMED = False
    #MAIN_WIDGET_CLASS   = wgmultiline.MultiLine
    STATUS_WIDGET_CLASS = npyscreen.Textfield
    COMMAND_WIDGET_CLASS= npyscreen.Textfield
    #MAIN_WIDGET_CLASS = grid.SimpleGrid
    #MAIN_WIDGET_CLASS = editmultiline.MultiLineEdit
    def __init__(self, *args, **keywords):
        super(ConfigForm, self).__init__(cycle_widgets=False, *args, **keywords)
    
    
    def draw_form(self):
        MAXY, MAXX = self.lines, self.columns #self.curses_pad.getmaxyx()
        self.curses_pad.hline(0, 0, curses.ACS_HLINE, MAXX-1)  
        self.curses_pad.hline(MAXY-2, 0, curses.ACS_HLINE, MAXX-1)  

    def create(self):
        MAXY, MAXX    = self.lines, self.columns
        self.wStatus1 = self.add(self.__class__.STATUS_WIDGET_CLASS,  rely=0, relx=0,      editable=False,  )
        self.wStatus1.value = "Config Chronos Firmware"
        #self.wMain    = self.add(npyscreen.SimpleGrid,    rely=1,  relx=0,     max_height = -2, )
        #self.wMain    = self.add(self.__class__.MAIN_WIDGET_CLASS,    rely=1,  relx=0,     max_height = -2, )
        self.wStatus2 = self.add(self.__class__.STATUS_WIDGET_CLASS,  rely=MAXY-6, relx=0, max_heigh=3, heigth=3, editable=False,  )
        self.wStatus2.value = ""
        #self.wCommand = self.add(self.__class__.COMMAND_WIDGET_CLASS, rely = MAXY-1, relx=0,)
        self.wStatus1.important = True
        self.wStatus2.important = False
        self.nextrely = 2

    def set_help(self):
        if hasattr(self._widgets__[self.editw], "_datafield") and \
           "help" in self._widgets__[self.editw]._datafield:
            #print wrap(self._widgets__[self.editw]._datafield["help"], self.columns-40)
            val = wrap(self._widgets__[self.editw]._datafield["help"], self.columns-5)+"\n\n\n"
        else:
            val = ""
        self.wStatus2.value = val + "\n"*(2-val.count("\n"))
        
        self.wStatus2.display()

    def while_editing(self, *args, **kwargs):
        self.set_help()
        super(ConfigForm, self).while_editing(*args, **kwargs)


class OpenChronosApp(npyscreen.NPSApp):
    def main(self):
        self.fields = {}
        # These lines create the form and populate it with widgets.
        # A fairly complex screen in only 8 or so lines of code - a line for each control.
        F = ConfigForm(name = "Config Chronos Firmware",)

#        ms2= F.add(npyscreen.TitleMultiSelect, max_height =-2, value = [1,], name="Frequency", 
#                   values = ["911","868","Option3"], scroll_exit=True)
#        ms2= F.add(npyscreen.TitleMultiSelect, max_height =-2, value = [1,], name="Pick Several", 
#                    values = ["Option1","Option2","Option3"], scroll_exit=True)
 
    #		fn = F.add(npyscreen.TitleFilename, name = "Filename:")
    # 		dt = F.add(npyscreen.TitleDateCombo, name = "Date:")
    # 		s = F.add(npyscreen.TitleSlider, out_of=12, name = "Slider")
    # 		ml= F.add(npyscreen.MultiLineEdit, 
    # 			value = """try typing here!\nMutiline text, press ^R to reformat.\n""", 
    # 			max_height=5, rely=9)
    # 		ms2= F.add(npyscreen.TitleMultiSelect, max_height =-2, value = [1,], name="Pick Several", 
    # 				values = ["Option1","Option2","Option3"], scroll_exit=True)
        #t = F.add(npyscreen.TitleText, name = "Modules:",)

        for key,field in DATA.iteritems():
            if field.get("type", "bool") == "bool":
                #f=F.add(npyscreen.TitleMultiSelect, max_height = 1, value = ["ENABLED",], name=field["name"], 
                #	values = [""], scroll_exit=True)
                f = F.add(npyscreen.Checkbox, name=field["name"], value=field["value"])
            elif field["type"] == "choices":
                try:
                    value = field["values"].index(field["value"])
                except ValueError:
                    value = field["values"].index(field["default"])
                f = F.add(npyscreen.TitleSelectOne, max_height=4, value=value, name=field["name"], 
                        values = field["values"], scroll_exit=True)
            elif field["type"] == "text":
                f = F.add(npyscreen.TitleText, max_height=1, value=field["value"], name=field["name"], 
                        values = field["value"])
            elif field["type"] == "info":
                f = F.add(npyscreen.TitleText, max_height=1, name=field["name"])
            f._key = key
            f._datafield = field
            self.fields[key] = f


        # This lets the user play with the Form.
        F.edit()

    def save_config(self):
        for key,field in self.fields.iteritems():
            value = field.value
            if hasattr(field, "values"):
                 value = field.values[value[0]]
            DATA[key]["value"] = value

        fp = open("config.h", "w")
        fp.write("// !!!! DO NOT EDIT !!!, use: make config\n")
        fp.write(HEADER)
        for key,dat in DATA.iteritems():
            if not "value" in dat:
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

        #print DATA
        #sys.exit(0)

        set_default()

if __name__ == "__main__":
    App = OpenChronosApp()
    App.load_config()
    App.run()
    App.save_config()

