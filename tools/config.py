#!/usr/bin/env python
# encoding: utf-8

import npyscreen
import re, sys
#npyscreen.disableColor()

DATA = {
"CONFIG_FREQUENCY": {
        "name": "Frequency",
        "depends": [],
        "default": 911,
        "type": "choices",
        "values": [911, 868, 433]},

"CONFIG_METRIC_ONLY": {
        "name": "Metric only code (saves space)",
        "depends": [],
        "default": False
},

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
                f = F.add(npyscreen.TitleSelectOne, max_height=4, value=value, name="Frequency", 
                        values = field["values"], scroll_exit=True)
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
        for key,dat in DATA.iteritems():
            if isinstance(dat["value"], bool):
                if dat["value"]:
                    fp.write("#define %s\n" %key)
                else:
                    fp.write("// %s is not set\n" %key)
            else:
                fp.write("#define %s %s\n" %(key, dat["value"]))


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

