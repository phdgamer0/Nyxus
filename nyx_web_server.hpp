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

#ifndef _NYXUS_WEB_SERVER_HPP_
#define _NYXUS_WEB_SERVER_HPP_

#include <Nyxus/nyx_terminal_graphics.hpp>
#include <SdFat.h>
#include <array>
#include <cstdint>
#include <esp_http_server.h>
#include <esp_https_ota.h>
#include <esp_https_server.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <RTClib.h>
#include <string_view>
#include <sys/param.h>

using socket_limit = uint8_t;
using uri_limit = uint8_t;
using core_id = uint8_t;

inline constexpr core_id THREAD_CORE_AFFINITY_0 = 0;
inline constexpr core_id THREAD_CORE_AFFINITY_1 = 1;

inline constexpr socket_limit MAX_SOCKETS_POSSIBLE = 8;
inline constexpr socket_limit MIN_SOCKETS_POSSIBLE = 1;

/**
 * @struct HttpConfig
 * @brief Defines the operational parameters and hardware limits for the HTTP server.
 */
template <uri_limit URI_SIZE, core_id CORE_NUMBER = THREAD_CORE_AFFINITY_0, size_t STACK_SIZE = 4096, socket_limit MAX_CONNECTIONS = 4>
struct HttpConfig {
   void* global_user_ctx = nullptr; /**< Pointer to user-defined context passed to static URI handlers.               */
   uint32_t IP = 0;                 /**< The Ip to be appeard as, dont touch if you arent running MITM or injectrions */
   uint16_t send_timeout = 5;       /**< The maximum time, in seconds, the server will block waiting to send data.    */
   uint16_t recv_timeout = 5;       /**< The maximum time, in seconds, the server will block waiting to receive data. */
   uint16_t port = 80;              /**< The network port on which the server listens for incoming TCP connections.   */
};

/**
 * @class NYXUS_HTTP_SERVER
 * @brief Manages the HTTP server lifecycle, connection routing, and filesystem streaming.
 */
template <uri_limit URI_SIZE, core_id CORE_NUMBER = THREAD_CORE_AFFINITY_0, size_t STACK_SIZE = 4096, socket_limit MAX_CONNECTIONS = 4, size_t BUFF_SIZE = 1024>
class NYXUS_HTTP_SERVER {
   private:
   /**
   * @brief The native ESP-IDF server instance handle.
   */
   httpd_handle_t server = nullptr;

   private:
   /**
   * @brief The configuration structure containing templated hardware limits.
   */
   HttpConfig<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS> config;

   private:
   /**
   * @brief Pointer to the global FAT32 filesystem instance.
   */
   SdFat* sd;

   private:
   /**
   * @brief Pointer to the global real time clock if available
   */
   RTC_DS3231* rtc;

   private:
   /**
   * @brief The absolute root directory path on the filesystem used for resolving file requests.
   */
   char base_path[64]{};

   private:
   bool verbose;

   public:
   /**
   * @brief Constructs the HTTP server instance.
   * @paragraph
   * Initializes the server object with a reference to the active filesystem.
   * The server daemon is not started until Start() is explicitly called.
   * @param sd_ptr Pointer to an initialized SdFat instance.
   */
   NYXUS_HTTP_SERVER(bool verbose = true, SdFat* sd_ptr = nullptr, RTC_DS3231* rtc = nullptr) : verbose(verbose), sd(sd_ptr), rtc(rtc) {}

   public:
   /**
   * @brief Allocates and starts the FreeRTOS server task.
   * @paragraph
   * This method initializes the internal ESP-IDF server structures, applies the limits
   * defined in the HttpConfig template, and binds to the specified port. It copies the
   * provided root path into the internal buffer for safe concurrent access.
   * @param port The TCP port to bind the server to.
   * @param sd_root_path The root directory on the SD card for serving files. Defaults to nullptr.
   * @return true if the server task was allocated and started successfully.
   * @return false if the task allocation failed due to memory exhaustion.
   */
   [[nodiscard]] inline bool Start(uint16_t port, const char* __restrict sd_root_path = nullptr);

