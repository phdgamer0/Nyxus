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
#include <SdFat.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <pinout.hpp>
#include <HardwareSerial.h>
#include <Nyxus/nyx_terminal_graphics.hpp>

/**
 * @namespace RenderMode
 * @brief Defines the visual rendering resolution and text-chunking density for the explorer UI.
 */
namespace RenderMode{
   inline constexpr uint8_t LIGHT   = 0x00; /**< Lightweight rendering mode. */
   inline constexpr uint8_t HEAVY   = 0x01; /**< Intensive rendering mode with full details. */
   inline constexpr uint8_t CHUNK8  = 0x08; /**< Render text in 8-byte chunks. */
   inline constexpr uint8_t CHUNK16 = 0x10; /**< Render text in 16-byte chunks. */
   inline constexpr uint8_t CHUNK32 = 0x20; /**< Render text in 32-byte chunks. */
   inline constexpr uint8_t CHUNK64 = 0x40; /**< Render text in 64-byte chunks. */
}

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
 * @struct StyleConfig
 * @brief Styling matrix defining the ANSI colors and visibility filters for the explorer UI.
 */
struct StyleConfig{
   std::optional<std::unordered_map<std::string, const char *>> ExtensionColors = std::nullopt; /**< Custom color mapping for specific file extensions. */
   const char* BackgroundColor = Color::NVIMDARK;   /**< Primary terminal background color.                     */
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
    * @brief 1D Vector to remember the CWD Files
    */
   std::vector<FileNode> Files;

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
   HWCDC* serial = nullptr;

   private:
   /**
    * @brief Pointer to the mounted SD card FAT32 volume.
    */
   SdFat* sd = nullptr;

   private:
   /**
    * @brief Hardware serial communication baud rate.
    */
   uint32_t Baud;

   private:
   /**
    * @brief Page Offset
    */
   uint32_t pager_offset = 0;

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
    * @brief Updated through helpers.
    */
   uint16_t cursPos = 1;

