/*
MIT License

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

#include "Arduino.h"
#include "dups.h"
#include "internal.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define REFRESH_INTERVAL    4000

Inverter::Inverter(){
    isConnected = false;
    pClient = NULL ;
    myDevice = NULL ;
    lastUpdated = 0 ;
    respBuf = new byte[10];
    isCharging = true ;
    isDischarging = true;
    backupSwitchTS = 0;
    alarmData.batteryCharged = false;
    alarmData.feedbackFail = 0;
    alarmData.firstCycleNotCompleted = true;
    alarmData.fuseBlown = false;
    alarmData.lowBattery = 0;
    alarmData.lowBatteryCount = 0;
    alarmData.overloadCount = 0;
    alarmData.overTemperature = 0;
    alarmData.shortCircuit = 0;
}

Inverter::~Inverter(){
    if (NULL != respBuf)
        delete respBuf;
}

bool Inverter::attach (){
    Serial.println("Inverter::attach");
    BLEDevice::init("My Phone");
    //BLEScan* pBLEScan = BLEDevice::getScan();
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(this);
    init();
    //pBLEScan->setAdvertisedDeviceCallbacks(this);
    //pBLEScan->setInterval(1349);
    //pBLEScan->setWindow(449);
    //pBLEScan->setActiveScan(true);
    //pBLEScan->start(5, false);
    return true ;
}

bool Inverter::detach (){
    Serial.println("Inverter::detach");
    return true;
}

//BLE Client Callback
void Inverter::onConnect(BLEClient* pclient) {
    Serial.println("Inverter::onConnect");
    isConnected = true;
}

void Inverter::onDisconnect(BLEClient* pclient) {
    Serial.println("Inverter::onDisconnect");
    isConnected = false;
}

//Advertisement Callaback
void Inverter::onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.println(advertisedDevice.toString().c_str());
    if(advertisedDevice.getName() == "VG_SMART_BT"){
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        isConnected = false;
    }
}

bool Inverter::init(){
    //Connect if not connected
    if (!isConnected){
        if (false == pClient->connect (BLEAddress(deviceAddr)))
            return false;
        BLERemoteService* pRemoteService = pClient->getService(BLEUUID("0003cdd0-0000-1000-8000-00805f9b0131"));
        if (pRemoteService == nullptr) {
            Serial.print("Failed to find our service UUID: ");
            //Serial.println(serviceUUID.toString().c_str());
            pClient->disconnect();
            return false;
        }
        //Serial.println(" - Found our service");

        /*Enable Read Characteristcs*/
        pRemoteRXCharacteristic = pRemoteService->getCharacteristic(BLEUUID("0003cdd1-0000-1000-8000-00805f9b0131"));
        if (pRemoteRXCharacteristic == nullptr) {
            Serial.print("Failed to find our characteristic UUID: ");
            pClient->disconnect();
            return false;
        }
        
        if(pRemoteRXCharacteristic->canRead()) {
            std::string value = pRemoteRXCharacteristic->readValue();
        }
        
        if(pRemoteRXCharacteristic->canNotify()||pRemoteRXCharacteristic->canIndicate()){
            const uint8_t notifyOn[] = {0x1,0x0};
            //const uint8_t notifyOff[] = {0x0,0x0};
            pRemoteRXCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notifyOn,2,true);
            delay(200);
            pRemoteRXCharacteristic->registerForNotify(this);
            delay(400);
        }

        //Setup TX Characteristcs
        pRemoteTXCharacteristic = pRemoteService->getCharacteristic(BLEUUID("0003cdd2-0000-1000-8000-00805f9b0131"));
        if (pRemoteTXCharacteristic == nullptr) {
            Serial.print("Failed to find our characteristic UUID: ");
            pClient->disconnect();
            return false;
        }
    }
    return true;
}

