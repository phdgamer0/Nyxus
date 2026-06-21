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
#pragma once
#ifndef _NEXUS_FILE_EXPLORER_HPP_
#define _NEXUS_FILE_EXPLORER_HPP_

#include <cstdint>
#include <functional>
#include <vector>
#include <Stream.h>
#include <SdFat.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <HardwareSerial.h>
#include <Nyxus/nyx_terminal_graphics.hpp>

/**
 * @namespace Key
 * @brief Raw hardware byte mappings and parsed terminal input codes.
 */
namespace Key {
   inline constexpr uint8_t NONE  = 0x00;   /**< No Input        */
   inline constexpr uint8_t UP    = 0x01;   /**< Arrow Up Key    */
   inline constexpr uint8_t DOWN  = 0x02;   /**< Arrow Down Key  */
   inline constexpr uint8_t RIGHT = 0x03;   /**< Arrow Right Key */
   inline constexpr uint8_t LEFT  = 0x04;   /**< Arrow Left Key  */
   inline constexpr uint8_t ENTER = 0x0D;   /**< Enter Key       */
   inline constexpr uint8_t ESC   = 0x1B;   /**< Escape Key      */
}

/**
 * @namespace UIEvent
 * @brief System event codes emitted by the explorer engine for hardware integration.
 */
namespace UIEvent {
   inline constexpr uint8_t IDLE      = 0x00;
   inline constexpr uint8_t NAVIGATED = 0x01; 
   inline constexpr uint8_t OPENED    = 0x02; 
   inline constexpr uint8_t SELECTED  = 0x03; 
   inline constexpr uint8_t DELETED   = 0x04;
   inline constexpr uint8_t YANKED    = 0x05;
   inline constexpr uint8_t PASTED    = 0x06;
   inline constexpr uint8_t MADE      = 0x07;
   inline constexpr uint8_t TOUCHED   = 0x08;
   inline constexpr uint8_t MOVEDUP   = 0x09;
   inline constexpr uint8_t MOVEDDOWN = 0x0A;
   inline constexpr uint8_t RENAMED   = 0x0B;
   inline constexpr uint8_t ERROR     = 0xFF;
}

/**
 * @struct StyleConfig
 * @brief Styling matrix defining the ANSI colors and visibility filters for the explorer UI.
 */
struct StyleConfig{
   std::optional<std::unordered_map<std::string, const char *>> ExtensionColors = std::nullopt; /**< Custom color mapping for specific file extensions. */
   const char* BackgroundColor = BgColor::NVIMDARK; /**< Primary terminal background color.                     */
   const char* DirectoryColor = Color::NVIMGREEN;   /**< Text color applied to directory folders.               */
   const char* HiddenFileColor = Color::DARKGRAY;   /**< Text color applied to hidden system files.             */
   const char* FileColor = Color::NVIMBLUE;         /**< Default text color for standard files.                 */
   const char* LinkColor = Color::NVIMPURPLE;       /**< Text color applied to symbolic links.                  */
   bool ShowHiddenFiles = true;                     /**< Toggles visibility of hidden system files.             */
   bool ShowSizes = true;                           /**< Toggles rendering of file byte sizes.                  */
   bool ShowLinks = true;                           /**< Toggles rendering of symbolic link paths.              */
   bool ShowDates = true;                           /**< Toggles rendering of last-modified timestamps.         */
   bool ShowPermissions = true;                     /**< Toggles rendering of read/write execution permissions. */
   const char* operator[](const std::string &key) const {
      if (ExtensionColors.has_value()){
         auto it = ExtensionColors.value().find(key);
         if (it != ExtensionColors.value().end()){
            return it->second;
         }
      }
      return nullptr;
   }
   const char*& operator[](const std::string &key){
      if (!ExtensionColors.has_value()){
         ExtensionColors = std::unordered_map<std::string, const char *>();
      }
      return ExtensionColors.value()[key];
   }
};

class NYXUS_FILE_EXPLORER{
   protected:
   /**
    * @brief Internal styling configuration object.
    */
   StyleConfig style;

   protected:
   /**
    * @struct File Node
    * @brief Saves Information about a file
    */
   struct FileNode {
      std::string raw_name;
      std::string display_name;
      const char* color;
      bool is_dir;
   };

   protected:
   /** 
   * @brief Active window cache strictly limited to terminal height. 0(1) Memory usage.
   */
   std::vector<FileNode> view_cache;

   private:
   /**
   * @brief Tracks the exact byte offset for the start of each line in the pager.
   */
   std::vector<uint32_t> pager_line_offsets;

   private:
   /** 
   * @brief Non-blocking buffer storing active user keystrokes for UI prompts.
   */
   std::string prompt_input_buffer = "";
   
