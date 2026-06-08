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
#ifndef _NYXUS_NETWORK_TYPES_HPP_
#define _NYXUS_NETWORK_TYPES_HPP_

#include <IPAddress.h>
#include <vector>
#include <string>
#include <optional>
#include <SdFat.h>
#include <cstdint>
static inline constexpr uint8_t MAC_SIZE = 0x06;

/**
* @namespace WifiMode
* @brief Defines hardware radio state macros for the ESP32-S3 Wi-Fi silicon.
*/
namespace WifiMode{
   inline constexpr auto connective = WIFI_MODE_STA;     /**< Station Mode: Operates as a client to infiltrate or connect to existing infrastructure.     */
   inline constexpr auto assertive  = WIFI_MODE_AP;      /**< Access Point Mode: Broadcasts a localized network, acting as the master router.             */
   inline constexpr auto passive    = WIFI_MODE_APSTA;   /**< Hybrid Mode: Simultaneously hosts an AP while maintaining a client uplink.                  */
   inline constexpr auto invasive   = WIFI_MODE_MAX;     /**< Promiscuous Mode: Bypasses standard LwIP routing for raw packet injection and sniffing.     */
   inline constexpr auto off        = WIFI_MODE_NULL;    /**< Radio Silence: Physically powers down the 2.4GHz antenna to evade detection and save power. */
}

/**
* @struct ConnectionConfig
* @brief Defines the static IP overrides, routing payloads, and timeout limits for the network interface.
* @note Operates as an aggregate struct to guarantee memory safety during designated initialization.
*/
struct ConnectionConfig{
   long long connection_wait_time_ms;   /**< The hardware timeout threshold (in milliseconds) before abandoning a handshake attempt.  */
   IPAddress ip1;                       /**< The spoofed or assigned static IPv4 address presented to the target network.             */
   IPAddress gateway;                   /**< The target network's primary gateway router address for outbound traffic.                */
   IPAddress subnet;                    /**< The subnet mask payload to define the local network boundaries.                          */
   IPAddress dns1;                      /**< Primary Domain Name System server address override.                                      */
   IPAddress dns2;                      /**< Secondary Domain Name System server fallback address.                                    */
   uint8_t retry_amount;                /**< Maximum number of sequential authentication attempts before declaring target exhaustion. */
};

/**
* @namespace ScanMode
* @brief Bitmask definitions for network reconnaissance and raw packet injection protocols.
*/
namespace ScanMode{
   inline constexpr uint8_t NONE    = 0x00U;   /**< Idle state; no active packet transmission.                                                 */
   inline constexpr uint8_t SYN     = 0x01U;   /**< Half-open stealth scan (SYN only); prevents target systems from logging a full connection. */
   inline constexpr uint8_t SYN_ACK = 0x02U;   /**< Full TCP handshake verification; highly accurate but leaves heavy forensic footprints.     */
   inline constexpr uint8_t UDP_IN  = 0x04U;   /**< Ingress UDP mapping; listens for unsolicited incoming datagrams on defined ports.          */
   inline constexpr uint8_t UDP_OUT = 0x08U;   /**< Egress UDP sweeping; blasts connectionless payloads to map open routing ports.             */
   inline constexpr uint8_t TCP_IN  = 0x10U;   /**< Ingress TCP mapping; captures and logs incoming TCP connection requests.                   */
   inline constexpr uint8_t TCP_OUT = 0x20U;   /**< Egress TCP sweeping; actively probes outbound routing rules and firewalls.                 */
   inline constexpr uint8_t ALL     = 0x3FU;   /**< Omnidirectional assault; activates all reconnaissance vectors simultaneously.              */
}

