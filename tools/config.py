#!/usr/bin/env python
# encoding: utf-8

import npyscreen
import re, sys, random
#npyscreen.disableColor()

# {0x79, 0x56, 0x34, 0x12}
def rand_hw():
    res = []
    for i in range(4):
        res.append(random.randint(1, 254))
    #res.insert(0, random.randint(1, 254))
    res.sort(reverse=True)
    return "{" + ",".join([hex(x) for x in res]) + "}"


DATA = {
"CONFIG_FREQUENCY": {
        "name": "Frequency",
        "depends": [],
        "default": 902,
        "type": "choices",
        "values": [902, 869, 433]},

"CONFIG_METRIC_ONLY": {
        "name": "Metric only code (saves space)",
        "depends": [],
        "default": False
},

"THIS_DEVICE_ADDRESS": {
        "name": "Hardware address",
        "type": "text",
        "default": rand_hw()
},

"DEBUG": {
        "name": "Debug",
        "default": False},

# modules

"CONFIG_ACCEL": {
        "name": "Accleration",
        "depends": [],
        "default": True},
"CONFIG_ALARM": {
        "name": "Alarm",
        "depends": [],
        "default": True},
"CONFIG_ALTITUDE": {
        "name": "Altitude",
        "depends": [],
        "default": True},
"CONFIG_BATTERY": {
        "name": "Battery",
        "depends": [],
        "default": True},
"CONFIG_CLOCK": {
        "name": "Clock",
        "depends": [],
        "default": True},
"CONFIG_DATE": {
        "name": "Date",
        "depends": [],
        "default": True},
"CONFIG_DAY_OF_WEEK": {
        "name": "Date: Day of Week",
        "depends": [],
        "default": True},
"CONFIG_PHASE_CLOCK": {
        "name": "Phase Clock",
        "depends": [],
        "default": False},
"CONFIG_RFBSL": {
        "name": "Wireless Update",
        "depends": [],
        "default": True},
"CONFIG_STOP_WATCH": {
        "name": "Stop Watch",
        "depends": [],
        "default": True},
"CONFIG_STOP_TEMP": {
        "name": "Temperature",
        "depends": [],
        "default": True},
}


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

class OpenChronosApp(npyscreen.NPSApp):
    def main(self):
        self.fields = {}
        # These lines create the form and populate it with widgets.
        # A fairly complex screen in only 8 or so lines of code - a line for each control.
        F = npyscreen.Form(name = "Config Chronos Firmware",)

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
                f._key = key
                self.fields[key] = f
            elif field["type"] == "choices":
                try:
                    value = field["values"].index(field["value"])
                except ValueError:
                    value = field["values"].index(field["default"])
                f = F.add(npyscreen.TitleSelectOne, max_height=4, value=value, name=field["name"], 
                        values = field["values"], scroll_exit=True)
                f._key = key
                self.fields[key] = f
            elif field["type"] == "text":
                f = F.add(npyscreen.TitleText, max_height=1, value=field["value"], name=field["name"], 
                        values = field["value"])
                f._key = key
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
            if isinstance(dat["value"], bool):
                if dat["value"]:
                    fp.write("#define %s\n" %key)
                else:
                    fp.write("// %s is not set\n" %key)
            else:
                fp.write("#define %s %s\n" %(key, dat["value"]))
        fp.write(FOOTER)


    def load_config(self):
        def set_default():
            for key,dat in DATA.iteritems():
                #print dat
                if not "value" in dat:
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
                if m:
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