bool Inverter::sendCommand (int cmd, bool log, uint32_t tout, size_t length){
    if (cmd > CMD_MAX)
        return false;
    memset (respBuf, 0, 10);
    byte *byteArray = (byte *)invCmd[cmd];
    //Serial.printf ("\r\nWriting data of length[%d]\r\n", length) ;
    //hexDump(byteArray, length) ;
    semaphoreCmdWait.take ("CommandResponseWait");
    pRemoteTXCharacteristic->writeValue(byteArray, length) ;
    if (false == semaphoreCmdWait.timedWait ("CommandResponseWait", tout)){
        Serial.println ("semaphore didn't yield on time");
        return false ;
    }else{
        //Serial.println ("semaphore yielded on time");
        if (log){
            Serial.print("Notify data: ");
            hexDump (respBuf, 8) ; 
        }
        return true ;
    }
}

void Inverter::onNotify(
    BLERemoteCharacteristic* pBLERemoteCharacteristic,
    uint8_t* pData,
    size_t length,
    bool isNotify) {
    //Serial.println("Notify callback");
    if (pBLERemoteCharacteristic->getUUID().equals(BLEUUID("0003cdd1-0000-1000-8000-00805f9b0131"))){
        if (isNotify){
            if (NULL != pData && length > 0){
                memcpy (respBuf, pData, MIN(10, length));
                semaphoreCmdWait.give();
            }
        }
    }
}

void Inverter::hexDump (uint8_t *data, size_t length){
    if (NULL == data || length == 0)
        return ;
    for (int i = 0; i<length; i++)
        Serial.printf("0x%02x ", data[i]) ;
    Serial.println() ;
}

bool Inverter::refresh(bool full){
    if (!isConnected)
        init();

    if (0 == lastUpdated || (millis() - lastUpdated) >= REFRESH_INTERVAL){
        //Serial.println();
        //Serial.println("Refreshing...");
        readBackupModeStatus ();
        readAlarmData ();
        readMainsVoltage ();
        readBatteryVoltage ();
        readSystemTemperature ();
        
        if (m_backupMode /*0 == m_mainsVoltage*/){//parameters to be read when on backup
            if (backupSwitchTS == 0){//Record power failure time
                backupSwitchTS = millis()/1000; //In seconds
            }
            m_backupSince = (millis()/1000)-backupSwitchTS;
            isDischarging = true ;
            readLoadPercentage ();
            readLoadCurrent ();
            calculateBackupTime ();
            if (m_forcedCutOff){
                readTimeToResume ();
            }
            
        }else{
            m_loadCurrent = 0;
            m_loadPercent = 0;
            backupSwitchTS = 0;
            //m_backupSince = 0;
            isCharging = true ;
            readChargingCurrent ();
        }

        if (m_applianceMode){
            readApplianceModeStatus ();
        }

        if (0 == lastUpdated || full){//Status Params that wont change until initiated by this module
            readBatteryType ();
            readBatteryCount ();
            readHolidayMode ();
            readInverterMode ();
            readRegulatorLevel ();
            readInverterSwitchStatus ();
            readTurboChargeStatus ();
            readCutoffStatus ();
            readApplianceModeStatus ();
        }
        calculateBatteryPercent ();
        //Serial.println ("\r\n");
        lastUpdated = millis();
        return true; 
    }
    return false;
}

void Inverter::resetIHealValues(){
    alarmData.feedbackFail = 0;
    alarmData.overTemperature = 0;
    alarmData.overloadCount = 0;
    alarmData.shortCircuit = 0;
}