   public:
   /**
   * @brief Terminates the server task and releases allocated network resources.
   * @paragraph
   * Closes all active client sockets, deregisters all URI handlers, and frees the
   * associated FreeRTOS task memory. The server handle is safely reset to nullptr.
   * @return void
   */
   inline void Stop();

   public:
   /**
   * @brief Dynamically attaches a URI route to the server.
   * @param uri The endpoint string.
   * @param method The HTTP method (e.g., HTTP_GET, HTTP_POST).
   * @param handler The static C-callback function to execute.
   * @return true if the route was successfully bound to the daemon.
   */
   [[nodiscard]] inline bool AddRoute(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *));

   private:
   /**
   * @brief prints the timestamp in 24 Hour format if timestamping is enabled.
   * @return void
   */
   inline void timestamp();

   public:
   /**
   * @brief Transmits a static string payload to the connected client.
   * @paragraph
   * Used for sending brief textual responses or JSON payloads directly from SRAM
   * without accessing the filesystem.
   * @param req Pointer to the active HTTP request structure.
   * @param str The null-terminated character array to transmit.
   * @return esp_err_t ESP_OK on success, or an appropriate ESP error code on failure.
   */
   [[nodiscard]] inline esp_err_t Stream(httpd_req_t* __restrict req, const char* __restrict str);

   private:
   /**
   * @brief Streams a file from the SD card to the connected client in optimized chunks.
   * @paragraph
   * Opens the specified file and reads it into the stack-allocated buffer defined by BUFF_SIZE.
   * Utilizes chunked transfer encoding to transmit large files without fragmenting the heap.
   * @param req Pointer to the active HTTP request structure.
   * @param file_path The relative path to the requested file on the SD card.
   * @return esp_err_t ESP_OK on successful transmission, or ESP_FAIL if the file cannot be read.
   */
   [[nodiscard]] inline esp_err_t StreamFromSD(httpd_req_t* __restrict req, const char* __restrict file_path);

   private:
   /**
   * @brief Intercepts wildcard requests to enforce captive portal redirection.
   * @paragraph
   * A static callback function that processes unmapped DNS/HTTP requests and routes
   * the client to the default index file, enforcing the captive portal behavior.
   * @param req Pointer to the active HTTP request structure.
   * @return esp_err_t ESP_OK after successfully streaming the redirection payload.
   */
   [[nodiscard]] static inline esp_err_t CaptivePortalHandler(httpd_req_t* __restrict req);
};