   private:
   /**
   * @brief Contextual title displayed during an active prompt.
   */
   std::string prompt_title = "";

   private:
   /**
    * @brief Current Working Directory
    */
   std::string current_path = "/";

   private:
   /**
    * @brief Current Clipboard
    */
   std::string clipboard_path = "";

   private:
   /**
    * @brief Pointer to the hardware serial interface executing ANSI commands.
    */
   Stream* serial = nullptr;

   private:
   /**
    * @brief Pointer to the mounted SD card FAT32 volume.
    */
   SdFat* sd = nullptr;

   private:
   /**
    * @brief Pointer to the function you want to run on each UI event.
    */
   std::function<void(uint8_t)> HardwareCallback = nullptr;

   private:
   /**
    * @brief Page Offset
    */
   uint32_t pager_offset = 0;

   private:
   /**
   * @brief The globally absolute index of the currently highlighted file.
   */
   uint32_t selected_index = 0;

   private:
   /**
   * @brief The globally absolute index of the file at the very top of the terminal screen.
   */
   uint32_t top_index = 0;

   private:
   /**
   * @brief The total number of files in the current physical directory.
   */
   uint32_t total_files = 0;

   private:
   /**
    * @brief Buffer allocation size for serial chunking.
    */
   uint16_t BufferTextSize;

   private:
   /**
    * @brief Character width of the connected terminal screen.
    */
   uint16_t Width;

   private:
   /**
    * @brief Character height of the connected terminal screen.
    */
   uint16_t Height;
   
   private:
   /**
    * @brief Target frames per second for UI refresh sequences.
    */
   uint8_t FPS;

   private:
   /**
    * @enduml EngineState
    * @brief Defines the modes.
    */
   enum class EngineState : uint8_t { EXPLORER, PAGER, PROMPT_MKDIR, PROMPT_MKFILE, PROMPT_RENAME, PROMPT_DELETE };

   private:
   /**
    * @brief Current state that the Egine is in.
    */
   EngineState current_state = EngineState::EXPLORER;

   private:
   /**
    * @brief Internal state tracker to force a complete terminal repaint on directory changes.
    */
   bool needs_full_redraw = true;

   private:
   /**
   * @brief Triggers a partial re-fetch from the SD card when the cursor slides off-screen.
   */
   bool needs_window_redraw = false;

   public:
   /**
    * @brief Instantiates the terminal-based File Explorer module.
    * @param Serialptr Pointer to the hardware CDC Serial interface.
    * @param SDptr Pointer to the initialized SdFat filesystem object.
    * @param Style Configuration struct dictating visual rendering colors and toggles.
    * @param Fps Target frames per second for UI refresh rates.
    * @param Path Initial absolute directory path to open.
    * @paragraph Engine Instantiation
    * Binds the terminal interface to the active SD card volume and prepares the hardware buffers 
    * for rendering raw ANSI escape sequences at high refresh rates.
    */
   NYXUS_FILE_EXPLORER(Stream* Serialptr, SdFat* SDptr, void (*handler)(uint8_t) = nullptr, const StyleConfig& Style = {}, uint8_t Fps = 30, const std::string& Path = "/") : serial(Serialptr), sd(SDptr), HardwareCallback(handler), style(Style), FPS(Fps), current_path(Path){
      if (Fps < 1){
         FPS = 30;
      }
   };

   public:
   ~NYXUS_FILE_EXPLORER() = default;

   public:
   NYXUS_FILE_EXPLORER(const NYXUS_FILE_EXPLORER&) = default;

   public:
   NYXUS_FILE_EXPLORER& operator=(const NYXUS_FILE_EXPLORER&) = default;

   public:
   NYXUS_FILE_EXPLORER(NYXUS_FILE_EXPLORER&&) = default;

   public:
   NYXUS_FILE_EXPLORER& operator=(NYXUS_FILE_EXPLORER&&) = default;

   public:
   /**
    * @brief Initializes the serial hardware and requests active terminal boundaries.
    * @param SdPin CS Pin number of the sd.
    * @param Baudrate Operating speed for the serial communication line.
    * @param BufferSize Hardware RAM block allocated to prevent Tx/Rx bottlenecks.
    * @paragraph Terminal Boot Sequence
    * Opens the serial port and executes a blind ANSI CPR (Cursor Position Report) probe. 
    * The ESP32 waits for the terminal software to bounce back its exact Window Size so 
    * the UI can scale dynamically.
    * @return void
    */
   void Kickstart(const uint8_t SdPin, uint16_t BufferSize = 8192);

   public:

   void AttachEventHandler(std::function<void(uint8_t)> handler);

   public:

   void DetachEventHandler(void (*handler)(uint8_t));

