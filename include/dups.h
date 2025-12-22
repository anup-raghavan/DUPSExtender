/**
 * @file dups.h
 * @author Anup
 * @brief Header file for the V-Guard DUPS Inverter Controller library.
 * @version 1.0
 * @date 2020
 * 
 * @copyright Copyright (c) 2020 Anup
 * 
 */

#include "BLEDevice.h"
#ifndef __DUPS_H_
#define __DUPS_H_

/// @brief Overload alarm flag bitmask
#define STATUS_OVERLOAD                 0x0000
/// @brief Short circuit alarm flag bitmask
#define STATUS_SHORT_CIRCUIT            0x0001
/// @brief Low battery alarm flag bitmask
#define STATUS_LOW_BATTERY              0x0002
/// @brief MCB tripped alarm flag bitmask
#define STATUS_MCB_TRIPPED              0x0004
/// @brief Feedback alarm flag bitmask
#define STATUS_FEEDBACK                 0x0008
/// @brief Over temperature alarm flag bitmask
#define STATUS_OVER_TEMPERATURE         0x0010
/// @brief Feedback fail alarm flag bitmask
#define STATUS_FEEDBACK_FAIL            0x0020
/// @brief Battery charging alarm flag bitmask
#define STATUS_BATTERY_CHARGING         0x0040
/// @brief Battery charged alarm flag bitmask
#define STATUS_BATTERY_CHARGED          0x0080
/// @brief Water topping required alarm flag bitmask
#define STATUS_WATER_TOPPING_REQUIRED   0x0100
/// @brief High battery voltage alarm flag bitmask
#define STATUS_BATTERY_HIGH             0x0200

#define BATTERY_CAPACITY_AH             150
#define BATTERY_FULL_VOLTAGE_TUBULAR    12.8F
#define BATTERY_FULL_VOLTAGE_FLAT       12.6F
#define BATTERY_EMPTY_VOLTAGE           10.4F
#define BATTERY_NOMINAL_VOLTAGE         12.0F
#define BATTERY_BACKUP_CALC_THRESHOLD   12.2F

/**
 * @brief Structure to hold alarm/error counters and statuses.
 */
struct __alarm_data{
    byte overloadCount;         ///< Counter for overload events
    byte overTemperature;       ///< Counter for over-temperature events
    byte shortCircuit;          ///< Counter for short-circuit events
    byte feedbackFail;          ///< Counter for feedback failures
    bool lowBattery;            ///< Flag indicating low battery status
    int lowBatteryCount;        ///< Counter for low battery events
    bool fuseBlown;             ///< Flag indicating blown fuse
    bool batteryCharged;        ///< Flag indicating battery is fully charged
    bool firstCycleNotCompleted;///< Flag indicating if the first charge cycle is pending
};

/**
 * @brief Main Inverter Controller Class.
 * 
 * Handles BLE communication with the V-Guard Inverter, including
 * connection management, command sending, and status parsing.
 */
class Inverter : public BLERCCallbacks, public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks{
    public:
    /**
     * @brief Construct a new Inverter object.
     * Initializes default values for all member variables.
     */
    Inverter();

    /**
     * @brief Destroy the Inverter object.
     * Cleans up allocated memory.
     */
    ~Inverter();

    /**
     * @brief Initialize and start the BLE scanning process.
     * 
     * @return true If initialization was successful.
     * @return false If initialization failed.
     */
    bool attach ();

    /**
     * @brief Stop the BLE client and clean up.
     * 
     * @return true Always returns true.
     */
    bool detach ();

    /**
     * @brief Refreshes the inverter status data.
     * 
     * Connects if not connected, and then queries the inverter for the latest data.
     * Data is refreshed based on a time interval unless forced.
     * 
     * @param full If true, refreshes all parameters including static ones. Default is false.
     * @return true If data was successfully refreshed.
     * @return false If refresh was skipped (timer) or failed.
     */
    bool refresh(bool full=false);

