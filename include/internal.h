/*MIT License

Copyright (c) 2020 Anup

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#define CMD_GET_MAINS_VOLTAGE       0
#define CMD_GET_FW_VERSION          1
#define CMD_GET_HOLIDAY_MODE_STATUS 2 //boolean
#define CMD_SET_HOLIDAY_MODE_ON     3
#define CMD_SET_HOLIDAY_MODE_OFF    4
#define CMD_GET_INVERTER_MODE       5 // 0 - Normal Mode 1 - UPS
#define CMD_SET_POWER_MODE_UPS      6
#define CMD_SET_POWER_MODE_NORMAL   7
#define CMD_GET_REGULATOR_LEVEL     8
#define CMD_SET_REGULATOR_LEVEL0    9  //220v <-- Not to be used
#define CMD_SET_REGULATOR_LEVEL1    10 //180v
#define CMD_SET_REGULATOR_LEVEL2    11 //190v
#define CMD_SET_REGULATOR_LEVEL3    12 //200v
#define CMD_SET_REGULATOR_LEVEL4    13 //210v
#define CMD_SET_REGULATOR_LEVEL5    14 //220v
#define CMD_SET_REGULATOR_LEVEL6    15 //230v
#define CMD_SET_REGULATOR_LEVEL7    16 //240v <-- Never go here (Forbidden)
#define CMD_GET_INV_SWITCH_STATUS   17 //boolean
#define CMD_SET_INV_SWITCH_ON       18
#define CMD_SET_INV_SWITCH_OFF      19
#define CMD_GET_BATTERY_PERCENT     20
#define CMD_GET_BATTERY_REMAINING   21
#define CMD_GET_BATTERY_TYPE        22 //1 - Tubular
#define CMD_GET_BATTERY_VOLTAGE     23
#define CMD_GET_SELECTED_BATTERY    24
#define CMD_GET_TURBO_CHG_STATUS    25
#define CMD_SET_TURBO_CHG_ON        26
#define CMD_SET_TURBO_CHG_OFF       27
#define CMD_GET_CHG_CURRENT         28 // div 10A
#define CMD_GET_CHG_MODE            29
#define CMD_SET_CLEAR_FAULTS        30
#define CMD_GET_SYS_TEMPERATURE     31 //deg celcius
#define CMD_GET_LOAD_PERCENT        32 //%age
#define CMD_GET_LOAD_CURRENT        33 // div 10 A
#define CMD_GET_CUTOFF_TIME         34 //Time remaining to get out of a forced cutoff
#define CMD_SET_CUTOFF_TIME_0M      35 //CUTOFF DISABLE (reset ongoing cutoff timer) Only when needs to be woke up
#define CMD_SET_CUTOFF_TIME_30M     36
#define CMD_SET_CUTOFF_TIME_60M     37
#define CMD_SET_CUTOFF_TIME_120M    38
#define CMD_GET_CUTOFF_STATUS       39
#define CMD_SET_CUTOFF_ON           40
#define CMD_SET_CUTOFF_OFF          41 //No USE. Just call 35 to disable the cut off
#define CMD_GET_APPL_MODE_STATUS    42 //boolean
#define CMD_SET_APPL_MODE_ON        43
#define CMD_SET_APPL_MODE_OFF       44
#define CMD_GET_BATTERY_COUNT       45
#define CMD_GET_BACKUP_MODE_STATUS  46
#define CMD_GET_ALARM_FLAGS         47
#define CMD_MAX                     48

const byte invCmd[][8] {
                            {0xFF, 0xFF, 0xFF, 0x08, 0x0C, 0x01, 0xFF, 0xFF}, //Get mains voltage
                            {0xFF, 0xFF, 0xFF, 0xCA, 0x0B, 0x01, 0xFF, 0xFF}, //Get Firmware version
                            {0xFF, 0xFF, 0xFF, 0x32, 0x0C, 0x01, 0xFF, 0xFF}, //Holiday Mode - Status
                            {0xFF, 0x01, 0x00, 0x32, 0x0C, 0x00, 0xFF, 0xFF}, //Holiday Mode - ON
                            {0xFF, 0x00, 0x00, 0x32, 0x0C, 0x00, 0xFF, 0xFF}, //Holiday Mode - OFF
                            {0xFF, 0xFF, 0xFF, 0x18, 0x0C, 0x01, 0xFF, 0xFF}, //Get Output Mode
                            {0xFF, 0x01, 0x00, 0x18, 0x0C, 0x00, 0xFF, 0xFF}, //UPS MODE
                            {0xFF, 0x00, 0x00, 0x18, 0x0C, 0x00, 0xFF, 0xFF}, //NORMAL MODE
                            {0xFF, 0xFF, 0xFF, 0x24, 0x0C, 0x01, 0xFF, 0xFF}, //Voltage Regulator Level Status
                            {0xFF, 0x00, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 0
                            {0xFF, 0x01, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 1
                            {0xFF, 0x02, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 2
                            {0xFF, 0x03, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 3
                            {0xFF, 0x04, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 4
                            {0xFF, 0x05, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 5
                            {0xFF, 0x06, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 6
                            {0xFF, 0x07, 0x00, 0x24, 0x0C, 0x00, 0xFF, 0xFF}, //Level 7
                            {0xFF, 0xFF, 0xFF, 0x16, 0x0C, 0x01, 0xFF, 0xFF}, //Inverter Switch Status
                            {0xFF, 0x01, 0x00, 0x16, 0x0C, 0x00, 0xFF, 0xFF}, //Inverter Switch ON
                            {0xFF, 0x00, 0x00, 0x16, 0x0C, 0x00, 0xFF, 0xFF}, //Inverter Switch OFF
                            {0xFF, 0xFF, 0xFF, 0x3C, 0x0C, 0x01, 0xFF, 0xFF}, //Get Battery Percentage
                            {0xFF, 0xFF, 0xFF, 0x38, 0x0C, 0x01, 0xFF, 0xFF}, //Get Battery Remaining
                            {0xFF, 0xFF, 0xFF, 0x1C, 0x0C, 0x01, 0xFF, 0xFF}, //Get Battery Type
                            {0xFF, 0xFF, 0xFF, 0x06, 0x0C, 0x01, 0xFF, 0xFF}, //Get Battery Voltage
                            {0xFF, 0x00, 0x00, 0xCC, 0x0B, 0x01, 0xFF, 0xFF}, //Get Selected Battery
                            {0xFF, 0xFF, 0xFF, 0x30, 0x0C, 0x01, 0xFF, 0xFF}, //Get Turbo Charging Status
                            {0xFF, 0x01, 0x00, 0x30, 0x0C, 0x00, 0xFF, 0xFF}, //Turbo Charging ON
                            {0xFF, 0x00, 0x00, 0x30, 0x0C, 0x00, 0xFF, 0xFF}, //Turbo Charging OFF
                            {0xFF, 0xFF, 0xFF, 0x10, 0x0C, 0x01, 0xFF, 0xFF}, //Get Charging Current
                            {0xFF, 0xFF, 0xFF, 0x1A, 0x0C, 0x01, 0xFF, 0xFF}, //Get Charging Mode (Based on the switch)
                            {0xFF, 0x01, 0x00, 0x6C, 0x0C, 0x00, 0xFF, 0xFF}, //Clear All Faults
                            {0xFF, 0xFF, 0xFF, 0x0E, 0x0C, 0x01, 0xFF, 0xFF}, //Get System Temperature
                            {0xFF, 0xFF, 0xFF, 0x2C, 0x0C, 0x01, 0xFF, 0xFF}, //Get Load Percentage
                            {0xFF, 0xFF, 0xFF, 0x0C, 0x0C, 0x01, 0xFF, 0xFF}, //Get Load Current
                            {0xFF, 0xFF, 0xFF, 0x2A, 0x0C, 0x01, 0xFF, 0xFF}, //Get CUTOFF Time
                            {0xFF, 0x00, 0x00, 0x2A, 0x0C, 0x00, 0xFF, 0xFF}, //CUTOFF Time 0 (now)
                            {0xFF, 0x1E, 0x00, 0x2A, 0x0C, 0x00, 0xFF, 0xFF}, //CUTOFF Time 30min
                            {0xFF, 0x3C, 0x00, 0x2A, 0x0C, 0x00, 0xFF, 0xFF}, //CUTOFF Time 60min
                            {0xFF, 0x78, 0x00, 0x2A, 0x0C, 0x00, 0xFF, 0xFF}, //CUTOFF Time 120min
                            {0xFF, 0xFF, 0xFF, 0x28, 0x0C, 0x01, 0xFF, 0xFF}, //Forced CUTOFF Status
                            {0xFF, 0x01, 0x00, 0x28, 0x0C, 0x00, 0xFF, 0xFF}, //Forced CUTOFF ON
                            {0xFF, 0x00, 0x00, 0x28, 0x0C, 0x00, 0xFF, 0xFF}, //Forced CUTOFF OFF
                            {0xFF, 0xFF, 0xFF, 0x26, 0x0C, 0x01, 0xFF, 0xFF}, //Get Appliance Mode Status
                            {0xFF, 0x01, 0x00, 0x26, 0x0C, 0x00, 0xFF, 0xFF}, //Appliance Mode ON
                            {0xFF, 0x00, 0x00, 0x26, 0x0C, 0x00, 0xFF, 0xFF}, //Appliance Mode OFF
                            {0xFF, 0xFF, 0xFF, 0x74, 0x0C, 0x01, 0xFF, 0xFF}, //Battery Count
                            {0xFF, 0xFF, 0xFF, 0x1E, 0x0C, 0x01, 0xFF, 0xFF}, //Backup Mode Status
                            {0xFF, 0xFF, 0xFF, 0x1E, 0x0C, 0x01, 0xFF, 0xFF}, //Alarm Flags
                            };