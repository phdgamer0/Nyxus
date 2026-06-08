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
#ifndef _NYXUS_SCANNER_HPP_
#define _NYXUS_SCANNER_HPP_
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>
#include <esp_timer.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <Nyxus/nyx_file_parser.hpp>
#include <Nyxus/nyx_terminal_graphics.hpp>
#include <algorithm>
#include <RTClib.h>

extern SdFat sd;
extern RTC_DS3231 rtc;
inline constexpr uint32_t init_delay_time = 100;
/**
* @brief Ping-Pong Buffer Size (8 Kilobytes per buffer).
* @note 8KB perfectly aligns with standard FAT32 SD card cluster sizes for maximum SPI bus write speeds.
*/
inline constexpr size_t SNIFFER_BUFFER_SIZE = 8192;
class NYXUS_SCANNER{
   public:
   friend class NYXUS_FILE_PARSER;

   private:
   /**
   * @brief Global singleton pointer required to bridge the C-based ESP-IDF hardware callback to this C++ class instance.
   */
   inline static NYXUS_SCANNER* active_sniffer_instance = nullptr;

   protected:
   /**
   * @brief Configuration profile for scanning a network.
   */
   ScanningConfig scanConfig;

   private:
   /** 
   * @brief The active PCAP database file
   */
   FsFile sniffer_file;

   private:
   /**
   * @brief Primary SRAM Capture Buffer
   */
   uint8_t sniffer_buffer_A[SNIFFER_BUFFER_SIZE];

   private:
   /**
   * @brief Secondary SRAM Capture Buffer
   */
   uint8_t sniffer_buffer_B[SNIFFER_BUFFER_SIZE];


   protected:
   /**
   * @brief Pointer to the SD card.
   */
   SdFat* sd = nullptr;

   private:
   /**
   * @brief Pointer tracking which buffer the radio is actively filling 
   */
   uint8_t* active_rx_buffer = sniffer_buffer_A;

   private:
   /**
   * @brief byte offset in the active buffer
   */
   size_t active_rx_offset = 0;
   
   private:
   /**
   * @brief process communication queue for the writer task 
   */
   QueueHandle_t sniffer_flush_queue = nullptr;

   private:
   /**
   * @brief FreeRTOS handle for the background SD disk thread    
   */
   TaskHandle_t sniffer_writer_task_handle = nullptr;
   
   private:
   /**
   * @brief Hardware state toggle
   */
   bool is_sniffing = false; 

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
   * @brief Standard TCP Header layout for raw packet forging.
   * @note Packed to prevent compiler padding from corrupting the byte sequence.
   * @paragraph Header Forging
   * Standard LwIP sockets abstract the TCP layer away. To execute stealth SYN scans, we must 
   * manually construct the exact byte sequence of a TCP header. The `__attribute__((packed))` 
   * directive is absolutely critical; it prevents the GCC compiler from inserting invisible memory 
   * alignment padding, which would instantly invalidate the payload structure on the wire.
   */
   struct __attribute__((packed)) tcp_header {
      uint16_t source_port;
      uint16_t dest_port;
      uint32_t sequence;
      uint32_t acknowledge;
      uint8_t data_offset;  // 4 bits data offset, 4 bits reserved
      uint8_t flags;        // CWR, ECE, URG, ACK, PSH, RST, SYN, FIN
      uint16_t window;
      uint16_t checksum;
      uint16_t urgent_ptr;
   };

   private:
   /**
   * @brief Pseudo Header required by RFC 793 for calculating TCP checksums.
   * @paragraph Checksum Cryptography
   * The internet relies on strict validation. A TCP checksum isn't just calculated over the TCP 
   * header itself; it requires a "Pseudo Header" containing the Source IP, Destination IP, and 
   * Protocol ID. If this is calculated incorrectly by even a single bit, the target's firewall 
   * will silently drop the packet as corrupted noise.
   */
   struct __attribute__((packed)) pseudo_header {
      uint32_t source_address;
      uint32_t dest_address;
      uint8_t placeholder;
      uint8_t protocol;
      uint16_t tcp_length;
   };

   private:
   /**
   * @brief Standard UDP Header layout for raw datagram forging.
   * @note Packed to prevent compiler alignment padding.
   */
   struct __attribute__((packed)) udp_header {
      uint16_t source_port;
      uint16_t dest_port;
      uint16_t length;
      uint16_t checksum;
   };