   public:
   /**
    * @brief Updates the active styling configuration for the UI engine.
    * @param __nConfig The new StyleConfig struct containing color and visibility parameters.
    * @paragraph Style Replacement
    * Updates the formatting rules globally. The rendering engine will adopt 
    * the new visual configuration on the next frame redraw.
    * @return void
    */
   void SetStyleConfig(const StyleConfig& __nConfig);

   public:
   /**
    * @brief Retrieves a reference to the active styling configuration.
    * @paragraph Direct Access
    * Allows external processes to read or modify the UI configuration dynamically 
    * without requiring a full engine restart.
    * @return StyleConfig& Reference to the internal configuration struct.
    */
   StyleConfig& GetStyleConfig();

   public:
   /**
    * @brief Adds a custom ANSI text color mapping for a specific file extension.
    * @param EXT The target file extension string (e.g., ".pcap").
    * @param COL The ANSI color sequence to apply.
    * @paragraph Dynamic Mapping
    * Updates the internal hash map to alter UI rendering heuristics during runtime.
    * @return void
    */
   void AddNewExtensionColor(const char* EXT, const char* COL);

   public:
   /**
    * @brief Removes a specific file extension color mapping from the configuration.
    * @param EXT The file extension string to remove from the styling matrix.
    * @paragraph Safe Erasure
    * Verifies the existence of the mapping before attempting removal to prevent 
    * invalid memory access.
    * @return void
    */
   void RemoveExtensionColor(const char* EXT);

   public:
   /**
    * @brief Dynamically alters the hardware transmission buffer limits.
    * @param Size Target allocation block in bytes.
    * @paragraph Buffer Reallocation
    * Flushes and resizes the serial queues. Use carefully to avoid fragmenting FreeRTOS memory maps.
    * @return void
    */
   void SetTargetBufferSize(uint32_t Size);

   public:
   /**
    * @brief Overrides the target rendering frame rate.
    * @param FPS Target visual refresh speed.
    * @paragraph Frame Rate Limiter
    * Constrains the drawing engine to prevent flooding the USB-C serial line with redundant ANSI text redraws.
    * @return void
    */
   void SetTargetFPS(uint8_t FPS);

   public:
   /**
    * @brief Binds the explorer to a new physical filesystem target.
    * @param SDptr Pointer to the SdFat interface.
    * @paragraph Volume Swapping
    * Allows seamless transitioning if the cyberdeck utilizes multiple SPI storage controllers.
    * @return void
    */
   void SetTargetSD(SdFat* SDptr);

   public:
   /**
    * @brief Re-routes the output ANSI stream to a different hardware interface.
    * @param Serialptr Pointer to the new hardware serial stream.
    * @paragraph Stream Handoff
    * Can be used to switch rendering from a local terminal to a remote Bluetooth or Wi-Fi TCP serial tunnel.
    * @return void
    */
   void SetTargetSerial(HWCDC* Serialptr);
   
   public: 
   /**
    * @brief Jumps the explorer into a specific target directory.
    * @param Path The absolute directory current_path to evaluate.
    * @paragraph Path Validation
    * Primes the UI to fetch and render the contents of the specified path on the next drawing cycle.
    * @return void
    */
   void SetTargetPath(const std::string& Path);

   public: 
   /**
    * @brief Evaluates hardware state to check if rendering should be terminated.
    * @paragraph Hardware Halt Check
    * Triggers a graceful failure if the physical USB connection drops, preventing an infinite output loop.
    * @return Boolean true if the sequence should abort.
    */
   bool ExplorerShouldEnd();
   
   private:
   /**
    * @brief Non-blocking hardware parser for live terminal keystrokes.
    * @paragraph ANSI Extraction
    * Bypasses standard blocking streams. Checks the HWCDC buffer, instantly intercepts 
    * 3-byte ANSI escape sequences (Arrow Keys), normalizes line endings, and returns 
    * execution to the FreeRTOS scheduler in microseconds if the buffer is empty.
    * @return A uint8_t representing the parsed Key macro or ASCII character.
    */
   uint8_t PollKeyboard();

   public:
   /**
    * @brief Gets the file extension and returns a pointer to where it is
    * @param start Where you want the search to start.
    * @param len The amount of charachters you want to serach from
    * @param includeDot Should the pointer returned be pointing at the '.'
    * @return `char*`
    */
   inline char* GetExtension(char* start, const size_t& len, const bool& includeDot = true);

   private:
   /**
    * @brief Recursively deletes a directory and all of its nested contents.
    * @param target_path The absolute path of the directory to remove.
    * @paragraph Recursive Deletion
    * Iterates through the filesystem tree, systematically closing file handles and 
    * deleting nested files and subdirectories before removing the target root directory.
    * @return bool True if the target path was successfully removed.
    */
   bool Rm_RF_Directory(const std::string& target_path);

   private:
   /** @brief Asynchronous state handler for typing inputs. */
   void RunPrompt(uint8_t input, EngineState target_operation);