/**
* @namespace ScanSpeed
* @brief Bitmask definitions for network scanning velocity, dictating packet injection delays and hardware timer frequencies.
*/
namespace ScanSpeed{
   inline constexpr uint8_t SLOW      = 0x3FU;   /**< High latency (T0/T1): Maximizes stealth by blending into background noise. Avoids IDS triggers.       */
   inline constexpr uint8_t MEDIUM    = 0x7FU;   /**< Standard latency (T2/T3): Balanced transmission approach for normal network topologies.               */
   inline constexpr uint8_t FAST      = 0xBFU;   /**< Low latency (T4): Aggressive timing profile; risks packet drops on congested networks.                */
   inline constexpr uint8_t VERY_FAST = 0xFFU;   /**< Zero latency (T5): Floods the interface at maximum hardware limits; highly noisy and easily detected. */
}

/**
* @struct ScanningConfig
* @brief Configuration payload for orchestrating low-level Nmap-style packet sweeps.
*/
struct ScanningConfig {
   std::optional<std::string> savePath;   /**< Optional absolute filepath for writing PCAP/log data.                      */
   std::vector<uint16_t> ports;           /**< Vector of target ports (uint16_t allows mapping up to port 65535).         */
   long long timeout;                     /**< Maximum wait time (in milliseconds) for a target to respond.               */
   SdFat* sd = nullptr;                   /**< Hardware pointer to the filesystem for logging. Nullptr disables disk I/O. */
   uint8_t mode;                          /**< Target packet injection type (SYN, UDP, etc.).                             */
   uint8_t speed;                         /**< Timing profile for packet transmission.                                    */
   uint8_t retry;                         /**< Maximum retransmissions before marking a port as filtered/closed.          */
   bool verbose;                          /**< Toggles serial telemetry output.                                           */
   bool timestampEnabled;                 /**< Flags whether to append RTC timestamps to output.                          */
};

/**
* @namespace PCAP
* @brief Constants and packed structures for creating Wireshark-compatible capture files.
*/
namespace PCAP {
   /**
   * @struct GlobalHeader
   * @brief The mandatory cryptographic file header that identifies a file as a PCAP format database.
   * @note Packed to prevent compiler alignment padding from corrupting the Wireshark read offsets.
   * @paragraph PCAP Architecture
   * This 24-byte structure must be written exactly once at the very top of the `.pcap` file. 
   * It defines the byte order, versioning, and the data link type (Network layer protocol). 
   * By setting the network link to 105, we instruct Wireshark to decode the subsequent payloads 
   * as raw IEEE 802.11 wireless frames (which includes MAC addresses, beacons, and handshakes).
   */
   struct __attribute__((packed)) GlobalHeader {
      uint32_t magic_number ; /**< magic_number (Identifies PCAP endianness)       */
      uint16_t version_major; /**< version_major                                   */
      uint16_t version_minor; /**< version_minor                                   */
      int32_t  thiszone;      /**< thiszone (GMT to local)                         */
      uint32_t sigfigs;       /**< sigfigs                                         */
      uint32_t snaplen;       /**< snaplen (Max packet length)                     */
      uint32_t network;       /**< network (105 = IEEE 802.11 Wireless MAC frames) */
   };

   /**
    * @brief Compile-time baked 802.11 PCAP Header. 
    * @note Stored directly in physical Flash Memory (.rodata). Costs 0 bytes of dynamic SRAM.
    */
   inline constexpr GlobalHeader Standard80211 = {
      0xa1b2c3d4, /**< Magic number used by Wireshark to detect PCAP file format and endianness.   */ 
      2,          /**< Major version number of the PCAP format. Always explicitly 2.               */
      4,          /**< Minor version number of the PCAP format. Always explicitly 4.               */
      0,          /**< GMT to local timezone correction. Usually set to 0 (timestamps act as UTC). */
      0,          /**< Accuracy of timestamps. In practice, modern parsers expect this to be 0.    */
      65535,      /**< Maximum length of captured packets (Snapshot Length). Prevents truncation.  */
      105         /**< Data link type. 105 specifically dictates IEEE 802.11 Wireless MAC frames.  */
   };