   private:
   /**
   * @brief Cryptographic checksum calculator for raw IP/TCP packets.
   * @param ptr Pointer to the raw byte buffer.
   * @param nbytes Length of the buffer in bytes.
   * @return The 16-bit One's Complement checksum.
   */
   uint16_t calculate_checksum(uint16_t *ptr, int nbytes) {
      long sum = 0;
      uint16_t oddbyte;
      while (nbytes > 1) {
         sum += *ptr++;
         nbytes -= 2;
      }
      if (nbytes == 1) {
         oddbyte = 0;
         *((uint8_t*)&oddbyte) = *(uint8_t*)ptr;
         sum += oddbyte;
      }
      sum = (sum >> 16) + (sum & 0xffff);
      sum += (sum >> 16);
      return static_cast<uint16_t>(~sum);
   }

   private:
   /**
   * @brief Fetches the active IPv4 address assigned to the ESP32 Station interface.
   * @return The 32-bit integer representation of the local IP.
   */
   uint32_t get_local_ipv4() {
      esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      if (!netif) return 0;
      esp_netif_ip_info_t ip_info;
      esp_netif_get_ip_info(netif, &ip_info);
      return ip_info.ip.addr;
   }


   private:
   /**
   * @struct SnifferFlushMsg
   * @brief The payload passed through the FreeRTOS queue instructing the background task to dump SRAM to disk.
   */
   struct SnifferFlushMsg {
      uint8_t* buffer;  /**< Pointer to the filled buffer ready for disk serialization */
      size_t length;    /**< Amount of valid byte data to write                        */
   };

   public:
   /**
   * @brief Instantiates the Nyxus Scanner Interface.
   * @param toggleVerbosity Enables detailed serial telemetry output.
   * @param timestamp Enables timestamping on files.
   * @param scanConfig Optional configuration payload for timing/IP limits. Uses default safety parameters if omitted.
   */
   NYXUS_SCANNER(bool toggleVerbosity = true, bool timestamp = true, SdFat* disk = nullptr, const ScanningConfig &scanConfig = {.savePath = std::nullopt, .ports = {}, .timeout = 1000,  .sd = nullptr, .mode = ScanMode::NONE, .speed = ScanSpeed::MEDIUM, .retry = 0, .verbose = false, .timestampEnabled = false}): timestampEnabled(timestamp), verbose(toggleVerbosity), scanConfig(scanConfig), sd(disk) {

   };