   private:
   /** @brief Asynchronous state handler for deletion confirmation. */
   void RunDeletePrompt(uint8_t input);

   private:
   /**
    * @brief Copies a file from the internal clipboard path to the current working directory.
    * @paragraph Memory Transfer
    * Allocates a 2048-byte buffer to copy binary data from the source file to the 
    * destination file in chunks, ensuring stack safety.
    * @return void
    */
   void ExecutePaste();

   private:
   /**
    * @brief Executes a memory-efficient text reader for the selected file.
    * @param input The parsed hardware keystroke driving the scroll offset.
    * @paragraph Stream Reader
    * Streams raw text directly from the SD card into the terminal window based on 
    * the user's current scroll offset, maintaining a minimal memory footprint.
    * @return void
    */
   void RunPager(uint8_t input);

   public:
   /**
    * @brief Executes the primary ANSI rendering matrix, drawing the active directory structure.
    * @param hardware_override Optional override in case of not using the keyboard.
    * @note Not passing any argument defaults to using Keyboard as the navigator.
    * @paragraph Rendering Pipeline
    * Abstracts all state logic internally. Automatically detects if a full directory read is required,
    * or if a localized delta-render (cursor movement) is sufficient, minimizing SPI and CPU overhead.
    * @return void
    */
   void Draw(uint8_t hardware_override = Key::NONE);

   public:
   /**
    * @brief Suspends the rendering thread to respect the hardware framerate limits.
    * @param ms Additional FreeRTOS delay padding in milliseconds.
    * @paragraph Framerate Pacing
    * Pushes any pending ANSI bytes sitting in the Tx queue, then puts the core to sleep, returning control 
    * to background FreeRTOS tasks (like network scanning) to prevent UI stuttering.
    * @return void
    */
   void Refresh(uint32_t ms = 0);
};

/* IMPLEMENTATIONS - NOTHING HERE */

void NYXUS_FILE_EXPLORER::Kickstart(const uint8_t SdPin, uint16_t BufferSize){
   BufferTextSize = BufferSize;
   if(serial){
      serial->write(Cursor::HIDE);
      serial->printf("[%sEXPLORER%s] Connected to Serial %p with buffer size %d bytes\n", Color::GREEN, Color::RESET, serial, BufferTextSize);
      char buf[16];
      serial->write(Cursor::GETSIZE, strlen(Cursor::GETSIZE));
      size_t ret = serial->readBytesUntil('R', buf, 16);
      buf[ret++] = 'R';
      buf[ret++] = '\0';
      std::pair<uint16_t, uint16_t> size = Cursor::PARSERSIZE(buf);
      Width = size.first;
      Height = size.second;
   }
   else{
      serial->printf("[%sEXPLORER%s] Failed to connect to Serial %p with buffer size %d bytes \n", Color::RED, Color::RESET, serial, BufferTextSize);
   }
   if (sd->begin(SdPin)){
      serial->printf("[%sEXPLORER%s] Connected to SD card: ", Color::GREEN, Color::RESET);
      sd->printFatType(serial);
      serial->println();
   }
   else{
      serial->printf("[%sERROR%s] Failed to connect to SD card. Code: %d\n", Color::RED, Color::RESET, sd->sdErrorCode());
   }
}

void NYXUS_FILE_EXPLORER::AttachEventHandler(std::function<void(uint8_t)> handler) {
   HardwareCallback = handler;
}

void NYXUS_FILE_EXPLORER::DetachEventHandler(void (*handler)(uint8_t)) {
   HardwareCallback = nullptr;
}

void NYXUS_FILE_EXPLORER::RunDeletePrompt(uint8_t input) {
   if (input == 'y' || input == 'Y') {
      FileNode& target = view_cache[selected_index - top_index];
      std::string full_target = current_path;
      if (full_target.back() != '/') full_target += '/';
      full_target += target.raw_name;
      Rm_RF_Directory(full_target);
      if (HardwareCallback) HardwareCallback(UIEvent::DELETED);
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
   }
   else if (input == 'n' || input == 'N' || input == Key::ESC) {
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
   }
   else if (input == Key::NONE && !needs_full_redraw) return;
   FileNode& target = view_cache[selected_index - top_index];
   serial->write(Cursor::TOXY(1, Height));
   serial->write(Clear::LNE);
   serial->write(style.BackgroundColor);
   serial->write(Font::REVERSE);
   serial->printf(" DELETE '%s'? (y/n): ", target.raw_name.c_str());
   serial->write(Font::RESET);
   needs_full_redraw = false;
}

