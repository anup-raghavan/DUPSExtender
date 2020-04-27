#include "BLEDevice.h"

struct __alarm_data{
    byte overloadCount;
    byte overTemperature;
    byte shortCircuit;
    byte feedbackFail;
    bool lowBattery;
    int lowBatteryCount;
    bool fuseBlown;
    bool batteryCharged;
    bool firstCycleNotCompleted;
};

class Inverter : public BLERCCallbacks, public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks{
    public:
    Inverter();
    ~Inverter();
    bool attach ();
    bool detach ();
    bool refresh(bool full=false);
    float m_mainsVoltage ;
    float m_batteryVoltage ;
    float m_sysTemperature;
    byte m_batteryCount ;
    float m_loadCurrent ;
    float m_backupTime ;
    bool m_hoildayMode;
    bool m_UPSMode;
    byte m_regulatorLevel;
    bool m_inverterSwitch;
    //float m_batteryPercent;
    byte m_dischargingPercent;
    byte m_chargingPercent;
    bool m_turboCharge;
    float m_chargingCurrent;
    float m_loadPercent;
    bool m_forcedCutOff;
    byte m_timeToResume;
    bool m_applianceMode;
    byte m_batteryType;
    bool m_backupMode;

    bool setRegulatorLevel (byte level);
    bool setCutOffTime (byte cutOffTm);
    bool setApplianceMode (bool enable);
    bool setUPSMode (bool enable);
    bool setTurboCharging (bool enable);

    bool sendCommand (int cmd, bool log=false, uint32_t tout=150, size_t length=8);

    private:
    bool readMainsVoltage();            //Refreshed
    bool readBatteryVoltage();          //Refreshed
    bool readSystemTemperature();       //Refreshed
    bool readBatteryCount();            //Once
    bool readLoadCurrent();             //On Backup
    void calculateBackupTime();         //On Backup
    void calculateBatteryPercent();     //Refreshed
    bool readHolidayMode();             //Once
    bool readInverterMode();            //Once
    bool readRegulatorLevel();          //Once
    bool readInverterSwitchStatus();    //Once
    //bool readBatteryPercentage();       //Refreshed on Mains
    bool readTurboChargeStatus();       //Once
    bool readChargingCurrent();         //Refreshed on Mains
    bool readLoadPercentage();          //On Backup
    bool readCutoffStatus();            //Once
    bool readTimeToResume();            //On Forced Cutoff
    bool readApplianceModeStatus();     //Once
    bool readBatteryType();             //Once
    bool readBackupModeStatus();        //Refreshed
    bool readAlarmData();              //Refreshed

    FreeRTOS::Semaphore  semaphoreCmdWait  = FreeRTOS::Semaphore("CommandResponseWait");
    BLEAdvertisedDevice* myDevice;
    BLERemoteCharacteristic* pRemoteRXCharacteristic;
    BLERemoteCharacteristic* pRemoteTXCharacteristic;
    BLEClient *pClient;
    bool isConnected;
    bool isCharging;
    bool isDischarging;
    int chargingPer;
    int dischargingPer;
    uint32_t lastUpdated;
    bool init();
    void resetIHealValues();
    int getMaskedBattPercentage (int percent);
    void onNotify(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    //BLE Client Callback
    void onConnect(BLEClient* pclient);
    void onDisconnect(BLEClient* pclient);
    //Advertisement Callaback
    void onResult(BLEAdvertisedDevice advertisedDevice);
    //static void notifyCallback (BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    static void hexDump (uint8_t *data, size_t length);
    const char *deviceAddr = "ba:05:00:08:14:0b";
    struct __alarm_data alarmData;
    byte *respBuf;
    byte curCmd;
};