void Inverter::calculateBatteryPercent(){
    float f3;
    int i = m_batteryCount;
    float f1 = m_batteryVoltage;

    float f2 = i;
    if (1 == m_batteryType){
      f3 = BATTERY_FULL_VOLTAGE_TUBULAR;
    } else {
      f3 = BATTERY_FULL_VOLTAGE_FLAT;
    } 
    float batteryLowChargeFactor = f3 * f2;
    float batteryLowDischargeFactor = BATTERY_EMPTY_VOLTAGE * i;
    float batteryFactor = (i * 14);
   
    //Serial.printf ("battery battype[%d] cnt[%d] discharging [%d] charging[%d]\r\n", m_batteryType, m_batteryCount, isDischarging, isCharging);
    //Serial.printf ("batVolt[%.2f] lochfact[%.2f] batFact[%.2f] f3[%.2f] f2[%.2f]\r\n", m_batteryVoltage, batteryLowChargeFactor, batteryFactor, f3, f2);
    if (!m_backupMode) {
        isDischarging = false;
        if (alarmData.batteryCharged){
            m_chargingPercent = 100.0F;
            //Serial.printf ("1. [%d]", m_chargingPercent);
            return; 
        }
        if (f1 < batteryLowChargeFactor)
            f1 = batteryLowChargeFactor; 
        int n = (int)(100.0F * ((f1 - batteryLowChargeFactor) / (batteryFactor - (f2 * BATTERY_NOMINAL_VOLTAGE))));
        if (isCharging) {
            int i2 = getMaskedBattPercentage(n);
            m_chargingPercent = i2;
            dischargingPer = i2;
            //Serial.printf ("2. [%d]", m_chargingPercent);
            return;
        } 
        if (n > m_chargingPercent) {
            int i2 = getMaskedBattPercentage(n);
            m_chargingPercent = i2;
            dischargingPer = i2;
            //Serial.printf ("3. [%d]", m_chargingPercent);
            return;
        } 
        int i1 = chargingPer;
        dischargingPer = i1;
        m_chargingPercent = i1;
        //Serial.printf ("4. [%d]", m_chargingPercent);
        return;
    } 
    isCharging = false;
    if (f1 < batteryLowDischargeFactor)
        f1 = batteryLowDischargeFactor; 
    int j = m_loadPercent;
    int k = (int)(100.0F * ((f1 - batteryLowDischargeFactor) / (batteryFactor - 0.007F * (float)j - ((float)i * BATTERY_NOMINAL_VOLTAGE))));
    if (isDischarging) {
        int n = getMaskedBattPercentage(k);
        chargingPer = n;
        m_dischargingPercent = n;
        //Serial.printf ("5. [%d]", m_dischargingPercent);
        return ;
    } 
    if (k < dischargingPer) {
        int n = getMaskedBattPercentage(k);
        chargingPer = n;
        m_dischargingPercent = n;
        //Serial.printf ("6. [%d]", m_dischargingPercent);
        return;
    } 
    int m = dischargingPer;
    chargingPer = m;
    m_dischargingPercent = m;
    //Serial.printf ("7. [%d]", m_dischargingPercent);
    return ;
}

int Inverter::getMaskedBattPercentage (int percent) {
    return (percent < 0) ? 0 : ((percent > 100) ? 100 : percent);
}

void Inverter::calculateBackupTime(){
    float f6;
    int i = BATTERY_CAPACITY_AH;
    int j = m_batteryCount;
    float f1 = 1.0F * i;
    float f2 = 15.0F * j;
    float f3 = BATTERY_BACKUP_CALC_THRESHOLD * j;
    float f4 = 10.0F * j;
    float f5 = m_batteryVoltage;
    if (f5 < BATTERY_BACKUP_CALC_THRESHOLD)
      f1 = i - i / f2 * f4 * (f3 - f5); 
    int k = m_loadCurrent;
    if (k < 1) {
      f6 = f1 * 60.0F;
    } else {
      f6 = 60.0F * f1 / k;
    } 
    if (f6 < 0.0F)
      f6 = 0.0F; 
    //int m = (int)f6 / 60;
    //Serial.printf ("Backup Timer mins[%2.3f] [%d]hr [%d]min\r\n", f6, m, (int)(f6 - (m * 60)) );
    m_backupTime = f6;
    return ;
}

bool Inverter::readMainsVoltage(){
    bool ret = sendCommand (CMD_GET_MAINS_VOLTAGE) ;
    if (ret){
        m_mainsVoltage = (float)(((float)*((short*)(respBuf+6)))/10.0f);
    }
    return ret;
}