void NYXUS_FILE_EXPLORER::RunPrompt(uint8_t input, EngineState target_operation) {
   if (input == Key::ESC) {
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
      return;
   }
   if (input == Key::ENTER) {
      while(!prompt_input_buffer.empty() && (prompt_input_buffer.back() == '\n' || prompt_input_buffer.back() == '\r')) prompt_input_buffer.pop_back();
      if (!prompt_input_buffer.empty()) {
         if (target_operation == EngineState::PROMPT_MKDIR || target_operation == EngineState::PROMPT_MKFILE) {
            std::string full_target = current_path;
            if (full_target.back() != '/') full_target += '/';
            full_target += prompt_input_buffer;
            if (target_operation == EngineState::PROMPT_MKDIR) {
               sd->mkdir(full_target.c_str());
               if (HardwareCallback) HardwareCallback(UIEvent::MADE);
            }
            else {
               FsFile f = sd->open(full_target.c_str(), O_CREAT | O_WRITE);
               if (f) f.close();
               if (HardwareCallback) HardwareCallback(UIEvent::TOUCHED);
            }
         } else if (target_operation == EngineState::PROMPT_RENAME) {
            FileNode& target = view_cache[selected_index - top_index];
            std::string old_path = current_path;
            if (old_path.back() != '/') old_path += '/';
            old_path += target.raw_name;
            std::string new_path = current_path;
            if (new_path.back() != '/') new_path += '/';
            new_path += prompt_input_buffer;
            sd->rename(old_path.c_str(), new_path.c_str());
            if (HardwareCallback) HardwareCallback(UIEvent::RENAMED);
         }
      }
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
      return;
   }
   if (input == '\b' || input == 0x7F) if (!prompt_input_buffer.empty()) prompt_input_buffer.pop_back();
   else if (input >= 32 && input <= 126 && prompt_input_buffer.length() < 64) prompt_input_buffer += (char)input;
   serial->write(Cursor::TOXY(1, Height));
   serial->write(Clear::LNE);
   serial->write(style.BackgroundColor);
   serial->write(Font::REVERSE);
   serial->printf("%s%s", prompt_title.c_str(), prompt_input_buffer.c_str());
   serial->write(Font::RESET);
}

void NYXUS_FILE_EXPLORER::SetStyleConfig(const StyleConfig& __nConfig){
   style = __nConfig;
}

StyleConfig& NYXUS_FILE_EXPLORER::GetStyleConfig(){
   return style;
}

void NYXUS_FILE_EXPLORER::AddNewExtensionColor(const char* EXT, const char* COL){
   if (!style.ExtensionColors.has_value()){
      style.ExtensionColors = std::unordered_map<std::string, const char *>();
   }
   style.ExtensionColors.value()[EXT] = COL;
}

void NYXUS_FILE_EXPLORER::RemoveExtensionColor(const char* EXT){
   if (!style.ExtensionColors.has_value()){
      return;
   }
   style.ExtensionColors.value().erase(EXT);
}

void NYXUS_FILE_EXPLORER::SetTargetFPS(uint8_t FPS){
   if (FPS < 1){
      serial->printf("[%sERROR%s] FPS cant be set to %d\n", Color::RED, Color::RESET, FPS);
      return;
   }
   this->FPS = FPS;
   serial->printf("[%sEXPLORER%s] FPS set to %d\n", Color::GREEN, Color::RESET, FPS);
}

void NYXUS_FILE_EXPLORER::SetTargetSD(SdFat* SDptr){
   sd = SDptr;
   serial->printf("[%sEXPLORER%s] SD card connected: ", Color::GREEN, Color::RESET);
   sd->printFatType(serial);
   serial->println();
}

void NYXUS_FILE_EXPLORER::SetTargetSerial(HWCDC* Serialptr){
   if(!serial) serial = Serialptr;
   else Kickstart(BufferTextSize);
   serial->printf("[%sEXPLORER%s] Connected to Serial: %p\n", Color::GREEN, Color::RESET, serial);
}

void NYXUS_FILE_EXPLORER::SetTargetPath(const std::string& Path){
   if (sd->exists(Path.c_str())){
      current_path = Path;
      serial->printf("[%sEXPLORER%s] Path set to: %s\n", Color::GREEN, Color::RESET, current_path);
   }
   else{
      serial->printf("[%sERROR%s] %s does not exist.\n", Color::RED, Color::RESET, current_path);
   }
}

bool NYXUS_FILE_EXPLORER::ExplorerShouldEnd(){
   return !serial->available();
}

