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

#ifndef _NEXUS_FILE_PARSER_HPP_
#define _NEXUS_FILE_PARSER_HPP_

#include <Arduino.h>
#include <vector>
#include <string>
#include <SdFat.h>
#include <Nyxus/nyx_network_types.hpp>

extern SdFat sd;

/**
 * @class NYXUS_FILE_PARSER
 * @brief Static hardware parser with zero heap-allocation for injecting disk-based configuration files directly into SRAM structs.
 */
class NYXUS_FILE_PARSER{
   public:
   NYXUS_FILE_PARSER() = delete;
   ~NYXUS_FILE_PARSER() = delete;

   public:
   /**
   * @brief Reads a physical SD file and maps its contents into the target ScanningConfig SRAM structure.
   * @param sd Pointer to the active SPI filesystem controller.
   * @param filepath Absolute path to the configuration file.
   * @param config Strictly-typed pointer to the target ScanningConfig struct.
   */
   static inline void Parse(SdFat* sd, const std::string& filepath, ScanningConfig* config);

   public:
   /**
   * @brief Reads a physical SD file and maps its contents into the target ConnectionConfig SRAM structure.
   * @param sd Pointer to the active SPI filesystem controller.
   * @param filepath Absolute path to the configuration file.
   * @param config Strictly-typed pointer to the target ConnectionConfig struct.
   */
   static inline void Parse(SdFat* sd, const std::string& filepath, ConnectionConfig* config);
};

/* IMPLEMENTATIONS - NOTHING HERE */

inline void NYXUS_FILE_PARSER::Parse(SdFat* sd, const std::string& filepath, ScanningConfig* config){
   if (config == nullptr || !sd->exists(filepath.c_str())) return;
   char currentFlag = '\0';
   char line[128];
   FsFile file = sd->open(filepath.c_str(), O_READ);
   if (!file) return;
   while(file.fgets(line, sizeof(line)) > 0){
      char* read_ptr = line; 
      char* write_ptr = line;
      while (*read_ptr) {
         if (*read_ptr != ' ' && *read_ptr != '\t' && *read_ptr != '\r' && *read_ptr != '\n') { 
            *write_ptr++ = *read_ptr; 
         }
         read_ptr++;
      }
      *write_ptr = '\0';
      if (line[0] == '\0' || (line[0] == '/' && line[1] == '/')) continue;
      if (line[0] == '-' && line[1] != '\0' && isAlpha(line[1])) {
         currentFlag = line[1];
         bool negate = (line[2] == '-');
         if (negate) {
            switch(currentFlag){
               case 'p': config->ports.clear(); break;
               case 'P': config->savePath = std::nullopt; break;
               case 'v': config->verbose = false; break;
               case 'T': config->timestampEnabled = false; break;
            }
            currentFlag = '\0';
         } else {
            if (currentFlag == 'v') { config->verbose = true; currentFlag = '\0'; }
            if (currentFlag == 'T') { config->timestampEnabled = true; currentFlag = '\0'; }
         }
         continue;
      }
      if (currentFlag != '\0') {
         switch(currentFlag){
            case 'p': config->ports.push_back((uint16_t)atoi(line)); break;
            case 't': config->timeout = strtoll(line, nullptr, 10); break;
            case 'P': config->savePath = std::string(line); break;
            case 'm': config->mode = (uint8_t)strtol(line, nullptr, 0); break;
            case 's': config->speed = (uint8_t)strtol(line, nullptr, 0); break;
            case 'r': config->retry = (uint8_t)atoi(line); break;
         }
         currentFlag = '\0';
      }
   }
   file.close();
}

inline void NYXUS_FILE_PARSER::Parse(SdFat* sd, const std::string& filepath, ConnectionConfig* config){
   if (config == nullptr || !sd->exists(filepath.c_str())) return;
   char currentFlag = '\0';
   char line[128];
   FsFile file = sd->open(filepath.c_str(), O_READ);
   if (!file) return;
   while(file.fgets(line, sizeof(line)) > 0){
      char* read_ptr = line; 
      char* write_ptr = line;
      while (*read_ptr) {
         if (*read_ptr != ' ' && *read_ptr != '\t' && *read_ptr != '\r' && *read_ptr != '\n') { 
            *write_ptr++ = *read_ptr; 
         }
         read_ptr++;
      }
      *write_ptr = '\0';
      if (line[0] == '\0' || (line[0] == '/' && line[1] == '/')) continue;
      if (strcmp(line, "-") == 0) {
         *config = ConnectionConfig{
            .connection_wait_time_ms = 200,
            .ip1 = ESP_IP4TOUINT32(192, 168, 1, 1),
            .gateway = ESP_IP4TOUINT32(192, 168, 1, 1),
            .subnet = ESP_IP4TOUINT32(255, 255, 255, 0),
            .dns1 = ESP_IP4TOUINT32(8, 8, 8, 8),
            .dns2 = ESP_IP4TOUINT32(8, 8, 4, 4),
            .retry_amount = 5
         };
         currentFlag = '\0';
         continue;
      }
      if (line[0] == '-' && line[1] != '\0' && isAlpha(line[1])) {
         currentFlag = line[1];
         bool negate = (line[2] == '-');
         if (negate) {
            switch(currentFlag){
               case 'i': config->ip1 = ESP_IP4TOUINT32(0,0,0,0); break;
               case 'g': config->gateway = ESP_IP4TOUINT32(0,0,0,0); break;
               case 's': config->subnet = ESP_IP4TOUINT32(0,0,0,0); break;
               case 'd': config->dns1 = ESP_IP4TOUINT32(0,0,0,0); break;
               case 'D': config->dns2 = ESP_IP4TOUINT32(0,0,0,0); break;
               case 'r': config->retry_amount = 5; break;
            }
            currentFlag = '\0';
         }
         continue;
      }
      if (currentFlag != '\0') {
         auto parseIpToUint32 = [&](uint32_t &dst){
            int a=0,b=0,c=0,d=0;
            if (sscanf(line, "%d.%d.%d.%d", &a, &b, &c, &d) == 4){
               dst = ESP_IP4TOUINT32((uint8_t)a, (uint8_t)b, (uint8_t)c, (uint8_t)d);
            } else {
               dst = ESP_IP4TOUINT32(0,0,0,0);
            }
         };
         switch(currentFlag){
            case 'w': config->connection_wait_time_ms = strtoll(line, nullptr, 10); break;
            case 'r': config->retry_amount = (uint8_t)atoi(line); break;
            case 'i': parseIpToUint32(config->ip1); break;
            case 'g': parseIpToUint32(config->gateway); break;
            case 's': parseIpToUint32(config->subnet); break;
            case 'd': parseIpToUint32(config->dns1); break;
            case 'D': parseIpToUint32(config->dns2); break;
         }
         currentFlag = '\0';
      }
   }
   file.close();
}

#endif