bool Inverter::readBatteryVoltage(){
    bool ret = sendCommand (CMD_GET_BATTERY_VOLTAGE) ;
    if (ret){
        m_batteryVoltage = (float)(((float)*((short*)(respBuf+6)))/100.0f);
    }
    return ret;
}

bool Inverter::readSystemTemperature(){
    bool ret = sendCommand (CMD_GET_SYS_TEMPERATURE) ;
    if (ret){
        m_sysTemperature = (float)(((float)*((short*)(respBuf+6)))/10.0f);
        m_sysTemperature = (m_sysTemperature-32.0F) * (5.0F/9.0F) ;
    }
    return ret;
}

bool Inverter::readBatteryCount(){
    bool ret = sendCommand (CMD_GET_BATTERY_COUNT) ;
    if (ret){
        m_batteryCount = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readLoadCurrent(){
    bool ret = sendCommand (CMD_GET_LOAD_CURRENT) ;
    if (ret){
        m_loadCurrent = (float)(((float)*((short*)(respBuf+6)))/10.0f);
    }
    return ret;
}

bool Inverter::readHolidayMode(){
    bool ret = sendCommand (CMD_GET_HOLIDAY_MODE_STATUS) ;
    if (ret){
        m_hoildayMode = ((bool)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readInverterMode(){
    bool ret = sendCommand (CMD_GET_INVERTER_MODE) ;
    if (ret){
        m_UPSMode = ((bool)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readRegulatorLevel(){
    bool ret = sendCommand (CMD_GET_REGULATOR_LEVEL) ;
    if (ret){
        m_regulatorLevel = ((byte)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readInverterSwitchStatus(){
    bool ret = sendCommand (CMD_GET_INV_SWITCH_STATUS) ;
    if (ret){
        m_inverterSwitch = ((bool)*((short*)(respBuf+6)));
    }
    return ret;
}

/*bool Inverter::readBatteryPercentage(){
    bool ret = sendCommand (CMD_GET_BATTERY_PERCENT) ;
    if (ret){
        //(int)(100.0F * paramBundle.getInt("BATTERY_PERCENTAGE") / 180.0F * this.mInverterHomeFragment.batteryCapacity);
        m_batteryPercent = ((float)*((short*)(respBuf+6)));
        m_batteryPercent = (100.0F * m_batteryPercent)/27000.0F ;
    }
    return ret;
}*/

bool Inverter::readTurboChargeStatus(){
    bool ret = sendCommand (CMD_GET_TURBO_CHG_STATUS) ;
    if (ret){
        m_turboCharge = ((bool)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readChargingCurrent(){
    bool ret = sendCommand (CMD_GET_CHG_CURRENT) ;
    if (ret){
        m_chargingCurrent = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}
bool Inverter::readLoadPercentage(){
    bool ret = sendCommand (CMD_GET_LOAD_PERCENT) ;
    if (ret){
        m_loadPercent = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}
bool Inverter::readCutoffStatus(){
    bool ret = sendCommand (CMD_GET_CUTOFF_STATUS) ;
    if (ret){
        m_forcedCutOff = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readTimeToResume(){
    bool ret = sendCommand (CMD_GET_CUTOFF_TIME) ;
    if (ret){
        m_timeToResume = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readApplianceModeStatus(){
    bool ret = sendCommand (CMD_GET_APPL_MODE_STATUS) ;
    if (ret){
        m_applianceMode = ((bool)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readBatteryType(){
    bool ret = sendCommand (CMD_GET_BATTERY_TYPE) ;
    if (ret){
        m_batteryType = ((byte)*((short*)(respBuf+6)));
    }
    return ret;
}

bool Inverter::readBackupModeStatus(){
    bool ret = sendCommand (CMD_GET_BACKUP_MODE_STATUS) ;
    if (ret){
        m_backupMode = ((0x08 & ((short)*((short*)(respBuf+6)))) != 0x08);
    }
    return ret;
}

bool Inverter::readAlarmData(){
    bool ret = sendCommand (CMD_GET_BACKUP_MODE_STATUS) ;
    short alarmFlag ;
    if (ret){
        alarmFlag = ((short)*((short*)(respBuf+6)));
        if ((alarmFlag & 0x1) == 1) {
            alarmData.overloadCount = 1;
            alarmData.feedbackFail = 0;
            alarmData.overTemperature = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x2) == 2) {
            alarmData.shortCircuit = 1;
            alarmData.feedbackFail = 0;
            alarmData.overloadCount = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x4) == 4) {
            alarmData.lowBattery = true;
            alarmData.lowBatteryCount = 1 + alarmData.lowBatteryCount;
            resetIHealValues();
        } else if ((alarmFlag & 0x40) == 64) {
            alarmData.fuseBlown = true;
            resetIHealValues();
        } else if ((alarmFlag & 0x80) == 128) {
            resetIHealValues();
        } else if ((alarmFlag & 0x100) == 256) {
            alarmData.overTemperature = 1;
            alarmData.feedbackFail = 0;
            alarmData.overloadCount = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x200) == 512) {
            alarmData.feedbackFail = 1;
            alarmData.overTemperature = 0;
            alarmData.overloadCount = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x800) == 2048) {
            alarmData.batteryCharged = true;
            resetIHealValues();
        } else if ((alarmFlag & 0x1000) == 4096) {
            resetIHealValues();
        } else if ((alarmFlag & 0x4000) == 16384) {
            resetIHealValues();
        } else {
            alarmData.batteryCharged = false;
            alarmData.lowBattery = false;
            alarmData.fuseBlown = false;
            alarmData.lowBatteryCount = 0;
            resetIHealValues();
        } 
        alarmData.firstCycleNotCompleted = ((alarmFlag & 0x2000) == 8192);
        updateStatus (alarmFlag);
    }
    return ret;
}

#define STATUS_SHORT_CIRCUIT            0x0001
#define STATUS_LOW_BATTERY              0x0002
#define STATUS_MCB_TRIPPED              0x0004
#define STATUS_FEEDBACK                 0x0008
#define STATUS_OVER_TEMPERATURE         0x0010
#define STATUS_FEEDBACK_FAIL            0x0020
#define STATUS_BATTERY_CHARGED          0x0040
#define STATUS_WATER_TOPPING_REQUIRED   0x0080
#define STATUS_BATTERY_HIGH             0x0100
#define STATUS_OVERLOAD                 0x0200

void Inverter::updateStatus (short status){
    m_alarmStatus = ((status & 0x1) == 1) ? m_alarmStatus | STATUS_OVERLOAD : m_alarmStatus & ~(STATUS_OVERLOAD);
    m_alarmStatus = ((status & 0x2) == 2) ? m_alarmStatus | STATUS_SHORT_CIRCUIT : m_alarmStatus & ~(STATUS_SHORT_CIRCUIT);
    m_alarmStatus = ((status & 0x4) == 4) ? m_alarmStatus | STATUS_LOW_BATTERY : m_alarmStatus & ~(STATUS_LOW_BATTERY);
    m_alarmStatus = ((status & 0x40) == 64) ? m_alarmStatus | STATUS_MCB_TRIPPED : m_alarmStatus & ~(STATUS_MCB_TRIPPED);
    m_alarmStatus = ((status & 0x80) == 128) ? m_alarmStatus | STATUS_FEEDBACK : m_alarmStatus & ~(STATUS_FEEDBACK);
    m_alarmStatus = ((status & 0x100) == 256) ? m_alarmStatus | STATUS_OVER_TEMPERATURE : m_alarmStatus & ~(STATUS_OVER_TEMPERATURE);
    m_alarmStatus = ((status & 0x200) == 512) ? m_alarmStatus | STATUS_FEEDBACK_FAIL : m_alarmStatus & ~(STATUS_FEEDBACK_FAIL);
    m_alarmStatus = ((status & 0x800) == 2048) ? m_alarmStatus | STATUS_BATTERY_CHARGED : m_alarmStatus & ~(STATUS_BATTERY_CHARGED);
    m_alarmStatus = ((status & 0x1000) == 4096) ? m_alarmStatus | STATUS_WATER_TOPPING_REQUIRED : m_alarmStatus & ~(STATUS_WATER_TOPPING_REQUIRED);
    m_alarmStatus = ((status & 0x4000) == 16384) ? m_alarmStatus | STATUS_BATTERY_HIGH : m_alarmStatus & ~(STATUS_BATTERY_HIGH);
}

bool Inverter::setRegulatorLevel (byte level){
    if (level < 1 && level > 7)
        return false ;
    bool ret = sendCommand (CMD_SET_REGULATOR_LEVEL0+level) ;
    if (ret){
        if (0 == memcmp (invCmd[CMD_SET_REGULATOR_LEVEL0+level], respBuf, 6) && 0x55aa == ((short)*((short*)(respBuf+6)))){
            m_regulatorLevel = level ;
            return true ;
        }else{
            return false;
        }
    }
    return ret;
}

bool Inverter::setCutOffTime (byte cutOffTm){
    bool ret = false ;
    int cmd = 0;
    if ((m_forcedCutOff && 0 != cutOffTm) || (!m_forcedCutOff && 0 == cutOffTm))
        return false;
    switch (cutOffTm){
        case 0:
            cmd = CMD_SET_CUTOFF_TIME_0M;
            break;
        case 30:
            cmd = CMD_SET_CUTOFF_TIME_30M;
            break;
        case 60:
            cmd = CMD_SET_CUTOFF_TIME_60M;
            break;
        case 120:
            cmd = CMD_SET_CUTOFF_TIME_120M;
            break;
    }
    if (cmd != 0){
        ret = sendCommand (cmd) ;
        if (0 == memcmp (invCmd[cmd], respBuf, 6) && 0x55aa == ((short)*((short*)(respBuf+6)))){
            m_timeToResume = cutOffTm ;
            if (cutOffTm == 0){
                m_forcedCutOff = false;
            }else{
                ret = sendCommand (CMD_SET_CUTOFF_ON) ;
                if (0 == memcmp (invCmd[CMD_SET_CUTOFF_ON], respBuf, 6) && 0x55aa == ((short)*((short*)(respBuf+6)))){
                    m_forcedCutOff = true;
                    return true;
                }else{
                    return false;
                }
            }
        }else{
            return false;
        }
    }
    return ret;
}

bool Inverter::setApplianceMode (bool enable){
    bool ret = sendCommand (enable?CMD_SET_APPL_MODE_ON:CMD_SET_APPL_MODE_OFF) ;
    if (ret){
        if (0 == memcmp (invCmd[enable?CMD_SET_APPL_MODE_ON:CMD_SET_APPL_MODE_OFF], respBuf, 6) && 0x55aa == ((short)*((short*)(respBuf+6)))){
            m_applianceMode = enable ;
            return true ;
        }else{
            return false;
        }
    }
    return ret;
}

bool Inverter::setUPSMode (bool enable){
    bool ret = sendCommand (enable?CMD_SET_POWER_MODE_UPS:CMD_SET_POWER_MODE_NORMAL) ;
    if (ret){
        if (0 == memcmp (invCmd[enable?CMD_SET_POWER_MODE_UPS:CMD_SET_POWER_MODE_NORMAL], respBuf, 6) && 0x55aa == ((short)*((short*)(respBuf+6)))){
            m_UPSMode = enable ;
            return true ;
        }else{
            return false;
        }
    }
    return ret;
}

bool Inverter::setTurboCharging (bool enable){
    bool ret = sendCommand (enable?CMD_SET_TURBO_CHG_ON:CMD_SET_TURBO_CHG_OFF) ;
    if (ret){
        if (0 == memcmp (invCmd[enable?CMD_SET_TURBO_CHG_ON:CMD_SET_TURBO_CHG_OFF], respBuf, 6) && 0x55aa == ((short)*((short*)(respBuf+6)))){
            m_turboCharge = enable ;
            return true ;
        }else{
            return false;
        }
    }
    return ret;
}