uint8_t NYXUS_FILE_EXPLORER::PollKeyboard() {
   if (!serial->available()) return Key::NONE;
   uint8_t c = serial->read();
   if (c == '\r' || c == '\n') return Key::ENTER;
   if (c == Key::ESC) {
      uint32_t timeout = millis();
      while (serial->available() < 2) {
         if (millis() - timeout > 5) return Key::NONE;
      }
      if (serial->read() == '[') {
         uint8_t dir = serial->read();
         if (dir == 'A') return Key::UP;
         if (dir == 'B') return Key::DOWN;
         if (dir == 'C') return Key::RIGHT;
         if (dir == 'D') return Key::LEFT;
      }
      return Key::NONE;
   }
   if (c == 'w' || c == 'W') return Key::UP;
   if (c == 's' || c == 'S') return Key::DOWN;
   if (c == 'd' || c == 'D') return Key::RIGHT;
   if (c == 'a' || c == 'A') return Key::LEFT;
   return c;
}

inline char* NYXUS_FILE_EXPLORER::GetExtension(char* start, const size_t& len, const bool& includeDot){
   for(char* i = start + len - 1; i >= start; --i){
      if (*i == '.'){
         return includeDot ? i : i + 1;
      }
   }
   return start + len;
}

bool NYXUS_FILE_EXPLORER::Rm_RF_Directory(const std::string& target_path) {
   std::vector<std::string> dirs_to_delete;
   std::vector<std::string> stack;
   stack.push_back(target_path);
   while (!stack.empty()) {
      std::string current = stack.back();
      stack.pop_back();
      FsFile f = sd->open(current.c_str(), O_READ);
      if (!f) continue;
      if (!f.isDir()) {
         f.close();
         sd->remove(current.c_str());
         continue;
      }
      dirs_to_delete.push_back(current);
      f.rewind();
      FsFile entry;
      while (entry.openNext(&f, O_READ)) {
         char name[256];
         entry.getName(name, sizeof(name));
         entry.close();
         if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
         std::string sub_path = current;
         if (sub_path.back() != '/') sub_path += '/';
         sub_path += name;
         stack.push_back(sub_path);
      }
      f.close();
   }
   bool success = true;
   for (auto it = dirs_to_delete.rbegin(); it != dirs_to_delete.rend(); ++it) {
      if (!sd->rmdir(it->c_str())) success = false;
   }
   return success;
}

void NYXUS_FILE_EXPLORER::ExecutePaste() {
   if (clipboard_path.empty()) return;
   size_t last_slash = clipboard_path.find_last_of('/');
   if (last_slash == std::string::npos) return;
   std::string filename = clipboard_path.substr(last_slash + 1);
   std::string dest_path = current_path;
   if (dest_path.back() != '/') dest_path += '/';
   dest_path += filename;
   if (clipboard_path == dest_path) return;
   FsFile src = sd->open(clipboard_path.c_str(), O_READ);
   if (!src) return;
   if (src.isDir()) {
      src.close();
      serial->write(Cursor::TOXY(1, Height));
      serial->write(Clear::LNE);
      serial->write(style.BackgroundColor);
      serial->write(Font::REVERSE);
      serial->printf(" ERR: DIRECTORY COPY NOT SUPPORTED ");
      serial->write(Font::RESET);
      return;
   }
   FsFile dest = sd->open(dest_path.c_str(), O_WRITE | O_CREAT | O_TRUNC);
   if (dest) {
      uint8_t* buf = new (std::nothrow) uint8_t[2048];
      if (buf) {
         int bytesRead;
         while ((bytesRead = src.read(buf, 2048)) > 0) dest.write(buf, bytesRead);
         delete[] buf;
      } 
      else serial->printf(" [%sERROR%s] SRAM exhausted. Paste failed. ", Color::RED, Color::RESET);
      dest.close();
   }
   src.close();
   needs_full_redraw = true;
}

void NYXUS_FILE_EXPLORER::RunPager(uint8_t input) {
   if (input == 'q' || input == 'Q' || input == Key::ESC) {
      current_state = EngineState::EXPLORER;
      pager_line_offsets.clear();
      needs_full_redraw = true;
      return;
   }
   if (input == Key::DOWN || input == Key::ENTER) pager_offset++;
   else if (input == Key::UP) if (pager_offset > 0) pager_offset--;
   else if (input == Key::NONE && !needs_full_redraw) return;
   FileNode& target = view_cache[selected_index - top_index];
   std::string full_target = current_path;
   if (full_target.back() != '/') full_target += '/';
   full_target += target.raw_name;
   FsFile f = sd->open(full_target.c_str(), O_READ);
   if (!f) {
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
      return;
   }
   f.seek(pager_line_offsets.back());
   while (pager_line_offsets.size() <= pager_offset + Height && f.available()) {
      char dummy[256];
      f.fgets(dummy, sizeof(dummy));
      pager_line_offsets.push_back(f.position());
   }
   serial->write(style.BackgroundColor);
   serial->write(Clear::ALL);
   serial->write(Cursor::TOXY(1,1));
   if (pager_offset < pager_line_offsets.size()) f.seek(pager_line_offsets[pager_offset]);
   for(uint32_t i = 0; i < Height - 1; ++i) {
      char line[256];
      if(f.available()) {
         int n = f.fgets(line, sizeof(line));
         if(n > 0) serial->write(line);
      }
      else break;
   }
   f.close();
   serial->write(Cursor::TOXY(1, Height));
   serial->write(Font::REVERSE);
   serial->printf(" PAGER: %s | Q to exit | Up/Down to scroll ", target.raw_name.c_str());
   serial->write(Font::RESET);
   needs_full_redraw = false;
}