    float m_mainsVoltage ;      ///< Current Mains Voltage (V)
    float m_batteryVoltage ;    ///< Current Battery Voltage (V)
    float m_sysTemperature;     ///< System Temperature (Celsius)
    byte m_batteryCount ;       ///< Number of batteries connected
    float m_loadCurrent ;       ///< Current Load (Amps)
    float m_backupTime ;        ///< Estimated Backup Time (Minutes)
    bool m_hoildayMode;         ///< Holiday Mode Status
    bool m_UPSMode;             ///< UPS Mode Status (True=UPS, False=Normal)
    byte m_regulatorLevel;      ///< Current Regulator Level (1-7)
    bool m_inverterSwitch;      ///< Inverter Switch Status
    
    //float m_batteryPercent;
    byte m_dischargingPercent;  ///< Battery Discharge Percentage (0-100)
    byte m_chargingPercent;     ///< Battery Charge Percentage (0-100)
    bool m_turboCharge;         ///< Turbo Charging Status
    float m_chargingCurrent;    ///< Charging Current (Amps)
    float m_loadPercent;        ///< Load Percentage (0-100)
    bool m_forcedCutOff;        ///< Forced Cut-off Status
    byte m_timeToResume;        ///< Time remaining for forced cut-off to end (Minutes)
    bool m_applianceMode;       ///< Appliance Mode Status (True=High Power, False=Normal)
    byte m_batteryType;         ///< Battery Type (1=Tubular, Other=Flat)
    bool m_backupMode;          ///< Backup Mode Status (True=On Battery, False=On Mains)
    int m_backupSince;          ///< Time in seconds since switching to backup
    short m_alarmStatus;        ///< Bitmask of current alarm statuses

    /**
     * @brief Sets the Output Voltage Regulator Level.
     * 
     * @param level Level to set (1-7).
     * @return true If command was successful.
     * @return false If command failed or level is invalid.
     */
    bool setRegulatorLevel (byte level);

    /**
     * @brief Sets the Force Cut-off Time.
     * 
     * @param cutOffTm Time in minutes (0, 30, 60, 120). 0 disables cut-off.
     * @return true If command was successful.
     * @return false If command failed or value is invalid.
     */
    bool setCutOffTime (byte cutOffTm);

    /**
     * @brief Enable or Disable Appliance Mode.
     * 
     * @param enable True to enable, False to disable.
     * @return true If command was successful.
     * @return false If command failed.
     */
    bool setApplianceMode (bool enable);

    /**
     * @brief Enable or Disable UPS Mode.
     * 
     * @param enable True for UPS Mode, False for Normal Mode.
     * @return true If command was successful.
     * @return false If command failed.
     */
    bool setUPSMode (bool enable);

    /**
     * @brief Enable or Disable Turbo Charging.
     * 
     * @param enable True to enable, False to disable.
     * @return true If command was successful.
     * @return false If command failed.
     */
    bool setTurboCharging (bool enable);

    /**
     * @brief Send a raw command to the inverter.
     * 
     * @param cmd Command ID (from internal.h defines).
     * @param log If true, logs the response to Serial.
     * @param tout Timeout in milliseconds to wait for response.
     * @param length Length of the command data in bytes.
     * @return true If command sent and response received.
     * @return false If command failed.
     */
    bool sendCommand (int cmd, bool log=false, uint32_t tout=350, size_t length=8);

    private:
    // Internal helper methods for reading specific parameters
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
    int backupSwitchTS;
    
    /**
     * @brief Internal initialization logic.
     * Connects to the BLE device and discovers services.
     * @return true if successful.
     */
    bool init();

    void resetIHealValues();
    int getMaskedBattPercentage (int percent);
    void updateStatus (short status);

    /**
     * @brief BLE Notification Callback.
     * Handles incoming data from the inverter.
     */
    void onNotify(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    
    //BLE Client Callback
    void onConnect(BLEClient* pclient);
    void onDisconnect(BLEClient* pclient);
    
    //Advertisement Callaback
    void onResult(BLEAdvertisedDevice advertisedDevice);
    
    static void hexDump (uint8_t *data, size_t length);
    const char *deviceAddr = "ba:05:00:08:14:0b";
    struct __alarm_data alarmData;
    byte *respBuf;
    byte curCmd;
};
#endif /*__DUPS_H_*/