   /**
   * @struct pcap_packet_header
   * @brief The localized header prepended to every single captured frame within the PCAP file.
   * @note Packed to guarantee exactly 16 bytes of overhead per packet.
   * @paragraph Hardware Capture Overhead
   * In promiscuous mode, the ESP32 will capture raw RF data. Before writing that payload to the 
   * SD card ring buffer, this 16-byte header must be constructed and written first. It provides 
   * Wireshark with the exact microsecond timestamp of the capture and tells the parser exactly 
   * how many bytes to read before expecting the next packet header.
   */
   struct __attribute__((packed)) PacketHeader {
      uint32_t ts_sec;   /**< Capture timestamp in absolute seconds since the Unix Epoch.                                    */
      uint32_t ts_usec;  /**< Capture timestamp microsecond fraction. Critical for sequential analysis in Wireshark.         */
      uint32_t incl_len; /**< Number of bytes of the packet actually saved in the file (Should match orig_len in our setup). */
      uint32_t orig_len; /**< Actual length of the packet as it appeared on the physical radio wire.                         */
   };
}

/**
* @namespace MITM
* @brief Raw data-link layer structures for Man-in-the-Middle network manipulation.
*/
namespace MITM {
   /**
   * @struct ethernet_header
   * @brief Standard IEEE 802.3 Ethernet frame header for Layer 2 injection.
   */
   struct __attribute__((packed)) ethernet_header {
      uint8_t  dest_mac[MAC_SIZE];
      uint8_t  src_mac[MAC_SIZE];
      uint16_t ethertype; // 0x0806 for ARP, 0x0800 for IPv4
   };

   /**
   * @struct arp_header
   * @brief Address Resolution Protocol payload for cache poisoning and network routing manipulation.
   */
   struct __attribute__((packed)) arp_header {
      uint16_t hardware_type;        /**< 0x0001 for Ethernet                    */  
      uint16_t protocol_type;        /**< 0x0800 for IPv4                        */
      uint8_t  hardware_size;        /**< 6 (MAC length)                         */  
      uint8_t  protocol_size;        /**< 4 (IPv4 length)                        */  
      uint16_t opcode;               /**< 1 for Request, 2 for Reply (Spoof)     */  
      uint8_t  sender_mac[MAC_SIZE]; /**< The MAC address we want them to trust  */   
      uint32_t sender_ip;            /**< The IP address we are pretending to be */
      uint8_t  target_mac[MAC_SIZE]; /**< The victim's MAC                       */   
      uint32_t target_ip;            /**< The victim's IP                        */    
   };

   /**
    * @struct dns_header
    * @brief Standard 12-byte Domain Name System query/response header.
    * @note Packed to ensure strict RFC 1035 alignment.
    */
   struct __attribute__((packed)) dns_header {
      uint16_t id;         /**< Transaction ID to match request with response. */
      uint16_t flags;      /**< Bitmask for Query/Response flags and Error codes. */
      uint16_t qdcount;    /**< Number of questions. */
      uint16_t ancount;    /**< Number of answer resource records. */
      uint16_t nscount;    /**< Number of authority resource records. */
      uint16_t arcount;    /**< Number of additional resource records. */
   };

   /**
    * @struct dns_answer_trailer
    * @brief 16-byte DNS Answer payload for IPv4 (A-Record) spoofing.
    * @paragraph DNS Pointer Compression
    * To save SRAM, DNS uses a compression trick. The `name_ptr` (usually 0xC00C) 
    * tells the victim's browser: "The domain name you asked for is located at byte 
    * offset 12 of this packet." This allows us to answer ANY query instantly without 
    * manually parsing or rewriting the requested domain string.
    */
   struct __attribute__((packed)) dns_answer_trailer {
      uint16_t name_ptr;  /**< Byte offset pointer (0xC00C) pointing back to the queried name. */
      uint16_t type;      /**< 0x0001 (A record / IPv4).                                       */
      uint16_t cls;       /**< 0x0001 (IN / Internet Class).                                   */
      uint32_t ttl;       /**< Time-to-Live (Seconds) before cache expires.                    */
      uint16_t data_len;  /**< Length of the IP address (0x0004 for IPv4).                     */
      uint32_t ip_addr;   /**< The malicious IP address we are injecting.                      */
   };
}

