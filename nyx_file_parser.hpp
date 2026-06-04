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
#pragma once
#ifndef _NEXUS_FILE_PARSER_HPP_
#define _NEXUS_FILE_PARSER_HPP_

#include <Arduino.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <SdFat.h>
#include <nyx_network_types.hpp>

struct ScanningConfig;
extern SdFat sd;

/**
 * @class NYXUS_FILE_PARSER
 * @brief Static hardware parser with zero heap-allocation for injecting disk-based configuration files directly into SRAM structs.
 */
class NYXUS_FILE_PARSER{
   public:
   NYXUS_FILE_PARSER() = default;
   NYXUS_FILE_PARSER(const NYXUS_FILE_PARSER&) = default;
   NYXUS_FILE_PARSER& operator=(const NYXUS_FILE_PARSER&) = default;
   ~NYXUS_FILE_PARSER() = default;

   public:
   /**
   * @brief Reads a physical SD file and maps its contents into the target SRAM structure.
   * @param sd Pointer to the active SPI filesystem controller.
   * @param filepath Absolute path to the configuration file.
   * @param target Pointer to the memory address of the target struct/map.
   * @param limit Max payload bounds only utilized by dictionary parsing to prevent heap fragmentation.
   * @warning Bypasses type safety via void* casting. Ensure the target pointer exactly matches the file extension's intended struct.
   */
   static inline void Parse(SdFat* sd, const std::string& filepath, void* target = nullptr, uint64_t limit = UINT64_MAX){
      if (target == nullptr || !sd->exists(filepath.c_str())) return;
      size_t dotPos = filepath.find_last_of(".");
      if (dotPos == std::string::npos) return;
      std::string extension = filepath.substr(dotPos);
      if (strcmp(extension.c_str(), Extension::ssidpswd) == 0){
         auto map = static_cast<std::unordered_map<std::string, std::unordered_set<std::string>>*>(target);
         size_t lastSlash = filepath.find_last_of('/');
         size_t startPos = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
         std::string ssid = filepath.substr(startPos, dotPos - startPos);
         char line[128];
         FsFile file = sd->open(filepath.c_str(), O_READ);
         while(limit > 0 && file.fgets(line, sizeof(line)) > 0){
            int len = strlen(line);
            while (len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
               line[--len] = '\0';
            }
            if (line[0] == '\0' || (line[0] == '/' && line[1] == '/')) continue;
            if (map->operator[](ssid).insert(line).second){
               limit--;
            }
         }
         file.close();
      }
      else if (strcmp(extension.c_str(), Extension::scnconf) == 0){
         auto config = static_cast<ScanningConfig*>(target);
         char currentFlag = '\0';
         char line[128];
         FsFile file = sd->open(filepath.c_str(), O_READ);
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
                  if (currentFlag == 'v') config->verbose = true;
                  if (currentFlag == 'T') config->timestampEnabled = true;
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
            }
         }
         file.close();
      }
      else if (strcmp(extension.c_str(), Extension::connconf) == 0){
         auto config = static_cast<ConnectionConfig*>(target);
         char currentFlag = '\0';
         char line[128];
         FsFile file = sd->open(filepath.c_str(), O_READ);
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
                  .ip1 = IPAddress(192, 168, 1, 1),
                  .gateway = IPAddress(192, 168, 1, 1),
                  .subnet = IPAddress(255, 255, 255, 0),
                  .dns1 = IPAddress(8, 8, 8, 8),
                  .dns2 = IPAddress(8, 8, 4, 4),
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
                     case 'i': config->ip1 = IPAddress(0,0,0,0); break;
                     case 'g': config->gateway = IPAddress(0,0,0,0); break;
                     case 's': config->subnet = IPAddress(0,0,0,0); break;
                     case 'd': config->dns1 = IPAddress(0,0,0,0); break;
                     case 'D': config->dns2 = IPAddress(0,0,0,0); break;
                     case 'r': config->retry_amount = 5; break;
                  }
                  currentFlag = '\0';
               }
               continue;
            }
            
            if (currentFlag != '\0') {
               switch(currentFlag){
                  case 'w': config->connection_wait_time_ms = strtoll(line, nullptr, 10); break;
                  case 'r': config->retry_amount = (uint8_t)atoi(line); break;
                  case 'i': config->ip1.fromString(line); break;
                  case 'g': config->gateway.fromString(line); break;
                  case 's': config->subnet.fromString(line); break;
                  case 'd': config->dns1.fromString(line); break;
                  case 'D': config->dns2.fromString(line); break;
               }
            }
         }
         file.close();
      }
   }
};
#endif