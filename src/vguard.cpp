#include "Arduino.h"
#include "vguard.h"
#include "internal.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define REFRESH_INTERVAL    5000

Inverter::Inverter(){
    isConnected = false;
    pClient = NULL ;
    myDevice = NULL ;
    lastUpdated = 0 ;
    respBuf = new byte[10];
    isCharging = true ;
    isDischarging = true;
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
    //Serial.println("Inverter::onResult");
    //Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    //Serial.print("RSSI: ");
    //Serial.printf("%ddB\r\n", advertisedDevice.getRSSI()) ;
    //Serial.printf("Address Type %d\r\n", advertisedDevice.getAddressType());

    if(advertisedDevice.getName() == "VG_SMART_BT"){
        //Serial.println("Found our device");
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
            //Serial.println(charUUIDRX.toString().c_str());
            pClient->disconnect();
            return false;
        }
        
        if(pRemoteRXCharacteristic->canRead()) {
            std::string value = pRemoteRXCharacteristic->readValue();
            //Serial.print("The characteristic value was: ");
            //Serial.println(value.c_str());
        }
        
        if(pRemoteRXCharacteristic->canNotify()||pRemoteRXCharacteristic->canIndicate()){
            const uint8_t notifyOn[] = {0x1,0x0};
            //const uint8_t notifyOff[] = {0x0,0x0};
            pRemoteRXCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notifyOn,2,true);
            //Serial.println("Enabling Notification");
            //pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notifyOff,2,true);
                delay(200);
            pRemoteRXCharacteristic->registerForNotify(this);
            delay(400);
        }

        //Setup TX Characteristcs
        pRemoteTXCharacteristic = pRemoteService->getCharacteristic(BLEUUID("0003cdd2-0000-1000-8000-00805f9b0131"));
        if (pRemoteTXCharacteristic == nullptr) {
            Serial.print("Failed to find our characteristic UUID: ");
            //Serial.println(charUUIDRX.toString().c_str());
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
        if (NULL != pData && length > 0){
            memcpy (respBuf, pData, MIN(10, length));
            semaphoreCmdWait.give();
            //Serial.print("Notify callback for RX characteristic ");
            //Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
            //Serial.print(" of data length ");
            //Serial.println(length);
            //Serial.print("Notify data: ");
            //hexDump (pData, length) ; 
            //short paramVal = *((short*)(pData+6)) ;
            //Serial.println(paramVal);
        }
    }
}

void Inverter::hexDump (uint8_t *data, size_t length){
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
            isDischarging = true ;
            readLoadPercentage ();
            readLoadCurrent ();
            calculateBackupTime ();
            if (m_forcedCutOff){
                readTimeToResume ();
            }
        }else{
            isCharging = true ;
            readChargingCurrent ();
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
        Serial.println ("\r\n");
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
      f3 = 12.8F;
    } else {
      f3 = 12.6F;
    } 
    float batteryLowChargeFactor = f3 * f2;
    float batteryLowDischargeFactor = 10.4F * i;
    float batteryFactor = (i * 14);
   
    Serial.printf ("battery battype[%d] cnt[%d] discharging [%d] charging[%d]\r\n", m_batteryType, m_batteryCount, isDischarging, isCharging);
    Serial.printf ("batVolt[%.2f] lochfact[%.2f] batFact[%.2f] f3[%.2f] f2[%.2f]\r\n", m_batteryVoltage, batteryLowChargeFactor, batteryFactor, f3, f2);
    if (!m_backupMode) {
        isDischarging = false;
        if (alarmData.batteryCharged){
            m_chargingPercent = 100.0F;
            Serial.printf ("1. [%d]", m_chargingPercent);
            return; 
        }
        if (f1 < batteryLowChargeFactor)
            f1 = batteryLowChargeFactor; 
        int n = (int)(100.0F * ((f1 - batteryLowChargeFactor) / (batteryFactor - (f2 * 12.0F))));
        if (isCharging) {
            int i2 = getMaskedBattPercentage(n);
            m_chargingPercent = i2;
            dischargingPer = i2;
            Serial.printf ("2. [%d]", m_chargingPercent);
            return;
        } 
        if (n > m_chargingPercent) {
            int i2 = getMaskedBattPercentage(n);
            m_chargingPercent = i2;
            dischargingPer = i2;
            Serial.printf ("3. [%d]", m_chargingPercent);
            return;
        } 
        int i1 = chargingPer;
        dischargingPer = i1;
        m_chargingPercent = i1;
        Serial.printf ("4. [%d]", m_chargingPercent);
        return;
    } 
    isCharging = false;
    if (f1 < batteryLowDischargeFactor)
        f1 = batteryLowDischargeFactor; 
    int j = m_loadPercent;
    int k = (int)(100.0F * ((f1 - batteryLowDischargeFactor) / (batteryFactor - 0.007F * (float)j - ((float)i * 12.0F))));
    if (isDischarging) {
        int n = getMaskedBattPercentage(k);
        chargingPer = n;
        m_dischargingPercent = n;
        Serial.printf ("5. [%d]", m_dischargingPercent);
        return ;
    } 
    if (k < dischargingPer) {
        int n = getMaskedBattPercentage(k);
        chargingPer = n;
        m_dischargingPercent = n;
        Serial.printf ("6. [%d]", m_dischargingPercent);
        return;
    } 
    int m = dischargingPer;
    chargingPer = m;
    m_dischargingPercent = m;
    Serial.printf ("7. [%d]", m_dischargingPercent);
    return ;
}

