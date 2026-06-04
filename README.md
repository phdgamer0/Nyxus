# Nyxus Core

Nyxus is an enterprise-grade, baremetal network exploitation and terminal graphics framework engineered specifically for ESP32/Xtensa silicon. It completely bypasses standard high-level network stacks, utilizing direct DMA transfers, LwIP raw sockets, and custom zero-allocation memory structures to maximize execution speed and protect SRAM.

> **Status:** Private / Active Development. 

## Core Architecture Architecture

The framework is highly modular, split across distinct header engines that can be invoked independently or orchestrated together.

### 1. Network Protocol Engine
* **`nyx_network_types.hpp`**: The strict RFC-compliant structural backbone. Contains zero-padded, `__attribute__((packed))` wire protocols for raw Ethernet, ARP, UDP, TCP, PCAP generation, and DNS manipulation.
* **`nyx_wifi.hpp`**: The Layer 2 access controller. Handles ESP-IDF event loops, BSSID mapping, assertive (Rogue AP) deployments, and hardware-accelerated dictionary attacks from SD storage.

### 2. Reconnaissance & Interception
* **`nyx_scanner.hpp`**: An active network sweeper utilizing LwIP `SOCK_RAW` sockets to forge TCP SYN and UDP datagrams. Includes a Promiscuous Mode interceptor driven by a double-buffered FreeRTOS task, capturing raw 802.11 frames to `.pcap` files via direct DMA-to-SD transfers.
* **`nyx_mitm.hpp`**: The Man-in-the-Middle command module. Executes dual-vector ARP cache poisoning (Layer 2.5 bypassing WPA2 encryption logic) and Evil Twin DNS spoofing (intercepting Port 53 to force Captive Portal redirects).

### 3. Filesystem & Telemetry
* **`nyx_file_parser.hpp`**: An O(1) hardware parser for dynamically loading configuration profiles (`.scnconf`, `.connconf`) from FAT32/exFAT volumes without fragmenting the heap.
* **`nyx_file_expplorer.hpp`**: A memory-safe, asynchronous FAT32 directory iterator designed to feed terminal interfaces or external UI renderers without exhausting FreeRTOS file handles.

### 4. Terminal UI Engine
* **`nyx_terminal_graphics.hpp`**: A zero-allocation ANSI escape sequence generator. Replaces heavy `printf` buffers with custom circular ring-buffers and base-10 bitwise math to render terminal interfaces (TUIs) over serial lines at extreme speeds.

## License
This software is provided under the **Nyxus Source-Available Non-Derivative License**. See the `LICENSE` file for details. Usage is permitted; modification and redistribution of derivative works are strictly prohibited.