   private:
   /**
    * @brief Updated through helpers, caches the number of files in the current directory, making the drawing faster.
    */
   uint16_t dirFileCounts = 1;
   
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
   enum class EngineState : uint8_t { EXPLORER, PAGER };

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
   NYXUS_FILE_EXPLORER(HWCDC* Serialptr, SdFat* SDptr, const StyleConfig& Style = {}, uint8_t Fps = 30, const std::string& Path = "/") : serial(Serialptr), sd(SDptr), style(Style), FPS(Fps), current_path(Path), Files(){
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
    * @param Baudrate Operating speed for the serial communication line.
    * @param BufferSize Hardware RAM block allocated to prevent Tx/Rx bottlenecks.
    * @paragraph Terminal Boot Sequence
    * Opens the serial port and executes a blind ANSI CPR (Cursor Position Report) probe. 
    * The ESP32 waits for the terminal software to bounce back its exact Window Size so 
    * the UI can scale dynamically.
    * @return void
    */
   void Kickstart(uint32_t Baudrate = 115200, uint16_t BufferSize = 8192){
      BufferTextSize = BufferSize;
      serial->begin(115200);
      serial->setTxBufferSize(BufferSize);
      serial->setRxBufferSize(BufferSize);
      if(serial->isConnected()){
         serial->write(Cursor::HIDE);
         serial->printf("[%sEXPLORER%s] Connected to Serial %p with buffer size %d bytes on baud %d\n", Color::GREEN, Color::RESET, serial, BufferTextSize, serial->baudRate());
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
         serial->printf("[%sEXPLORER%s] Failed to connect to Serial %p with buffer size %d bytes on baud %d\n", Color::RED, Color::RESET, serial, BufferTextSize, serial->baudRate());
      }
      if (sd->begin(SDCARDP::CS)){
         serial->printf("[%sEXPLORER%s] Connected to SD card: ", Color::GREEN, Color::RESET);
         sd->printFatType(serial);
         serial->println();
      }
      else{
         serial->printf("[%sERROR%s] Failed to connect to SD card. Code: %d\n", Color::RED, Color::RESET, sd->sdErrorCode());
      }
   }

   public:
   /**
    * @brief Dynamically alters the hardware transmission buffer limits.
    * @param Size Target allocation block in bytes.
    * @paragraph Buffer Reallocation
    * Flushes and resizes the serial queues. Use carefully to avoid fragmenting FreeRTOS memory maps.
    * @return void
    */
   void SetTargetBufferSize(uint32_t Size){
      BufferTextSize = Size;
      serial->setTxBufferSize(Size);
      serial->setRxBufferSize(Size);
      serial->printf("[%sEXPLORER%s] Rx and Tx buffer size set to %d\n", Color::RED, Color::RESET, Size);
   }

   public:
   /**
    * @brief Overrides the target rendering frame rate.
    * @param FPS Target visual refresh speed.
    * @paragraph Frame Rate Limiter
    * Constrains the drawing engine to prevent flooding the USB-C serial line with redundant ANSI text redraws.
    * @return void
    */
   void SetTargetFPS(uint8_t FPS){
      if (FPS < 1){
         serial->printf("[%sERROR%s] FPS cant be set to %d\n", Color::RED, Color::RESET, FPS);
         return;
      }
      this->FPS = FPS;
      serial->printf("[%sEXPLORER%s] FPS set to %d\n", Color::GREEN, Color::RESET, FPS);
   }

   public:
   /**
    * @brief Binds the explorer to a new physical filesystem target.
    * @param SDptr Pointer to the SdFat interface.
    * @paragraph Volume Swapping
    * Allows seamless transitioning if the cyberdeck utilizes multiple SPI storage controllers.
    * @return void
    */
   void SetTargetSD(SdFat* SDptr){
      sd = SDptr;
      serial->printf("[%sEXPLORER%s] SD card connected: ", Color::GREEN, Color::RESET);
      sd->printFatType(serial);
      serial->println();
   }

   public:
   /**
    * @brief Re-routes the output ANSI stream to a different hardware interface.
    * @param Serialptr Pointer to the new hardware serial stream.
    * @paragraph Stream Handoff
    * Can be used to switch rendering from a local terminal to a remote Bluetooth or Wi-Fi TCP serial tunnel.
    * @return void
    */
   void SetTargetSerial(HWCDC* Serialptr){
      if(!serial) serial = Serialptr;
      else{
         serial->end();
         Kickstart(Baud, BufferTextSize);
      }
      serial->printf("[%sEXPLORER%s] Connected to Serial: %p\n", Color::GREEN, Color::RESET, serial);
   }

   public: 
   /**
    * @brief Jumps the explorer into a specific target directory.
    * @param Path The absolute directory current_path to evaluate.
    * @paragraph Path Validation
    * Primes the UI to fetch and render the contents of the specified path on the next drawing cycle.
    * @return void
    */
   void SetTargetPath(const std::string& Path){
      current_path = Path;
      serial->printf("[%sEXPLORER%s] Path set to: %s\n", Color::GREEN, Color::RESET, current_path);
   }

   public: 
   /**
    * @brief Evaluates hardware state to check if rendering should be terminated.
    * @paragraph Hardware Halt Check
    * Triggers a graceful failure if the physical USB connection drops, preventing an infinite output loop.
    * @return Boolean true if the sequence should abort.
    */
   bool ExplorerShouldEnd(){
      return !serial->isConnected();
   }

   private:
   /**
    * @brief Suspends execution and captures raw keyboard input from the terminal user.
    * @param prompt Optional message printed before freezing execution.
    * @param bufferSize The maximum byte length expected from the terminal.
    * @paragraph Memory Safety Architecture
    * Securely traps the thread using `readBytesUntil` and formats the captured payload into a safe C++ string. 
    * Completely eliminates the Use-After-Free heap crash associated with standard stack char arrays.
    * @return The sanitized string response extracted from the terminal interface.
    */
   std::string GetInputResponse(const char* prompt = nullptr, const size_t& bufferSize = 64){
      while(serial->available()) serial->read();
      
      if(prompt) serial->write(prompt, strlen(prompt));
      std::string result;
      while(true){
         if (serial->available()) {
            char c = serial->read();
            if (c == '\r' || c == '\n') {
               break;
            }
            if (c == Key::ESC) {
               return "";
            }
            if (c == '\b' || c == 0x7F) {
               if (!result.empty()) {
                  result.pop_back();
                  serial->write("\b \b");
               }
            } else if (result.length() < bufferSize) {
               result += c;
               serial->write(c);
            }
         }
         vTaskDelay(pdMS_TO_TICKS(10));
      }
      return result;
   }

   private:
   /**
    * @brief Non-blocking hardware parser for live terminal keystrokes.
    * @paragraph ANSI Extraction
    * Bypasses standard blocking streams. Checks the HWCDC buffer, instantly intercepts 
    * 3-byte ANSI escape sequences (Arrow Keys), normalizes line endings, and returns 
    * execution to the FreeRTOS scheduler in microseconds if the buffer is empty.
    * @return A uint8_t representing the parsed Key macro or ASCII character.
    */
   uint8_t PollKeyboard() {
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

   public:
   /**
    * @brief Gets the file extension and returns a pointer to where it is
    * @param start Where you want the search to start.
    * @param len The amount of charachters you want to serach from
    * @param includeDot Should the pointer returned be pointing at the '.'
    * @return `char*`
    */
   inline char* GetExtension(char* start, const size_t& len, const bool& includeDot = true){
      for(char* i = start + len - 1; i >= start; --i){
         if (*i == '.'){
            return includeDot ? i : i + 1;
         }
      }
      return start + len;
   }

   private:

   void ExecuteMake(bool is_dir) {
      serial->write(Cursor::TOXY(1, Height));
      serial->write(Clear::LNE);
      serial->write(style.BackgroundColor);
      serial->write(Font::REVERSE);
      std::string name = GetInputResponse(is_dir ? " NEW FOLDER NAME: " : " NEW FILE NAME: ");
      while(!name.empty() && (name.back() == '\n' || name.back() == '\r')) {
         name.pop_back();
      }
      if (!name.empty()) {
         std::string full_target = current_path;
         if (full_target.back() != '/') full_target += '/';
         full_target += name;
         if (is_dir) {
            sd->mkdir(full_target.c_str());
         } else {
            FsFile f = sd->open(full_target.c_str(), O_CREAT | O_WRITE);
            if (f) f.close();
         }
      }
      serial->write(Font::RESET);
      needs_full_redraw = true;
   }

   private:
   bool Rm_RF_Directory(const std::string& target_path) {
      FsFile f = sd->open(target_path.c_str(), O_READ);
      if (!f) return false;
      
      if (!f.isDir()) {
         f.close();
         return sd->remove(target_path.c_str());
      }
      
      f.rewind();
      FsFile entry;
      while (entry.openNext(&f, O_READ)) {
         char name[256];
         entry.getName(name, sizeof(name));
         entry.close();
         
         if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
         
         std::string sub_path = target_path;
         if (sub_path.back() != '/') sub_path += '/';
         sub_path += name;
         
         Rm_RF_Directory(sub_path);
      }
      f.close();
      return sd->rmdir(target_path.c_str());
   }

   private:

   void ExecuteDelete() {
      FileNode& target = Files[cursPos - 1];
      if (target.raw_name == "." || target.raw_name == "..") return;
      serial->write(Cursor::TOXY(1, Height));
      serial->write(Clear::LNE);
      serial->write(style.BackgroundColor);
      serial->write(Font::REVERSE);
      serial->printf(" DELETE '%s'? (y/n): ", target.raw_name.c_str());
      while(serial->available()) serial->read();
      char response = 0;
      while(true) {
         if (serial->available()) {
            response = serial->read();
            break;
         }
         vTaskDelay(pdMS_TO_TICKS(10));
      }
      serial->write(Font::RESET);
      if (response == 'y' || response == 'Y') {
         std::string full_target = current_path;
         if (full_target.back() != '/') full_target += '/';
         full_target += target.raw_name;
         
         Rm_RF_Directory(full_target);
      }
      needs_full_redraw = true;
   }

   private:

   void ExecuteRename() {
      FileNode& target = Files[cursPos - 1];
      if (target.raw_name == "." || target.raw_name == "..") return;
      serial->write(Cursor::TOXY(1, Height));
      serial->write(Clear::LNE);
      serial->write(style.BackgroundColor);
      serial->write(Font::REVERSE);
      serial->printf(" RENAME '%s' TO: ", target.raw_name.c_str());
      std::string new_name = GetInputResponse("");
      while(!new_name.empty() && (new_name.back() == '\n' || new_name.back() == '\r')) {
         new_name.pop_back();
      }
      if (!new_name.empty()) {
         std::string old_path = current_path;
         if (old_path.back() != '/') old_path += '/';
         old_path += target.raw_name;
         std::string new_path = current_path;
         if (new_path.back() != '/') new_path += '/';
         new_path += new_name;
         sd->rename(old_path.c_str(), new_path.c_str());
      }
      serial->write(Font::RESET);
      needs_full_redraw = true;
   }

   private:

   void ExecutePaste() {
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
         uint8_t buf[2048];
         int bytesRead;
         while ((bytesRead = src.read(buf, sizeof(buf))) > 0) {
            dest.write(buf, bytesRead);
         }
         dest.close();
      }
      src.close();
      needs_full_redraw = true;
   }

   private:

   void RunPager(uint8_t input) {
      if (input == 'q' || input == 'Q' || input == Key::ESC) {
         current_state = EngineState::EXPLORER;
         needs_full_redraw = true;
         return;
      }
      if (input == Key::DOWN || input == Key::ENTER) {
         pager_offset++;
      } 
      else if (input == Key::UP) {
         if (pager_offset > 0) pager_offset--;
      } 
      else if (input == Key::NONE && !needs_full_redraw) {
         return; 
      }
      FileNode& target = Files[cursPos - 1];
      std::string full_target = current_path;
      if (full_target.back() != '/') full_target += '/';
      full_target += target.raw_name;
      FsFile f = sd->open(full_target.c_str(), O_READ);
      if (!f) {
         current_state = EngineState::EXPLORER;
         needs_full_redraw = true;
         return;
      }
      serial->write(Clear::ALL);
      serial->write(Cursor::TOXY(1,1));
      serial->write(style.BackgroundColor);
      for(uint32_t i = 0; i < pager_offset; ++i) {
         char dummy[128];
         if(f.available()) f.fgets(dummy, sizeof(dummy));
         else break;
      }
      for(uint32_t i = 0; i < Height - 1; ++i) {
         char line[256];
         if(f.available()) {
            int n = f.fgets(line, sizeof(line));
            if(n > 0) serial->write(line);
         } else break;
      }
      f.close();
      serial->write(Cursor::TOXY(1, Height));
      serial->write(Font::REVERSE);
      serial->printf(" PAGER: %s | Q to exit | Up/Down to scroll ", target.raw_name.c_str());
      serial->write(Font::RESET);
      needs_full_redraw = false;
   }

   public:
   /**
    * @brief Executes the primary ANSI rendering matrix, drawing the active directory structure.
    * @paragraph Rendering Pipeline
    * Abstracts all state logic internally. Automatically detects if a full directory read is required,
    * or if a localized delta-render (cursor movement) is sufficient, minimizing SPI and CPU overhead.
    * @return void
    */
   void Draw(){
      uint8_t input = PollKeyboard();
      if (current_state == EngineState::PAGER) {
         RunPager(input);
         return;
      }
      if (needs_full_redraw){
         Files.clear();
         dirFileCounts = 1;
         cursPos = 1;
         FsFile dir = sd->open(current_path.c_str(), O_READ);
         if (!dir || !dir.isDir()) {
            serial->printf("[%sERROR%s] Failed to open directory: %s\n", Color::RED, Color::RESET, current_path.c_str());
            return;
         }
         Files.push_back({".", ".", style.DirectoryColor, true});
         Files.push_back({"..", "..", style.DirectoryColor, true});
         FsFile it;
         while(it.openNext(&dir, O_READ)){
            char name[256];
            it.getName(name, 256);
            const char* c = style.FileColor;
            bool is_directory = it.isDir();
            if(is_directory){
               c = style.DirectoryColor;
            }
            else if(it.isHidden() && style.ShowHiddenFiles){
               c = style.HiddenFileColor;
            }
            else {
               if(style.ExtensionColors.has_value()){
                  std::string ext(GetExtension(name, strlen(name)));
                  auto match = style.ExtensionColors.value().find(ext);
                  if (match != style.ExtensionColors.value().end()){
                     c = match->second;
                  }
               }
            }
            std::string display_string = name;
            if (style.ShowSizes && !is_directory) {
               char size_buf[32];
               snprintf(size_buf, sizeof(size_buf), " [%llu B]", it.fileSize());
               display_string += size_buf;
            }
            Files.push_back({name, display_string, c, is_directory});
            it.close();
         }
         dir.close();
         
         serial->write(Clear::ALL);
         
         serial->write(Cursor::TOXY(1, 1));
         serial->write(style.BackgroundColor);
         serial->write(Font::REVERSE);
         serial->printf(" [ PATH: %s ] ", current_path.c_str());
         serial->write(Font::RESET);
         for(size_t i = 0; i < Files.size() && (i + 2) <= Height; ++i) {
            serial->write(Cursor::TOXY(1, i + 2));
            serial->write(style.BackgroundColor);
            if (i + 1 == cursPos) serial->write(Font::REVERSE);
            serial->write(Files[i].color);
            serial->write(Files[i].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Font::RESET);
            dirFileCounts++;
         }
         needs_full_redraw = false;
      }
      else if (input == Key::UP){
         if (cursPos > 1) {
            serial->write(Cursor::TOXY(1, cursPos + 1));
            serial->write(style.BackgroundColor);
            serial->write(Files[cursPos - 1].color);
            serial->write(Files[cursPos - 1].display_name.c_str());
            serial->write(Color::RESET);
            cursPos--;
            serial->write(Cursor::TOXY(1, cursPos + 1));
            serial->write(style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->write(Files[cursPos - 1].color);
            serial->write(Files[cursPos - 1].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Font::RESET);
         }
      }
      else if (input == Key::DOWN){
         if (cursPos < Files.size() && cursPos < (Height - 1)) {
            serial->write(Cursor::TOXY(1, cursPos + 1));
            serial->write(style.BackgroundColor);
            serial->write(Files[cursPos - 1].color);
            serial->write(Files[cursPos - 1].display_name.c_str());
            serial->write(Color::RESET);
            cursPos++;
            serial->write(Cursor::TOXY(1, cursPos + 1));
            serial->write(style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->write(Files[cursPos - 1].color);
            serial->write(Files[cursPos - 1].display_name.c_str());
            serial->write(Color::RESET);
            serial->write(Font::RESET);
         }
      }
      else if (input == 'm' || input == 'M'){
         ExecuteMake(true);
      }
      else if (input == 't' || input == 'T'){
         ExecuteMake(false);
      }
      else if (input == 'd' || input == 'D' || input == Key::RIGHT){
         ExecuteDelete();
      }
      else if (input == 'r' || input == 'R'){
         ExecuteRename();
      }
      else if (input == 'y' || input == 'Y'){
         if (Files[cursPos - 1].raw_name != "." && Files[cursPos - 1].raw_name != "..") {
            clipboard_path = current_path;
            if (clipboard_path.back() != '/') clipboard_path += '/';
            clipboard_path += Files[cursPos - 1].raw_name;
            serial->write(Cursor::TOXY(1, Height));
            serial->write(Clear::LNE);
            serial->write(style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->printf(" YANKED: %s ", Files[cursPos - 1].raw_name.c_str());
            serial->write(Font::RESET);
         }
      }
      else if (input == 'p' || input == 'P'){
         ExecutePaste();
      }
      else if (input == 'c' || input == 'C'){
         if (!Files[cursPos - 1].is_dir) {
            current_state = EngineState::PAGER;
            pager_offset = 0;
            needs_full_redraw = true;
            RunPager(Key::NONE);
         }
      }
      else if (input == Key::ENTER){
         FileNode& target = Files[cursPos - 1];
         if (target.is_dir) {
            if (target.raw_name == ".") {
               return; 
            } 
            else if (target.raw_name == "..") {
               size_t last_slash = current_path.find_last_of('/');
               if (last_slash != std::string::npos) {
                  if (last_slash == 0) {
                     current_path = "/";
                  }
                  else {
                     current_path = current_path.substr(0, last_slash);
                  }
               }
               needs_full_redraw = true;
            } 
            else {
               if (current_path.back() != '/') {
                  current_path += '/';
               }
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
      }
   }

   public:
   /**
    * @brief Suspends the rendering thread to respect the hardware framerate limits.
    * @param ms Additional FreeRTOS delay padding in milliseconds.
    * @paragraph Framerate Pacing
    * Pushes any pending ANSI bytes sitting in the Tx queue, then puts the core to sleep, returning control 
    * to background FreeRTOS tasks (like network scanning) to prevent UI stuttering.
    * @return void
    */
   void Refresh(uint32_t ms = 0){
      vTaskDelay(pdMS_TO_TICKS(ms + 1000 / FPS));
      if (!needs_full_redraw) serial->flush();
   }
};
#endif