void NYXUS_FILE_EXPLORER::Draw(uint8_t hardware_override){
   uint8_t input = (hardware_override != Key::NONE) ? hardware_override : PollKeyboard();
   if (current_state == EngineState::PAGER) {
      RunPager(input);
      return;
   }
   if (current_state == EngineState::PROMPT_DELETE) { RunDeletePrompt(input); return; }
   if (current_state == EngineState::PROMPT_MKDIR || current_state == EngineState::PROMPT_MKFILE || current_state == EngineState::PROMPT_RENAME) {
      RunPrompt(input, current_state); 
      return; 
   }
   if (needs_full_redraw) {
      total_files = 2;
      FsFile dir = sd->open(current_path.c_str(), O_READ);
      if (dir) {
         FsFile it;
         while(it.openNext(&dir, O_READ)) { total_files++; it.close(); }
         dir.close();
      }
      selected_index = 0;
      top_index = 0;
      needs_full_redraw = false;
      needs_window_redraw = true;
   }
   if (input == Key::UP) {
      if (selected_index > 0) {
         uint32_t old_index = selected_index;
         selected_index--;
         if (selected_index < top_index) {
            top_index = selected_index;
            needs_window_redraw = true;
         }
         else if (!needs_window_redraw) {
            serial->write(Cursor::TOXY(1, (old_index - top_index) + 2));
            serial->write(style.BackgroundColor);
            serial->write(view_cache[old_index - top_index].color);
            serial->write(view_cache[old_index - top_index].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Cursor::TOXY(1, (selected_index - top_index) + 2));
            serial->write(style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->write(view_cache[selected_index - top_index].color);
            serial->write(view_cache[selected_index - top_index].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Font::RESET);
            if (HardwareCallback) HardwareCallback(UIEvent::MOVEDUP);
         }
      }
   }
   else if (input == Key::DOWN) {
      if (selected_index < total_files - 1) {
         uint32_t old_index = selected_index;
         selected_index++;
         
         if (selected_index >= top_index + (Height - 2)) {
            top_index = selected_index - (Height - 2) + 1;
            needs_window_redraw = true;
         } else if (!needs_window_redraw) {
            serial->write(Cursor::TOXY(1, (old_index - top_index) + 2));
            serial->write(style.BackgroundColor);
            serial->write(view_cache[old_index - top_index].color);
            serial->write(view_cache[old_index - top_index].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Cursor::TOXY(1, (selected_index - top_index) + 2));
            serial->write(style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->write(view_cache[selected_index - top_index].color);
            serial->write(view_cache[selected_index - top_index].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Font::RESET);
            if (HardwareCallback) HardwareCallback(UIEvent::MOVEDDOWN);
         }
      }
   }
   if (needs_window_redraw) {
      view_cache.clear();
      FsFile dir = sd->open(current_path.c_str(), O_READ);
      FsFile it;
      uint32_t current_idx = 0;
      uint32_t limit = (Height - 2);
      for (uint32_t i = 0; i < limit && (top_index + i) < total_files; i++) {
         uint32_t target_idx = top_index + i;
         std::string name;
         bool is_dir = false;
         uint64_t size = 0;
         if (target_idx == 0) { name = "."; is_dir = true; }
         else if (target_idx == 1) { name = ".."; is_dir = true; }
         else {
            while (current_idx <= (target_idx - 2)) {
               it.close();
               it.openNext(&dir, O_READ);
               current_idx++;
            }
            char buf[256];
            it.getName(buf, sizeof(buf));
            name = buf;
            is_dir = it.isDir();
            size = it.fileSize();
         }
         const char* c = style.FileColor;
         if (is_dir) c = style.DirectoryColor;
         else if (name.length() > 0 && name[0] == '.') c = style.HiddenFileColor;
         else if (style.ExtensionColors.has_value()) {
            char* dot = GetExtension((char*)name.c_str(), name.length(), true);
            if (dot) {
               std::string ext(dot);
               auto match = style.ExtensionColors.value().find(ext);
               if (match != style.ExtensionColors.value().end()) c = match->second;
            }
         }
         std::string display = name;
         if (!is_dir && style.ShowSizes) {
            char sz_buf[32];
            snprintf(sz_buf, sizeof(sz_buf), " [%llu B]", size);
            display += sz_buf;
         }
         view_cache.push_back({name, display, c, is_dir});
      }
      it.close();
      dir.close();
      serial->write(style.BackgroundColor);
      serial->write(Clear::ALL);
      serial->write(Cursor::TOXY(1, 1));
      serial->write(style.BackgroundColor);
      serial->write(Font::REVERSE);
      serial->printf(" [ PATH: %s ] ", current_path.c_str());
      serial->write(Font::RESET);
      for (size_t i = 0; i < view_cache.size(); i++) {
         serial->write(Cursor::TOXY(1, i + 2));
         serial->write(style.BackgroundColor);
         if (top_index + i == selected_index) serial->write(Font::REVERSE);
         serial->write(view_cache[i].color);
         serial->write(view_cache[i].display_name.c_str());
         serial->write(Color::RESET);
         serial->write(Font::RESET);
      }
      needs_window_redraw = false;
   }
   if (input == 'm' || input == 'M'){
      current_state = EngineState::PROMPT_MKDIR;
      prompt_title = " NEW FOLDER NAME: ";
      prompt_input_buffer = "";
      RunPrompt(Key::NONE, current_state);
   }
   else if (input == 't' || input == 'T'){
      current_state = EngineState::PROMPT_MKFILE;
      prompt_title = " NEW FILE NAME: ";
      prompt_input_buffer = "";
      RunPrompt(Key::NONE, current_state);
   }
   else if (input == 'd' || input == 'D' || input == Key::RIGHT){
      FileNode& target = view_cache[selected_index - top_index];
      if (target.raw_name != "." && target.raw_name != "..") {
         current_state = EngineState::PROMPT_DELETE;
         RunDeletePrompt(Key::NONE);
      }
   }
   else if (input == 'r' || input == 'R'){
      FileNode& target = view_cache[selected_index - top_index];
      if (target.raw_name != "." && target.raw_name != "..") {
         current_state = EngineState::PROMPT_RENAME;
         char titleBuf[64];
         snprintf(titleBuf, sizeof(titleBuf), " RENAME '%s' TO: ", target.raw_name.c_str());
         prompt_title = titleBuf;
         prompt_input_buffer = "";
         RunPrompt(Key::NONE, current_state);
      }
   }
   else if (input == 'y' || input == 'Y'){
      FileNode& target = view_cache[selected_index - top_index];
      if (target.raw_name != "." && target.raw_name != "..") {
         clipboard_path = current_path;
         if (clipboard_path.back() != '/') clipboard_path += '/';
         clipboard_path += target.raw_name;
         serial->write(Cursor::TOXY(1, Height));
         serial->write(Clear::LNE);
         serial->write(style.BackgroundColor);
         serial->write(Font::REVERSE);
         serial->printf(" YANKED: %s ", target.raw_name.c_str());
         serial->write(Font::RESET);
         if (HardwareCallback) HardwareCallback(UIEvent::YANKED);
      }
   }
   else if (input == 'p' || input == 'P'){
      ExecutePaste();
      if (HardwareCallback) HardwareCallback(UIEvent::PASTED);
   }
   else if (input == 'c' || input == 'C'){
      FileNode& target = view_cache[selected_index - top_index];
      if (!target.is_dir) {
         current_state = EngineState::PAGER;
         pager_offset = 0;
         pager_line_offsets.clear();
         pager_line_offsets.push_back(0);
         needs_full_redraw = true;
         RunPager(Key::NONE);
         if (HardwareCallback) HardwareCallback(UIEvent::OPENED);
      }
   }
   else if (input == Key::ENTER){
      FileNode& target = view_cache[selected_index - top_index];
      if (target.is_dir) {
         if (target.raw_name == ".") {
            return; 
         } 
         else if (target.raw_name == "..") {
            size_t last_slash = current_path.find_last_of('/');
            if (last_slash != std::string::npos) {
               if (last_slash == 0) { current_path = "/"; }
               else { current_path = current_path.substr(0, last_slash); }
            }
            needs_full_redraw = true;
         } 
         else {
            if (current_path.back() != '/') current_path += '/';
            current_path += target.raw_name;
            needs_full_redraw = true;
         }
      } 
      else {
         serial->write(Clear::ALL);
         serial->write(Cursor::TOXY(1, 1));
         serial->printf("[%sSYSTEM%s] Selected Payload: %s\n", Color::YELLOW, Color::RESET, target.raw_name.c_str());
         needs_full_redraw = true; 
      }
      if (HardwareCallback) HardwareCallback(UIEvent::NAVIGATED);
   }
}

void NYXUS_FILE_EXPLORER::Refresh(uint32_t ms){
   vTaskDelay(pdMS_TO_TICKS(ms + 1000 / FPS));
   if (!needs_full_redraw) serial->flush();
}

#endif