int Inverter::getMaskedBattPercentage (int percent) {
    return (percent < 0) ? 0 : ((percent > 100) ? 100 : percent);
}

void Inverter::calculateBackupTime(){
    float f6;
    int i = 150;
    int j = m_batteryCount;
    float f1 = 1.0F * i;
    float f2 = 15.0F * j;
    float f3 = 12.2F * j;
    float f4 = 10.0F * j;
    float f5 = m_batteryVoltage;
    if (f5 < 12.2F)
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
        m_sysTemperature = (m_sysTemperature-32.0F) * 5.0F/9.0F ;
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
    bool ret = sendCommand (CMD_GET_BATTERY_COUNT) ;
    if (ret){
        m_batteryCount = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}
bool Inverter::readCutoffStatus(){
    bool ret = sendCommand (CMD_GET_BATTERY_COUNT) ;
    if (ret){
        m_batteryCount = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}
bool Inverter::readTimeToResume(){
    bool ret = sendCommand (CMD_GET_BATTERY_COUNT) ;
    if (ret){
        m_batteryCount = ((short)*((short*)(respBuf+6)));
    }
    return ret;
}
bool Inverter::readApplianceModeStatus(){
    bool ret = sendCommand (CMD_GET_BATTERY_COUNT) ;
    if (ret){
        m_batteryCount = ((short)*((short*)(respBuf+6)));
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
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296849));
            alarmData.overloadCount = 1;
            /*if (this.mOverLoadCount == 3)
                showWarningAlert(); */
            alarmData.feedbackFail = 0;
            alarmData.overTemperature = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x2) == 2) {
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296850));
            alarmData.shortCircuit = 1;
            /*if (alarmData.shortCircuit == 3)
            showWarningAlert(); */
            alarmData.feedbackFail = 0;
            alarmData.overloadCount = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x4) == 4) {
        /*showViewAnimated((View)this.mWarningContainer, true, getString(2131296845));
        if (this.adapter.getItem(this.mViewPager.getCurrentItem()) != null && this.adapter.getItem(this.mViewPager.getCurrentItem()) instanceof StatusFragment)
            ((StatusFragment)this.adapter.getItem(this.mViewPager.getCurrentItem())).batteryStatusHide(); */
            alarmData.lowBattery = true;
            alarmData.lowBatteryCount = 1 + alarmData.lowBatteryCount;
            resetIHealValues();
        } else if ((alarmFlag & 0x40) == 64) {
            /*String str;
            if (this.mProduct != null) {
            str = this.mProduct.getModelName();
            } else {
            str = "";
            } 
            if (str.contains(getString(2131296785)) || str.contains(getString(2131296786))) {
            showViewAnimated((View)this.mWarningContainer, true, getString(2131296843));
            } else {
            showViewAnimated((View)this.mWarningContainer, true, getString(2131296846));
            } */
            alarmData.fuseBlown = true;
            /*if (this.adapter.getItem(this.mViewPager.getCurrentItem()) instanceof StatusFragment)
            ((StatusFragment)this.adapter.getItem(this.mViewPager.getCurrentItem())).setData(this.mInverterStatus, this.firstCycleNotCompleted); */
            resetIHealValues();
        } else if ((alarmFlag & 0x80) == 128) {
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296839));
            resetIHealValues();
        } else if ((alarmFlag & 0x100) == 256) {
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296848));
            alarmData.overTemperature = 1;
            //if (alarmData.mOverTemperature == 3)
            //    showWarningAlert(); 
            alarmData.feedbackFail = 0;
            alarmData.overloadCount = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x200) == 512) {
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296842));
            alarmData.feedbackFail = 1;
            /*if (this.mFeedbackFail == 3)
                showWarningAlert(); */
            alarmData.overTemperature = 0;
            alarmData.overloadCount = 0;
            alarmData.shortCircuit = 0;
        } else if ((alarmFlag & 0x800) == 2048) {
            alarmData.batteryCharged = true;
            /*if ((alarmFlag & 0x1000) == 4096) {
            showViewAnimated((View)this.mWarningContainer, true, getString(2131296851));
            } else if ((alarmFlag & 0x1000) != 4096) {
            showViewAnimated((View)this.mWarningContainer, false, (String)null);
            this.mWarningContainer.setVisibility(8);
            } */
            resetIHealValues();
        } else if ((alarmFlag & 0x1000) == 4096) {
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296851));
            resetIHealValues();
        } else if ((alarmFlag & 0x4000) == 16384) {
            //showViewAnimated((View)this.mWarningContainer, true, getString(2131296840));
            resetIHealValues();
        } else {
            alarmData.batteryCharged = false;
            alarmData.lowBattery = false;
            alarmData.fuseBlown = false;
            alarmData.lowBatteryCount = 0;
            resetIHealValues();
            /*if (!this.isBatterySame && !this.skipConnection) {
            showViewAnimated((View)this.mWarningContainer, true, getString(2131296858));
            } else {
            showViewAnimated((View)this.mWarningContainer, false, (String)null);
            this.mWarningContainer.setVisibility(8);
            } */
        } 
        /*if ((paramInt & 0x2000) == 8192) {
        bool = true;
        } else {
        bool = false;
        }*/
        alarmData.firstCycleNotCompleted = ((alarmFlag & 0x2000) == 8192);
    }
    return ret;
}