/**
* @namespace Extension
* @brief Registry of parseable file extensions and their formatting rules.
*/
namespace Extension{
   /**
   * @brief Target payload dictionary mapping extension.
   * @note Utilized by the `NYXUS_WIFI` dictionary attack vectors.
   * * @attention Parsing Constraints:
   * * @attention - Lines starting with `//` are completely ignored (Comments).
   * * @attention - Whitespaces are strictly preserved (Passwords may contain spaces).
   * * @attention - Enforces a strict one-password-per-line structure.
   */
   inline constexpr const char* ssidpswd = ".ssidpswd";
   
   /**
   * @brief Network reconnaissance configuration file extension.
   * @note Utilized by the `NYXUS_SCANNER` LwIP packet sweeping engine.
   * * @attention Parsing Constraints:
   * * @attention - Lines starting with `//` are completely ignored.
   * * @attention - ALL whitespaces, tabs, and carriage returns are aggressively stripped.
   * * @attention - Arguments can be supplied in any arbitrary order using standard CLI flags.
   * * @attention - Valid Hardware Flags:
   * * * @attention - - `-p` : `std::vector<uint16_t> ports` (Supply each port on a new line)
   * * * @attention - - `-t` : `long long timeout` (in milliseconds)
   * * * @attention - - `-P` : `std::optional<std::string> savePath` (Absolute SD path for PCAP)
   * * * @attention - - `-m` : `uint8_t mode` (Accepts decimal or 0x HEX for ScanMode)
   * * * @attention - - `-s` : `uint8_t speed` (Accepts decimal or 0x HEX for ScanSpeed)
   * * * @attention - - `-r` : `uint8_t retry` (Retransmission threshold)
   * * * @attention - - `-v` : `bool verbose` (Presence enables telemetry)
   * * * @attention - - `-T` : `bool timestampEnabled` (Presence enables RTC timestamping)
   * * @attention - Flag Behaviors:
   * * * @attention - - Suffixing a flag with `-` immediately negates/clears it (e.g., `-v-` disables verbosity).
   * * * @attention - - Duplicate flags overwrite previous entries in memory.
   */
   inline constexpr const char* scnconf = ".scnconf";

   /**
   * @brief Connection configuration file extension.
   * @note Utilized by the `NYXUS_WIFI` standard routing engine.
   * * @attention Parsing Constraints:
   * * @attention - Lines starting with `//` are completely ignored.
   * * @attention - ALL whitespaces, tabs, and carriage returns are aggressively stripped.
   * * @attention - Valid Hardware Flags:
   * * * @attention - - `-w` : `long long connection_wait_time_ms`
   * * * @attention - - `-r` : `uint8_t retry_amount`
   * * * @attention - - `-i` : `IPAddress ip1` (Spoofed IP)
   * * * @attention - - `-g` : `IPAddress gateway`
   * * * @attention - - `-s` : `IPAddress subnet`
   * * * @attention - - `-d` : `IPAddress dns1`
   * * * @attention - - `-D` : `IPAddress dns2`
   * * @attention - Flag Behaviors:
   * * * @attention - - Suffixing a flag with `-` immediately negates/clears it (e.g., `-d-` sets dns1 to 0.0.0.0).
   * * * @attention - - Duplicate flags overwrite previous entries in memory.
   * * @attention - IP parsing natively supports standard IPv4 strings (e.g., "192.168.1.1").
   */
   inline constexpr const char* connconf = ".connconf";
}

#endif