/* IMPLEMENTATIONS - NOTHING HERE */

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
[[nodiscard]] inline bool NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::Start(uint16_t port, const char* __restrict sd_root_path) {
   if (server != nullptr) return true;
   if (sd_root_path) strlcpy(base_path, sd_root_path, sizeof(base_path));
   config.port = port;
   config.global_user_ctx = this;
   httpd_config_t def = HTTPD_DEFAULT_CONFIG();
   def.server_port = config.port;
   def.global_user_ctx = config.global_user_ctx;
   def.core_id = CORE_NUMBER;
   def.stack_size = STACK_SIZE;
   def.max_open_sockets = MAX_CONNECTIONS;
   def.max_uri_handlers = URI_SIZE;
   def.recv_wait_timeout = config.recv_timeout;
   def.send_wait_timeout = config.send_timeout;
   def.uri_match_fn = httpd_uri_match_wildcard;
   if (httpd_start(&server, &def) == ESP_OK) {
      char ip_str[16] = "0.0.0.0";
      esp_netif_ip_info_t ip_info;
      esp_netif_t* netif = nullptr;
      wifi_mode_t mode;
      if (esp_wifi_get_mode(&mode) == ESP_OK) {
         if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
         else netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      }
      if (netif != nullptr) {
         esp_netif_get_ip_info(netif, &ip_info);
         esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
      }
      Serial.printf("[%sHTTP%s] Daemon ignited on Core %d.\n", Color::GREEN, Color::RESET, CORE_NUMBER);
      timestamp();
      Serial.printf("[%sHTTP%s] Target Access: %shttp://%s:%d%s\n", Color::GREEN, Color::RESET, Color::CYAN, ip_str, port, Color::RESET);
      if (sd_root_path) {
         timestamp();
         Serial.printf("[%sHTTP%s] SD Card Mount: %s\n", Color::GREEN, Color::RESET, base_path);
      }
      return true;
   }
   timestamp();
   Serial.printf("[%sERROR%s] Failed to allocate HTTP daemon task.\n", Color::RED, Color::RESET);
   return false;
}

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
inline void NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::Stop() {
   if (server) {
      httpd_stop(server);
      server = nullptr;
   }
}

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
[[nodiscard]] inline bool NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::AddRoute(const char* uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *)) {
   if (!server) return false;
   httpd_uri_t uri_{};
   uri_.uri = uri;
   uri_.handler = handler;
   uri_.method = method;
   uri_.user_ctx = config.global_user_ctx;
   return httpd_register_uri_handler(server, &uri_) == ESP_OK;
}

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
inline void NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::timestamp(){
   if (!rtc) return;
   DateTime now = rtc->now();
   Serial.printf("[%s%d-%d-%d %d:%d:%d%s] ", Color::CYAN, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(), Color::RESET);
}

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
[[nodiscard]] inline esp_err_t NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::Stream(httpd_req_t* __restrict req, const char* __restrict str) {
   return httpd_resp_sendstr(req, str);
}

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
[[nodiscard]] inline esp_err_t NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::StreamFromSD(httpd_req_t* __restrict req, const char* __restrict file_path) {
   char full_path[128] = {0};
   snprintf(full_path, sizeof(full_path), "%s%s", base_path, file_path);
   FsFile file = sd->open(full_path, O_READ);
   if (!file) {
      if (verbose) {
         timestamp();
         Serial.printf("[%sHTTP%s] 404 File Not Found: %s\n", Color::RED, Color::RESET, full_path);
      }
      httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Payload file not found on SD card.");
      return ESP_FAIL;
   }
   const char* dot = strrchr(file_path, '.');
   if (dot) {
      if (strcmp(dot, ".html") == 0) httpd_resp_set_type(req, "text/html");
      else if (strcmp(dot, ".css") == 0) httpd_resp_set_type(req, "text/css");
      else if (strcmp(dot, ".js") == 0) httpd_resp_set_type(req, "application/javascript");
      else if (strcmp(dot, ".png") == 0) httpd_resp_set_type(req, "image/png");
      else if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) httpd_resp_set_type(req, "image/jpeg");
      else if (strcmp(dot, ".json") == 0) httpd_resp_set_type(req, "application/json");
      else httpd_resp_set_type(req, "text/plain");
   }
   else httpd_resp_set_type(req, "text/plain"); 
   char buffer[BUFF_SIZE];
   size_t bytes_read = 0;
   while (file.available()) {
      bytes_read = file.read(buffer, BUFF_SIZE);
      if (bytes_read > 0) {
         esp_err_t err = httpd_resp_send_chunk(req, buffer, bytes_read);
         if (err != ESP_OK) {
            file.close();
            return err;
         }
      }
   }
   file.close();
   return httpd_resp_send_chunk(req, nullptr, 0);
}

template <uri_limit URI_SIZE, core_id CORE_NUMBER, size_t STACK_SIZE, socket_limit MAX_CONNECTIONS, size_t BUFF_SIZE>
[[nodiscard]] inline esp_err_t NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>::CaptivePortalHandler(httpd_req_t* __restrict req) {
   auto* server_instance = static_cast<NYXUS_HTTP_SERVER<URI_SIZE, CORE_NUMBER, STACK_SIZE, MAX_CONNECTIONS, BUFF_SIZE>*>(req->user_ctx);
   if (server_instance == nullptr) {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Server Instance Dead");
      return ESP_FAIL;
   }
   const char* uri = req->uri;
   if (strstr(uri, ".css") != nullptr || strstr(uri, ".js") != nullptr || strstr(uri, ".png") != nullptr) return server_instance->StreamFromSD(req, uri);
   return server_instance->StreamFromSD(req, "/index.html");
}

#endif