   public:
   /**
    * @brief Verifies silicon availability and ensures the baremetal stack is primed for raw sockets.
    * @attention This method gracefully piggybacks on the existing `NYXUS_WIFI` connection. It will NOT kill your active Wi-Fi session.
    * @return void
    */
   void KickStart() {
      if (verbose) {
         Serial.println("===============================");
         timestamp(); Serial.printf("[%sSCANNER%s] Verifying hardware state for raw socket injection...\n", Color::YELLOW, Color::RESET);
      }
      wifi_mode_t current_mode;
      esp_err_t err = esp_wifi_get_mode(&current_mode);
      if (err == ESP_OK && current_mode != WifiMode::off) {
         if (verbose) {
            timestamp(); Serial.printf("[%sINFO%s] Active radio session detected. Piggybacking on existing LwIP stack...\n", Color::GREEN, Color::RESET);
         }
         esp_wifi_set_ps(WIFI_PS_NONE); 
      }
      else {
         if (verbose) {
            timestamp(); Serial.printf("[%sWARNING%s] Radio is offline or uninitialized. Booting standalone LwIP framework...\n", Color::ORANGE, Color::RESET);
         }
         esp_err_t ret = nvs_flash_init();
         if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
         }
         esp_netif_init();
         esp_event_loop_create_default();
         
         wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
         esp_wifi_init(&cfg);
         esp_wifi_set_storage(WIFI_STORAGE_RAM);
         esp_wifi_set_mode(WIFI_MODE_STA); 
         esp_wifi_start();
         esp_wifi_set_ps(WIFI_PS_NONE);
      }
      if (verbose) {
         timestamp(); Serial.printf("[%sSUCCESS%s] Scanner engine online. Raw sockets unlocked.\n", Color::GREEN, Color::RESET);
         Serial.println("===============================");
      }
   }
   
   private:
   /**
    * @brief The core baremetal sweeping engine using LwIP raw sockets.
    * @param target_ip The IPv4 address of the target machine.
    * @param save_result Enables serializing the output to the SD card.
    * @param save_path The absolute path for the results file.
    * @paragraph Dynamic Protocol Forging & Bitwise Execution
    * This engine parses the `scanConfig.mode` bitmask to simultaneously forge UDP datagrams or custom TCP packets. 
    * If multiple TCP modes (e.g., SYN | SYN_ACK) are supplied, the bits are dynamically OR'd together to craft 
    * advanced firewall-evasion packets. Transmission latency is mapped inversely from the 8-bit speed payload 
    * directly to the hardware timer (`255 - speed`), ensuring zero-latency floods at 0xFF.
    * @return void
    */
   void ExecuteSweep(const std::string& target_ip, bool save_result, const std::string& save_path) {
      bool scan_all = (std::find(scanConfig.ports.begin(), scanConfig.ports.end(), 0) != scanConfig.ports.end());
      if (scanConfig.ports.empty() && !scan_all) {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No ports defined in configuration.\n", Color::RED, Color::RESET); }
         return;
      }
      uint32_t src_ip = get_local_ipv4();
      if (src_ip == 0) {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Radio not assigned an IPv4. Cannot craft return headers.\n", Color::RED, Color::RESET); }
         return;
      }
      struct sockaddr_in dest_addr;
      dest_addr.sin_family = AF_INET;
      inet_pton(AF_INET, target_ip.c_str(), &dest_addr.sin_addr);
      bool use_tcp = (scanConfig.mode & (ScanMode::SYN | ScanMode::SYN_ACK | ScanMode::TCP_OUT | ScanMode::TCP_IN));
      bool use_udp = (scanConfig.mode & (ScanMode::UDP_OUT | ScanMode::UDP_IN));
      int tcp_sock = -1, udp_sock = -1;
      if (use_tcp) tcp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
      if (use_udp) udp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
      if (tcp_sock < 0 && udp_sock < 0) {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Failed to allocate any raw sockets. Code: %d\n", Color::RED, Color::RESET, errno); }
         return;
      }
      uint32_t hardware_delay_ms = 255 - scanConfig.speed; 
      if (verbose) {
         timestamp(); Serial.printf("[%sSCANNER%s] Initiating raw hardware sweep against %s%s%s. Inter-packet Latency: %dms\n", Color::MAGENTA, Color::RESET, Color::YELLOW, target_ip.c_str(), Color::RESET, hardware_delay_ms);
      }
      FsFile logFile;
      if (save_result && save_path != "" && sd) {
         logFile = sd->open(save_path.c_str(), O_WRITE | O_CREAT | O_AT_END);
         if (logFile) logFile.printf("\n--- TARGET: %s ---\n", target_ip.c_str());
      }
      uint32_t total_ports = scan_all ? INT16_MAX : scanConfig.ports.size();
      for (uint32_t i = 0; i < total_ports; i++) {
         uint16_t port = scan_all ? static_cast<uint16_t>(i + 1) : scanConfig.ports[i];
         if (port == 0) continue;
         bool target_responsive = false;
         std::string forensic_result = "NO RESPONSE";
         for (uint8_t attempt = 0; attempt <= scanConfig.retry; attempt++) {
            uint16_t dynamic_src_port = (uint16_t)random(10000, 60000); // Randomize origin to evade IDS filters
            if (use_tcp && tcp_sock >= 0) {
               uint8_t datagram[sizeof(pseudo_header) + sizeof(tcp_header)] = {0};
               pseudo_header* psh = (pseudo_header*) datagram;
               tcp_header* tcph = (tcp_header*) (datagram + sizeof(pseudo_header));
               tcph->source_port = htons(dynamic_src_port);
               tcph->dest_port   = htons(port);
               tcph->sequence    = htonl(random(1000, 999999));
               tcph->acknowledge = 0;
               tcph->data_offset = (5 << 4); // 20-byte standard header
               tcph->window      = htons(1024);
               tcph->urgent_ptr  = 0;
               tcph->checksum    = 0;
               tcph->flags = 0;
               if (scanConfig.mode & ScanMode::SYN)     tcph->flags |= 0x02; // TCP SYN
               if (scanConfig.mode & ScanMode::SYN_ACK) tcph->flags |= 0x12; // TCP SYN-ACK
               if (scanConfig.mode & ScanMode::TCP_OUT) tcph->flags |= 0x10; // TCP ACK
               psh->source_address = src_ip;
               psh->dest_address   = dest_addr.sin_addr.s_addr;
               psh->placeholder    = 0;
               psh->protocol       = IPPROTO_TCP;
               psh->tcp_length     = htons(sizeof(tcp_header));
               tcph->checksum = calculate_checksum((uint16_t*)datagram, sizeof(pseudo_header) + sizeof(tcp_header));
               sendto(tcp_sock, tcph, sizeof(tcp_header), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            }
            if (use_udp && udp_sock >= 0) {
               uint8_t datagram[sizeof(udp_header)] = {0};
               udp_header* udph = (udp_header*) datagram;
               udph->source_port = htons(dynamic_src_port);
               udph->dest_port   = htons(port);
               udph->length      = htons(sizeof(udp_header));
               udph->checksum    = 0;
               sendto(udp_sock, udph, sizeof(udp_header), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
            }
            unsigned long start_time = millis();
            char recv_buf[128];
            struct sockaddr_in source_addr;
            socklen_t socklen = sizeof(source_addr);
            while (millis() - start_time < scanConfig.timeout) {
               if (use_tcp && tcp_sock >= 0) {
                  int len = recvfrom(tcp_sock, recv_buf, sizeof(recv_buf), MSG_DONTWAIT, (struct sockaddr*)&source_addr, &socklen);
                  if (len > 0) {
                     uint8_t ip_ihl = (recv_buf[0] & 0x0F) * 4;
                     tcp_header* recv_tcph = (tcp_header*)(recv_buf + ip_ihl);
                     if (source_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr && ntohs(recv_tcph->dest_port) == dynamic_src_port) {
                        target_responsive = true;
                        if ((recv_tcph->flags & 0x12) == 0x12) { 
                           forensic_result = "OPEN (SYN-ACK)";
                        } else if ((recv_tcph->flags & 0x14) == 0x14) {
                           forensic_result = "CLOSED (RST)";
                        } else {
                           forensic_result = "FILTERED/ANOMALY";
                        }
                        break; 
                     }
                  }
               }
               if (use_udp && udp_sock >= 0) {
                  int len = recvfrom(udp_sock, recv_buf, sizeof(recv_buf), MSG_DONTWAIT, (struct sockaddr*)&source_addr, &socklen);
                  if (len > 0 && source_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr) {
                     target_responsive = true;
                     forensic_result = "OPEN (UDP BOUNCE)";
                     break;
                  }
               }
               vTaskDelay(pdMS_TO_TICKS(1));
            }
            if (target_responsive) break;
         }
         if (verbose) {
            const char* color = (forensic_result.find("OPEN") != std::string::npos) ? Color::GREEN : Color::RED;
            timestamp(); Serial.printf("[%s%s%s] Port %d\n", color, forensic_result.c_str(), Color::RESET, port);
         }
         if (logFile) {
            logFile.printf("Port %d: %s\n", port, forensic_result.c_str());
         }
         if (hardware_delay_ms > 0) vTaskDelay(pdMS_TO_TICKS(hardware_delay_ms)); 
      }
      if (logFile) logFile.close();
      if (tcp_sock >= 0) close(tcp_sock); 
      if (udp_sock >= 0) close(udp_sock); 
      if (verbose) Serial.println("===============================");
   }

   public:
   /**
    * @brief Executes a network sweep against a target using the currently loaded SRAM configuration.
    * @param target_ip The explicit IPv4 address of the target machine (e.g., "192.168.1.5").
    * @param save_result Enables serializing the output to the SD card.
    * @param save_path The absolute path for the results file (used if save_result is true).
    * @return void
    */
   void Scan(const std::string& target_ip, bool save_result = false, const std::string& save_path = ""){
      ExecuteSweep(target_ip, save_result, save_path);
   }

   public:
   /**
    * @brief Temporarily overrides the SRAM configuration and immediately executes a sweep.
    * @param target_ip The explicit IPv4 address of the target machine.
    * @param conf The temporary ScanningConfig profile to inject.
    * @param save_result Enables serializing the output to the SD card.
    * @param save_path The absolute path for the results file.
    * @return void
    */
   void Scan(const std::string& target_ip, const ScanningConfig& conf, bool save_result = false, const std::string& save_path = ""){
      ChangeScanConfig(conf);
      ExecuteSweep(target_ip, save_result, save_path);
   }

   public:
   /**
    * @brief Loads a `.scnconf` profile directly from the SD card and executes the sweep.
    * @param target_ip The explicit IPv4 address of the target machine.
    * @param path The absolute path to the `.scnconf` configuration file.
    * @param save_result Enables serializing the output to the SD card.
    * @param save_path The absolute path for the results file.
    * @return void
    */
   void Scan(const std::string& target_ip, const std::string& path, bool save_result = false, const std::string& save_path = ""){
      LoadScanConfig(path);
      ExecuteSweep(target_ip, save_result, save_path);
   }

   private:
   /**
   * @brief prints the timestamp in 24 Hour format if timestamping is enabled.
   * @return void
   */
   inline void timestamp(){
      if (!timestampEnabled) return;
      DateTime now = rtc.now();
      Serial.printf("[%s%d-%d-%d %d:%d:%d%s] ", Color::CYAN, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), Color::RESET);
   }

   private:
   /**
    * @brief Background FreeRTOS thread dedicated exclusively to writing 8KB chunks to the SD card.
    * @param arg Void pointer cast back to the scanner instance.
    * @paragraph The Poison Pill
    * This task sleeps completely (0% CPU usage) using `portMAX_DELAY` until the Interceptor sends a message 
    * through the queue. If it receives a message with `length == 0`, that acts as a "Poison Pill", forcing 
    * the task to gracefully terminate itself when the user stops the sniffer.
    */
   static void sniffer_writer_task(void* arg) {
      NYXUS_SCANNER* instance = static_cast<NYXUS_SCANNER*>(arg);
      SnifferFlushMsg msg;
      while(true) {
         if (xQueueReceive(instance->sniffer_flush_queue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.length == 0) break;
            if (instance->sniffer_file) {
               instance->sniffer_file.write(msg.buffer, msg.length);
            }
         }
      }
      vTaskDelete(NULL);
   }

   /**
    * @brief High-speed hardware callback triggered on every raw 802.11 frame captured by the antenna.
    * @param buf Pointer to the raw ESP-IDF packet structure.
    * @param type The physical layer frame type (Management, Control, or Data).
    * @paragraph Zero-Copy Header Injection
    * This callback manually constructs the `PCAP::PacketHeader` on the fly using the ESP32's microsecond 
    * hardware timer. It injects the header directly into the SRAM ring buffer immediately before 
    * injecting the raw radio payload, formatting the data for Wireshark natively.
    */
   static void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
      if (!active_sniffer_instance || !active_sniffer_instance->is_sniffing) return;
      wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
      uint32_t len = pkt->rx_ctrl.sig_len;
      size_t total_required = sizeof(PCAP::PacketHeader) + len;
      if (active_sniffer_instance->active_rx_offset + total_required > SNIFFER_BUFFER_SIZE) {
         SnifferFlushMsg msg = {
            active_sniffer_instance->active_rx_buffer, 
            active_sniffer_instance->active_rx_offset
         };
         if (xQueueSendFromISR(active_sniffer_instance->sniffer_flush_queue, &msg, NULL) == pdTRUE) {
            active_sniffer_instance->active_rx_buffer = (active_sniffer_instance->active_rx_buffer == active_sniffer_instance->sniffer_buffer_A) ? active_sniffer_instance->sniffer_buffer_B : active_sniffer_instance->sniffer_buffer_A;
         }
         active_sniffer_instance->active_rx_offset = 0;
      }
      int64_t time_us = esp_timer_get_time();
      PCAP::PacketHeader hdr = {
         (uint32_t)(time_us / 1000000LL), // Seconds
         (uint32_t)(time_us % 1000000LL), // Microseconds
         len, len
      };
      memcpy(active_sniffer_instance->active_rx_buffer + active_sniffer_instance->active_rx_offset, &hdr, sizeof(hdr));
      active_sniffer_instance->active_rx_offset += sizeof(hdr);
      memcpy(active_sniffer_instance->active_rx_buffer + active_sniffer_instance->active_rx_offset, pkt->payload, len);
      active_sniffer_instance->active_rx_offset += len;
   }

   public:
   /**
    * @brief Ignites the Promiscuous Mode radio filter and spawns the background logging tasks.
    * @param save_path The absolute path to save the `.pcap` file.
    * @return void
    */
   void StartSniffing(const std::string& save_path) {
      if (is_sniffing) return;
      if (!sd) {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No SD mount detected for PCAP storage.\n", Color::RED, Color::RESET); }
         return;
      }
      sniffer_file = sd->open(save_path.c_str(), O_WRITE | O_CREAT | O_TRUNC);
      if (!sniffer_file) {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] Failed to create PCAP database.\n", Color::RED, Color::RESET); }
         return;
      }
      if (verbose) {
         Serial.println("===============================");
         timestamp(); Serial.printf("[%sSNIFFER%s] Injecting global PCAP cryptographic headers...\n", Color::YELLOW, Color::RESET);
      }
      sniffer_file.write((const uint8_t*)&PCAP::Standard80211, sizeof(PCAP::GlobalHeader));
      sniffer_flush_queue = xQueueCreate(4, sizeof(SnifferFlushMsg)); // Can hold 4 buffers in backlog
      xTaskCreatePinnedToCore(sniffer_writer_task, "PCAP_Writer", 4096, this, 1, &sniffer_writer_task_handle, 1);
      active_sniffer_instance = this;
      active_rx_buffer = sniffer_buffer_A;
      active_rx_offset = 0;
      is_sniffing = true;
      wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL }; // Catch Data, Management, and Control Frames
      esp_wifi_set_promiscuous_filter(&filter);
      esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
      esp_wifi_set_promiscuous(true);
      if (verbose) { timestamp(); Serial.printf("[%sSUCCESS%s] Promiscuous Mode Armed. Vacuuming packets.\n", Color::GREEN, Color::RESET); }
   }

   public:
   /**
    * @brief Safely terminates the Promiscuous Mode vacuum and finalizes the PCAP file.
    * @return void
    */
   void StopSniffing() {
      if (!is_sniffing) return;
      esp_wifi_set_promiscuous(false);
      esp_wifi_set_promiscuous_rx_cb(NULL);
      is_sniffing = false;
      if (active_rx_offset > 0) {
         SnifferFlushMsg msg = { active_rx_buffer, active_rx_offset };
         xQueueSend(sniffer_flush_queue, &msg, portMAX_DELAY);
      }
      SnifferFlushMsg poison_pill = { nullptr, 0 };
      xQueueSend(sniffer_flush_queue, &poison_pill, portMAX_DELAY);
      delay(init_delay_time);
      vQueueDelete(sniffer_flush_queue);
      if (sniffer_file) sniffer_file.close();
      if (verbose) { 
         timestamp(); Serial.printf("[%sSUCCESS%s] Sniffer disarmed. PCAP file finalized.\n", Color::GREEN, Color::RESET); 
         Serial.println("===============================");
      }
   }

   public:
   /**
    * @brief Dynamically alters the active 2.4GHz physical radio channel.
    * @param channel Target frequency channel (1 - 13).
    * @paragraph Deauth / Handshake Mapping
    * Promiscuous mode only intercepts radio waves on the *currently active channel*. 
    * By cycling this method, you can build a channel-hopping script to survey the entire area, 
    * or lock onto a specific router's channel to capture WPA2/WPA3 4-way EAPOL handshakes.
    * @return void
    */
   void SetChannel(uint8_t channel) {
      if (channel >= 1 && channel <= 13) {
         esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
         if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Radio hopped to Channel %d\n", Color::CYAN, Color::RESET, channel); }
      }
   }
   
   public:
   /**
   * @brief Loads the scanning configuration file via the hardware parser.
   * @param path The absolute path of the .scnconf file.
   * @return void
   * @attention The file extension MUST be `.scnconf`.
   */
   void LoadScanConfig(const std::string& path){
      if (!sd){
         if (verbose) {
            timestamp(); Serial.printf("[%sERROR%s] no mount detected\n", Color::RED, Color::RESET);
         }
         return;
      }
      if (!sd->exists(path.c_str())){
         if (verbose) {
            timestamp(); Serial.printf("[%sERROR%s] %s%s%s does not exist!\n", Color::RED, Color::RESET, Color::YELLOW, path.c_str(), Color::RESET );
         }
         return;
      }
      if (verbose) {
         timestamp(); Serial.printf("[%sINFO%s] Delegating %s to NYXUS_FILE_PARSER...\n", Color::YELLOW, Color::RESET, path.c_str());
      }
      NYXUS_FILE_PARSER::Parse(sd, path, &scanConfig);
      if (verbose) {
         timestamp(); Serial.printf("[%sSUCCESS%s] Scanning configuration injected into SRAM.\n", Color::GREEN, Color::RESET);
         //displayScanConfig();
         // Optional: You could call a  here to verify the parsed parameters
      }
   }
   
   public:
   /**
   * @brief Serializes the active Scanning Configuration Profile from SRAM to persistent FAT32 storage.
   * @param path The absolute path and filename (excluding extension) to save to.
   * @return void
   */
   void SaveScanConfig(const std::string& path){
      if (!sd){
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
         return;
      }
      std::string filepath = path + Extension::scnconf;
      FsFile file = sd->open(filepath.c_str(), O_WRITE | O_CREAT | O_TRUNC);
      if (!file){
         if (verbose){
            timestamp(); Serial.printf("[%sERROR%s] I/O failure on file: %s\n", Color::RED, Color::RESET, filepath.c_str());
         }
         return;
      }
      file.printf("-t %lld\n", scanConfig.timeout);
      file.printf("-m %d\n", scanConfig.mode);
      file.printf("-s %d\n", scanConfig.speed);
      file.printf("-r %d\n", scanConfig.retry);
      if (scanConfig.verbose) file.printf("-v\n");
      if (scanConfig.timestampEnabled) file.printf("-T\n");
      if (scanConfig.savePath.has_value()) file.printf("-P %s\n", scanConfig.savePath.value().c_str());
      for (uint16_t port : scanConfig.ports) {
         file.printf("-p %u\n", port);
      }
      file.close();
      if (verbose){
         timestamp(); Serial.printf("[%sSUCCESS%s] Scan configuration serialized to %s\n", Color::GREEN, Color::RESET, filepath.c_str());
      }
   }

   public:
   /**
   * @brief Purges the active Nmap-style scanning parameters and restores default reconnaissance settings.
   * @return void
   */
   void ResetScanConfig(){
      scanConfig = ScanningConfig{
         .savePath = std::nullopt,
         .ports = {},
         .timeout = 1000,
         .sd = nullptr,
         .mode = ScanMode::NONE,
         .speed = ScanSpeed::MEDIUM,
         .retry = 0,
         .verbose = false,
         .timestampEnabled = false
      };
      if (verbose){
         timestamp(); Serial.printf("[%sINFO%s] Scanning configuration restored to factory baseline.\n", Color::YELLOW, Color::RESET);
      }
   }
   
   public:
   /**
   * @brief Manually overwrites the active scanning configuration profile from RAM.
   * @param conf The new ScanningConfig struct to inject.
   * @return void
   */
   void ChangeScanConfig(const ScanningConfig &conf){
      scanConfig = conf;
      if (verbose){
         timestamp(); Serial.printf("[%sINFO%s] Scanning configuration manually updated in SRAM.\n", Color::YELLOW, Color::RESET);
      }
   }

   public:
   /**
   * @brief Permanently deletes a specific scanning configuration file from the SD card.
   * @param path The absolute path to the `.scnconf` file (excluding extension).
   * @return void
   */
   void Delete_ScanConfig(const std::string &path) {
      std::string filepath = path + Extension::scnconf;
      if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Initiating deletion of scan config: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
      DestroyFile(filepath);
   }
   
   private:
   /**
   * @brief Internal hardware method to destroy a file on the FAT32 volume.
   * @param filepath The absolute path of the file.
   * @return void
   */
   void DestroyFile(const std::string& filepath) {
      if (!sd) {
         if (verbose) { timestamp(); Serial.printf("[%sERROR%s] No mount detected.\n", Color::RED, Color::RESET); }
         return;
      }
      if (sd->exists(filepath.c_str())) {
         if (sd->remove(filepath.c_str())) {
            if (verbose) { timestamp(); Serial.printf("[%sSUCCESS%s] File obliterated from disk: %s\n", Color::GREEN, Color::RESET, filepath.c_str()); }
         } else {
            if (verbose) { timestamp(); Serial.printf("[%sERROR%s] SPI Bus lock prevented deletion of: %s\n", Color::RED, Color::RESET, filepath.c_str()); }
         }
      } else {
         if (verbose) { timestamp(); Serial.printf("[%sINFO%s] Target file does not exist: %s\n", Color::YELLOW, Color::RESET, filepath.c_str()); }
      }
   }

};
#endif