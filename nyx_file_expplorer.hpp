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
#ifndef _NYXUS_FILE_EXPLORER_HPP_
#define _NYXUS_FILE_EXPLORER_HPP_

#include <cstdint>
#include <functional>
#include <vector>
#include <Stream.h>
#include <SdFat.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <cstring>
#include <new>
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
   /** @brief The SRAM text buffer for the active file. */
   std::vector<std::string> editor_buffer;

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
   /** @brief Buffer for typing :wq or :q commands */
   std::string editor_cmd_buffer = "";

   private:
   /** @brief Editor yank register for `y` and `p` operations. */
   std::vector<std::string> editor_yank_register;

   private:
   /** @brief Indicates whether the yank register contains linewise text. */
   bool yank_linewise = false;

   private:
   /** @brief Pending editor operator state for d/y/c sequences. */
   uint8_t pending_operator = 0;

   private:
   /** @brief Pending text object state for sequences like ciw/diw/yiw. */
   uint8_t pending_text_object = 0;

   private:
   struct EditorHistory {
      std::vector<std::string> buffer;
      uint32_t cursor_x = 0;
      uint32_t cursor_y = 0;
   };

   private:
   std::vector<EditorHistory> undo_stack;

   private:
   std::vector<EditorHistory> redo_stack;

   private:
   std::string last_edit_command = "";

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
   /** @brief Editor cursor tracking (X = Column, Y = Line in buffer) */
   uint32_t cursor_x = 0;

   private:
   /** @brief Editor cursor line inside the active SRAM text buffer. */
   uint32_t cursor_y = 0;

   private:
   /** @brief Vertical editor offset used to keep the active line visible. */
   uint32_t editor_scroll_y = 0;

   private:
   /** @brief Horizontal editor offset used to keep long lines inside the terminal width. */
   uint32_t editor_scroll_x = 0;

   private:
   /** @brief Visual selection anchor column captured when v/V starts. */
   uint32_t visual_anchor_x = 0;

   private:
   /** @brief Visual selection anchor line captured when v/V starts. */
   uint32_t visual_anchor_y = 0;

   private:
   /** @brief Limit editor to 64KB to prevent FreeRTOS Heap Panics */
   static constexpr uint32_t MAX_EDITOR_FILE_SIZE = 65536;

   private:
   /** @brief Oversized ANSI row that terminal emulators clamp to the physical bottom row. */
   static constexpr uint16_t TERMINAL_BOTTOM_ROW = 65535;

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
   enum class EngineState : uint8_t { EXPLORER, PAGER, EDITOR, PROMPT_MKDIR, PROMPT_MKFILE, PROMPT_RENAME, PROMPT_DELETE };

   private:
   /** @brief Vim Engine Sub-States */
   enum class EditorMode : uint8_t { NORMAL, INSERT, COMMAND, VISUAL, VISUAL_BLOCK };

   EditorMode editor_mode = EditorMode::NORMAL;

   private:
   /**
    * @brief Current state that the Egine is in.
    */
   EngineState current_state = EngineState::EXPLORER;

   private:
   /** @brief Pending multi-key editor command prefix, currently used for gg. */
   uint8_t pending_cmd = 0;

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
   inline void Kickstart(const uint8_t SdPin, uint16_t BufferSize = 8192);

   public:

   inline void AttachEventHandler(std::function<void(uint8_t)> handler);

   public:

   inline void DetachEventHandler(void (*handler)(uint8_t));

   public:
   /**
    * @brief Updates the active styling configuration for the UI engine.
    * @param __nConfig The new StyleConfig struct containing color and visibility parameters.
    * @paragraph Style Replacement
    * Updates the formatting rules globally. The rendering engine will adopt 
    * the new visual configuration on the next frame redraw.
    * @return void
    */
   inline void SetStyleConfig(const StyleConfig& __nConfig);

   public:
   /**
    * @brief Retrieves a reference to the active styling configuration.
    * @paragraph Direct Access
    * Allows external processes to read or modify the UI configuration dynamically 
    * without requiring a full engine restart.
    * @return StyleConfig& Reference to the internal configuration struct.
    */
   [[nodiscard]] inline StyleConfig& GetStyleConfig();

   public:
   /**
    * @brief Adds a custom ANSI text color mapping for a specific file extension.
    * @param EXT The target file extension string (e.g., ".pcap").
    * @param COL The ANSI color sequence to apply.
    * @paragraph Dynamic Mapping
    * Updates the internal hash map to alter UI rendering heuristics during runtime.
    * @return void
    */
   inline void AddNewExtensionColor(const char* EXT, const char* COL);

   public:
   /**
    * @brief Removes a specific file extension color mapping from the configuration.
    * @param EXT The file extension string to remove from the styling matrix.
    * @paragraph Safe Erasure
    * Verifies the existence of the mapping before attempting removal to prevent 
    * invalid memory access.
    * @return void
    */
   inline void RemoveExtensionColor(const char* EXT);

   public:
   /**
    * @brief Dynamically alters the hardware transmission buffer limits.
    * @param Size Target allocation block in bytes.
    * @paragraph Buffer Reallocation
    * Flushes and resizes the serial queues. Use carefully to avoid fragmenting FreeRTOS memory maps.
    * @return void
    */
   inline void SetTargetBufferSize(uint32_t Size);

   public:
   /**
    * @brief Overrides the target rendering frame rate.
    * @param FPS Target visual refresh speed.
    * @paragraph Frame Rate Limiter
    * Constrains the drawing engine to prevent flooding the USB-C serial line with redundant ANSI text redraws.
    * @return void
    */
   inline void SetTargetFPS(uint8_t FPS);

   public:
   /**
    * @brief Binds the explorer to a new physical filesystem target.
    * @param SDptr Pointer to the SdFat interface.
    * @paragraph Volume Swapping
    * Allows seamless transitioning if the cyberdeck utilizes multiple SPI storage controllers.
    * @return void
    */
   inline void SetTargetSD(SdFat* SDptr);

   public:
   /**
    * @brief Re-routes the output ANSI stream to a different hardware interface.
    * @param Serialptr Pointer to the new hardware serial stream.
    * @paragraph Stream Handoff
    * Can be used to switch rendering from a local terminal to a remote Bluetooth or Wi-Fi TCP serial tunnel.
    * @return void
    */
   inline void SetTargetSerial(Stream* Serialptr);
   
   public: 
   /**
    * @brief Jumps the explorer into a specific target directory.
    * @param Path The absolute directory current_path to evaluate.
    * @paragraph Path Validation
    * Primes the UI to fetch and render the contents of the specified path on the next drawing cycle.
    * @return void
    */
   inline void SetTargetPath(const std::string& Path);

   public: 
   /**
    * @brief Evaluates hardware state to check if rendering should be terminated.
    * @paragraph Hardware Halt Check
    * Triggers a graceful failure if the physical USB connection drops, preventing an infinite output loop.
    * @return Boolean true if the sequence should abort.
    */
   [[nodiscard]] inline bool ExplorerShouldEnd();
   
   private:
   /**
    * @brief Non-blocking hardware parser for live terminal keystrokes.
    * @paragraph ANSI Extraction
    * Bypasses standard blocking streams. Checks the Stream buffer, instantly intercepts
    * 3-byte ANSI escape sequences (Arrow Keys), normalizes line endings, and returns 
    * execution to the FreeRTOS scheduler in microseconds if the buffer is empty.
    * @return A uint8_t representing the parsed Key macro or ASCII character.
    */
   [[nodiscard]] inline uint8_t PollKeyboard();

   private:
   /**
    * @brief Returns a wrap-safe terminal width.
    * @note The final terminal column is intentionally unused because writing into it can
    *       trigger automatic wrapping and scroll the entire screen.
    */
   [[nodiscard]] inline uint16_t RenderWidth() const;

   private:
   /**
    * @brief Writes only the visible, wrap-safe portion of a string.
    * @param text Source text.
    * @param offset First source character to render.
    */
   inline void WriteClipped(const std::string& text, uint32_t offset = 0, uint16_t width = 0);

   private:
   /**
    * @brief Paints a complete wrap-safe row with the requested background color.
    * @param row One-based terminal row.
    * @param background ANSI background color sequence.
    * @param reverse Toggles Font::REVERSE to create a solid inverted status bar.
    */
   inline void FillLine(uint16_t row, const char* background, bool reverse = false);

   private:
   /**
    * @brief Truncates a status segment and appends an ellipsis when it exceeds its slot.
    */
   [[nodiscard]] inline std::string Ellipsize(const std::string& text, uint16_t width) const;

   public:
   /**
    * @brief Gets the file extension and returns a pointer to where it is
    * @param start Where you want the search to start.
    * @param len The amount of charachters you want to serach from
    * @param includeDot Should the pointer returned be pointing at the '.'
    * @return `char*`
    */
   [[nodiscard]] inline char* GetExtension(char* start, const size_t& len, const bool& includeDot = true);

   private:
   /**
    * @brief Recursively deletes a directory and all of its nested contents.
    * @param target_path The absolute path of the directory to remove.
    * @paragraph Recursive Deletion
    * Iterates through the filesystem tree, systematically closing file handles and 
    * deleting nested files and subdirectories before removing the target root directory.
    * @return bool True if the target path was successfully removed.
    */
   [[nodiscard]] inline bool Rm_RF_Directory(const std::string& target_path);

   private:

   inline void RunEditor(uint8_t input);

   private:
   /** @brief Asynchronous state handler for typing inputs. */
   inline void RunPrompt(uint8_t input, EngineState target_operation);

   private:
   /** @brief Asynchronous state handler for deletion confirmation. */
   inline void RunDeletePrompt(uint8_t input);

   private:
   /**
    * @brief Copies a file from the internal clipboard path to the current working directory.
    * @paragraph Memory Transfer
    * Allocates a 2048-byte buffer to copy binary data from the source file to the 
    * destination file in chunks, ensuring stack safety.
    * @return void
    */
   inline void ExecutePaste();

   private:
   /**
    * @brief Executes a memory-efficient text reader for the selected file.
    * @param input The parsed hardware keystroke driving the scroll offset.
    * @paragraph Stream Reader
    * Streams raw text directly from the SD card into the terminal window based on 
    * the user's current scroll offset, maintaining a minimal memory footprint.
    * @return void
    */
   inline void RunPager(uint8_t input);

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
   inline void Draw(uint8_t hardware_override = Key::NONE);

   public:
   /**
    * @brief Suspends the rendering thread to respect the hardware framerate limits.
    * @param ms Additional FreeRTOS delay padding in milliseconds.
    * @paragraph Framerate Pacing
    * Pushes any pending ANSI bytes sitting in the Tx queue, then puts the core to sleep, returning control 
    * to background FreeRTOS tasks (like network scanning) to prevent UI stuttering.
    * @return void
    */
   inline void Refresh(uint32_t ms = 0);
};

/* IMPLEMENTATIONS - NOTHING HERE */

inline void NYXUS_FILE_EXPLORER::Kickstart(const uint8_t SdPin, uint16_t BufferSize){
   BufferTextSize = BufferSize;
   if(serial){
      serial->write(Cursor::HIDE);
      serial->printf("[%sEXPLORER%s] Connected to Serial %p with buffer size %d bytes\n", Color::GREEN, Color::RESET, serial, BufferTextSize);
      bool probe_success = false;
      for (int attempts = 0; attempts < 3; attempts++) {
         while(serial->available()) serial->read();
         serial->write(Cursor::GETSIZE);
         serial->setTimeout(500);
         char buf[32]{};
         size_t ret = serial->readBytesUntil('R', buf, 31);
         if (ret > 0) {
            buf[ret++] = 'R';
            buf[ret] = '\0';
            std::pair<uint16_t, uint16_t> size = Cursor::PARSERSIZE(buf);
            if (size.first > 0 && size.second > 2) {
               Width = size.first;
               Height = size.second;
               probe_success = true;
               break;
            }
         }
         vTaskDelay(pdMS_TO_TICKS(100));
      }
      if (!probe_success) {
         Width = 80;
         Height = 24;
         serial->printf("[%sWARNING%s] Terminal CPR timeout. Defaulting to 80x24.\n", Color::YELLOW, Color::RESET);
      }
   }
   else{
      return;
   }
   if (!sd) {
      serial->printf("[%sERROR%s] SD card pointer is null.\n", Color::RED, Color::RESET);
      return;
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

inline void NYXUS_FILE_EXPLORER::AttachEventHandler(std::function<void(uint8_t)> handler) {
   HardwareCallback = handler;
}

inline void NYXUS_FILE_EXPLORER::DetachEventHandler(void (*handler)(uint8_t)) {
   HardwareCallback = nullptr;
}

inline void NYXUS_FILE_EXPLORER::RunDeletePrompt(uint8_t input) {
   if (input == 'y' || input == 'Y') {
      FileNode& target = view_cache[selected_index - top_index];
      std::string full_target = current_path;
      if (full_target.back() != '/') full_target += '/';
      full_target += target.raw_name;
      const bool removed = Rm_RF_Directory(full_target);
      if (HardwareCallback) HardwareCallback(removed ? UIEvent::DELETED : UIEvent::ERROR);
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
   }
   else if (input == 'n' || input == 'N' || input == Key::ESC) {
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
   }
   else if (input == Key::NONE && !needs_full_redraw) return;
   FileNode& target = view_cache[selected_index - top_index];
   FillLine(Height, style.BackgroundColor, true);
   serial->write(Font::REVERSE);
   WriteClipped(" DELETE '" + target.raw_name + "'? (y/n): ");
   serial->write(Font::RESET);
   serial->flush();
}

inline void NYXUS_FILE_EXPLORER::RunPrompt(uint8_t input, EngineState target_operation) {
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
               const bool made = sd->mkdir(full_target.c_str());
               if (HardwareCallback) HardwareCallback(made ? UIEvent::MADE : UIEvent::ERROR);
            }
            else {
               FsFile f = sd->open(full_target.c_str(), O_CREAT | O_WRITE);
               const bool touched = static_cast<bool>(f);
               if (f) f.close();
               if (HardwareCallback) HardwareCallback(touched ? UIEvent::TOUCHED : UIEvent::ERROR);
            }
         } else if (target_operation == EngineState::PROMPT_RENAME) {
            FileNode& target = view_cache[selected_index - top_index];
            std::string old_path = current_path;
            if (old_path.back() != '/') old_path += '/';
            old_path += target.raw_name;
            std::string new_path = current_path;
            if (new_path.back() != '/') new_path += '/';
            new_path += prompt_input_buffer;
            const bool renamed = sd->rename(old_path.c_str(), new_path.c_str());
            if (HardwareCallback) HardwareCallback(renamed ? UIEvent::RENAMED : UIEvent::ERROR);
         }
      }
      current_state = EngineState::EXPLORER;
      needs_full_redraw = true;
      return;
   }
   if (input == '\b' || input == 0x7F) {
      if (!prompt_input_buffer.empty()) prompt_input_buffer.pop_back();
   }
   else if (input >= 32 && input <= 126 && prompt_input_buffer.length() < 64) {
      prompt_input_buffer += (char)input;
   }
   FillLine(Height, style.BackgroundColor, true);
   serial->write(Font::REVERSE);
   WriteClipped(prompt_title + prompt_input_buffer);
   serial->write(Font::RESET);
   serial->flush();
}

inline void NYXUS_FILE_EXPLORER::SetStyleConfig(const StyleConfig& __nConfig){
   style = __nConfig;
}

[[nodiscard]] inline StyleConfig& NYXUS_FILE_EXPLORER::GetStyleConfig(){
   return style;
}

inline void NYXUS_FILE_EXPLORER::AddNewExtensionColor(const char* EXT, const char* COL){
   if (!style.ExtensionColors.has_value()){
      style.ExtensionColors = std::unordered_map<std::string, const char *>();
   }
   style.ExtensionColors.value()[EXT] = COL;
}

inline void NYXUS_FILE_EXPLORER::RemoveExtensionColor(const char* EXT){
   if (!style.ExtensionColors.has_value()){
      return;
   }
   style.ExtensionColors.value().erase(EXT);
}

inline void NYXUS_FILE_EXPLORER::SetTargetBufferSize(uint32_t Size){
   if (Size == 0) {
      if (serial) serial->printf("[%sERROR%s] Buffer size cant be set to 0\n", Color::RED, Color::RESET);
      return;
   }
   BufferTextSize = Size > 65535UL ? 65535 : static_cast<uint16_t>(Size);
   if (serial) serial->printf("[%sEXPLORER%s] Buffer size set to %u bytes\n", Color::GREEN, Color::RESET, static_cast<unsigned>(BufferTextSize));
}

inline void NYXUS_FILE_EXPLORER::SetTargetFPS(uint8_t FPS){
   if (FPS < 1){
      serial->printf("[%sERROR%s] FPS cant be set to %d\n", Color::RED, Color::RESET, FPS);
      return;
   }
   this->FPS = FPS;
   serial->printf("[%sEXPLORER%s] FPS set to %d\n", Color::GREEN, Color::RESET, FPS);
}

inline void NYXUS_FILE_EXPLORER::SetTargetSD(SdFat* SDptr){
   sd = SDptr;
   serial->printf("[%sEXPLORER%s] SD card connected: ", Color::GREEN, Color::RESET);
   sd->printFatType(serial);
   serial->println();
}

inline void NYXUS_FILE_EXPLORER::SetTargetSerial(Stream* Serialptr){
   if (!Serialptr) return;
   serial = Serialptr;
   serial->printf("[%sEXPLORER%s] Connected to Serial: %p\n", Color::GREEN, Color::RESET, serial);
}

inline void NYXUS_FILE_EXPLORER::SetTargetPath(const std::string& Path){
   if (sd->exists(Path.c_str())){
      current_path = Path;
      serial->printf("[%sEXPLORER%s] Path set to: %s\n", Color::GREEN, Color::RESET, current_path.c_str());
   }
   else{
      serial->printf("[%sERROR%s] %s does not exist.\n", Color::RED, Color::RESET, Path.c_str());
   }
}

[[nodiscard]] inline bool NYXUS_FILE_EXPLORER::ExplorerShouldEnd(){
   return !serial->available();
}

[[nodiscard]] inline uint8_t NYXUS_FILE_EXPLORER::PollKeyboard() {
   if (!serial->available()) return Key::NONE;
   uint8_t c = serial->read();
   static uint8_t last_newline = 0;
   if (c == '\r' || c == '\n') {
      if (last_newline != 0 && last_newline != c) {
         last_newline = 0;
         return Key::NONE;
      }
      last_newline = c;
      return Key::ENTER;
   }
   else last_newline = 0;
   if (c == Key::ESC) {
      uint32_t timeout = millis();
      while (serial->available() < 2) {
         if (millis() - timeout > 5) return Key::ESC;
      }
      if (serial->read() == '[') {
         uint8_t dir = serial->read();
         if (dir == 'A') return Key::UP;
         if (dir == 'B') return Key::DOWN;
         if (dir == 'C') return Key::RIGHT;
         if (dir == 'D') return Key::LEFT;
      }
      return Key::ESC;
   }
   return c;
}

[[nodiscard]] inline uint16_t NYXUS_FILE_EXPLORER::RenderWidth() const {
   return Width > 1 ? Width - 1 : 1;
}

inline void NYXUS_FILE_EXPLORER::WriteClipped(const std::string& text, uint32_t offset, uint16_t width) {
   if (offset >= text.length()) return;
   size_t count = text.length() - offset;
   const size_t max_width = width > 0 ? width : RenderWidth();
   if (count > max_width) count = max_width;
   serial->write(reinterpret_cast<const uint8_t*>(text.data() + offset), count);
}

inline void NYXUS_FILE_EXPLORER::FillLine(uint16_t row, const char* background, bool reverse) {
   static constexpr char spaces[] = "                                                                ";
   uint16_t remaining = Width > 0 ? Width : 1;
   serial->write(Cursor::TOXY(1, row));
   serial->write(Font::RESET);
   serial->write(background);
   if (reverse) serial->write(Font::REVERSE);
   while (remaining > 0) {
      const uint16_t chunk = remaining > sizeof(spaces) - 1 ? sizeof(spaces) - 1 : remaining;
      serial->write(reinterpret_cast<const uint8_t*>(spaces), chunk);
      remaining -= chunk;
   }
   serial->write(Cursor::TOXY(1, row));
}

[[nodiscard]] inline std::string NYXUS_FILE_EXPLORER::Ellipsize(const std::string& text, uint16_t width) const {
   if (width == 0) return "";
   if (text.length() <= width) return text;
   if (width <= 3) return std::string(width, '.');
   return text.substr(0, width - 3) + "...";
}

[[nodiscard]] inline char* NYXUS_FILE_EXPLORER::GetExtension(char* start, const size_t& len, const bool& includeDot){
   for(char* i = start + len - 1; i >= start; --i){
      if (*i == '.'){
         return includeDot ? i : i + 1;
      }
   }
   return start + len;
}

[[nodiscard]] inline bool NYXUS_FILE_EXPLORER::Rm_RF_Directory(const std::string& target_path) {
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

inline void NYXUS_FILE_EXPLORER::RunEditor(uint8_t input) {
   FileNode& target = view_cache[selected_index - top_index];
   std::string full_target = current_path;
   if (full_target.back() != '/') full_target += '/';
   full_target += target.raw_name;
   static EngineState last_known_state = EngineState::EXPLORER;
   bool full_redraw = (last_known_state != EngineState::EDITOR) || needs_full_redraw;
   last_known_state = EngineState::EDITOR;
   bool line_redraw = false;
   bool status_redraw = false;
   if (editor_buffer.empty()) editor_buffer.push_back("");
   auto visual_active = [&]() -> bool {
      return editor_mode == EditorMode::VISUAL || editor_mode == EditorMode::VISUAL_BLOCK;
   };
   auto clamp_cursor = [&]() {
      if (editor_buffer.empty()) editor_buffer.push_back("");
      if (cursor_y >= editor_buffer.size()) cursor_y = editor_buffer.size() - 1;
      uint32_t max_col = editor_buffer[cursor_y].length();
      if (editor_mode != EditorMode::INSERT && max_col > 0) max_col--;
      if (cursor_x > max_col) cursor_x = max_col;
   };
   auto mark_motion_redraw = [&]() {
      if (visual_active()) full_redraw = true;
      else status_redraw = true;
   };
   auto begin_visual = [&](EditorMode mode) {
      if (editor_mode == mode) {
         editor_mode = EditorMode::NORMAL;
      }
      else {
         editor_mode = mode;
         visual_anchor_x = cursor_x;
         visual_anchor_y = cursor_y;
      }
      pending_cmd = 0;
      pending_operator = 0;
      pending_text_object = 0;
      full_redraw = true;
   };
   auto is_word_char = [](char ch) -> bool {
      return ch != ' ' && ch != '\t';
   };
   auto move_word_forward = [&]() {
      uint32_t y = cursor_y;
      uint32_t x = cursor_x;
      while (y < editor_buffer.size()) {
         const std::string& line = editor_buffer[y];
         if (x < line.length()) {
            if (is_word_char(line[x])) {
               while (x < line.length() && is_word_char(line[x])) x++;
            }
            while (x < line.length() && !is_word_char(line[x])) x++;
            if (x < line.length()) {
               cursor_y = y;
               cursor_x = x;
               return;
            }
         }
         y++;
         x = 0;
      }
      cursor_y = editor_buffer.size() - 1;
      cursor_x = editor_buffer[cursor_y].empty() ? 0 : editor_buffer[cursor_y].length() - 1;
   };
   auto word_object_range = [&](std::pair<uint32_t, uint32_t> pos) {
      uint32_t y = pos.first;
      uint32_t x = pos.second;
      if (y >= editor_buffer.size()) return std::make_pair(pos, pos);
      std::string& line = editor_buffer[y];
      if (x < line.length() && is_word_char(line[x])) {
         uint32_t start = x;
         while (start > 0 && is_word_char(line[start - 1])) start--;
         uint32_t end = x;
         while (end + 1 < line.length() && is_word_char(line[end + 1])) end++;
         return std::make_pair(std::make_pair(y, start), std::make_pair(y, end));
      }
      uint32_t scan = x;
      while (scan < line.length() && !is_word_char(line[scan])) scan++;
      if (scan < line.length()) {
         uint32_t start = scan;
         uint32_t end = scan;
         while (end + 1 < line.length() && is_word_char(line[end + 1])) end++;
         return std::make_pair(std::make_pair(y, start), std::make_pair(y, end));
      }
      return std::make_pair(pos, pos);
   };
   auto move_word_backward = [&]() {
      uint32_t y = cursor_y;
      uint32_t x = cursor_x;
      while (true) {
         if (x > 0) x--;
         else {
            if (y == 0) {
               cursor_y = 0;
               cursor_x = 0;
               return;
            }
            y--;
            x = editor_buffer[y].length();
            if (x == 0) continue;
            x--;
         }
         const std::string& line = editor_buffer[y];
         while (x > 0 && !is_word_char(line[x])) x--;
         if (is_word_char(line[x])) {
            while (x > 0 && is_word_char(line[x - 1])) x--;
            cursor_y = y;
            cursor_x = x;
            return;
         }
      }
   };
   auto normalize_range = [&](std::pair<uint32_t, uint32_t> a, std::pair<uint32_t, uint32_t> b) {
      if (a.first > b.first || (a.first == b.first && a.second > b.second)) std::swap(a, b);
      return std::pair<std::pair<uint32_t, uint32_t>, std::pair<uint32_t, uint32_t>>(a, b);
   };
   auto line_end = [&](uint32_t y) {
      return editor_buffer[y].empty() ? 0 : editor_buffer[y].length() - 1;
   };
   auto move_by_motion = [&](std::pair<uint32_t, uint32_t> position, uint8_t key) {
      uint32_t y = position.first;
      uint32_t x = position.second;
      if (key == 'h' || key == Key::LEFT) {
         if (x > 0) x--;
      }
      else if (key == 'j' || key == Key::DOWN) {
         if (y + 1 < editor_buffer.size()) {
            y++;
            x = std::min<uint32_t>(x, editor_buffer[y].length());
         }
      }
      else if (key == 'k' || key == Key::UP) {
         if (y > 0) {
            y--;
            x = std::min<uint32_t>(x, editor_buffer[y].length());
         }
      }
      else if (key == 'l' || key == Key::RIGHT) {
         if (x < editor_buffer[y].length()) x++;
      }
      else if (key == 'w') {
         uint32_t ty = y;
         uint32_t tx = x;
         while (ty < editor_buffer.size()) {
            const std::string& line = editor_buffer[ty];
            while (tx < line.length() && is_word_char(line[tx])) tx++;
            while (tx < line.length() && !is_word_char(line[tx])) tx++;
            if (tx < line.length()) break;
            ty++;
            tx = 0;
         }
         if (ty >= editor_buffer.size()) {
            ty = editor_buffer.size() - 1;
            tx = line_end(ty);
         }
         if (ty == y && tx > 0) x = tx - 1;
         else { y = ty; x = tx; }
      }
      else if (key == 'b') {
         if (x == 0 && y == 0) return std::make_pair(y, x);
         if (x == 0) {
            y--;
            x = editor_buffer[y].length();
            if (x > 0) x--;
         }
         while (true) {
            if (x == 0) break;
            const std::string& line = editor_buffer[y];
            if (is_word_char(line[x])) {
               while (x > 0 && is_word_char(line[x - 1])) x--;
               break;
            }
            x--;
         }
      }
      else if (key == 'G') {
         y = editor_buffer.size() - 1;
         x = 0;
      }
      else if (key == '0') {
         x = 0;
      }
      else if (key == '$') {
         x = line_end(y);
      }
      return std::make_pair(y, x);
   };
   auto yank_range = [&](std::pair<uint32_t, uint32_t> a, std::pair<uint32_t, uint32_t> b, bool linewise) {
      editor_yank_register.clear();
      yank_linewise = linewise;
      auto range = normalize_range(a, b);
      auto start = range.first;
      auto end = range.second;
      if (linewise) {
         for (uint32_t line = start.first; line <= end.first && line < editor_buffer.size(); ++line) {
            editor_yank_register.push_back(editor_buffer[line]);
         }
      }
      else if (start.first == end.first) {
         if (start.second <= end.second) {
            editor_yank_register.push_back(editor_buffer[start.first].substr(start.second, end.second - start.second + 1));
         }
      }
      else {
         editor_yank_register.push_back(editor_buffer[start.first].substr(start.second));
         for (uint32_t line = start.first + 1; line < end.first; ++line) {
            editor_yank_register.push_back(editor_buffer[line]);
         }
         editor_yank_register.push_back(editor_buffer[end.first].substr(0, end.second + 1));
      }
   };
   auto delete_range = [&](std::pair<uint32_t, uint32_t> a, std::pair<uint32_t, uint32_t> b) {
      auto range = normalize_range(a, b);
      auto start = range.first;
      auto end = range.second;
      if (start.first == end.first) {
         if (start.second < editor_buffer[start.first].length()) {
            editor_buffer[start.first].erase(start.second, end.second - start.second + 1);
         }
         cursor_y = start.first;
         cursor_x = std::min<uint32_t>(start.second, editor_buffer[cursor_y].length());
      }
      else {
         std::string suffix = editor_buffer[end.first].substr(std::min<uint32_t>(end.second + 1, editor_buffer[end.first].length()));
         editor_buffer[start.first].erase(start.second);
         editor_buffer[start.first] += suffix;
         editor_buffer.erase(editor_buffer.begin() + start.first + 1, editor_buffer.begin() + end.first + 1);
         cursor_y = start.first;
         cursor_x = start.second;
         if (cursor_x > editor_buffer[cursor_y].length()) cursor_x = editor_buffer[cursor_y].length();
      }
      if (editor_buffer.empty()) editor_buffer.push_back("");
      return true;
   };
   auto save_undo = [&]() {
      if (undo_stack.size() >= 50) undo_stack.erase(undo_stack.begin());
      undo_stack.push_back({editor_buffer, cursor_x, cursor_y});
      redo_stack.clear();
   };
   auto restore_history = [&](std::vector<EditorHistory>& source, std::vector<EditorHistory>& target) -> bool {
      if (source.empty()) return false;
      target.push_back({editor_buffer, cursor_x, cursor_y});
      EditorHistory entry = std::move(source.back());
      source.pop_back();
      editor_buffer = std::move(entry.buffer);
      cursor_x = entry.cursor_x;
      cursor_y = entry.cursor_y;
      editor_scroll_x = editor_scroll_y = 0;
      full_redraw = true;
      return true;
   };
   auto flash_status = [&](const char* msg) {
      FillLine(Height, style.BackgroundColor, true);
      serial->write(Font::REVERSE);
      WriteClipped(msg);
      serial->write(Font::RESET);
      serial->flush();
   };
   auto apply_operator = [&](uint8_t op, std::pair<uint32_t, uint32_t> start, std::pair<uint32_t, uint32_t> end, bool linewise) {
      bool result = false;
      if (op == 'y') {
         yank_range(start, end, linewise);
         result = true;
      }
      else if (op == 'd') {
         save_undo();
         delete_range(start, end);
         flash_status(" DELETED ");
         result = true;
      }
      else if (op == 'c') {
         save_undo();
         delete_range(start, end);
         editor_mode = EditorMode::INSERT;
         serial->write(Cursor::LINE);
         flash_status(" CHANGED ");
         result = true;
      }
      if (result) full_redraw = true;
      return result;
   };
   auto paste_after = [&]() {
      if (editor_yank_register.empty()) return;
      save_undo();
      if (yank_linewise) {
         editor_buffer.insert(editor_buffer.begin() + cursor_y + 1, editor_yank_register.begin(), editor_yank_register.end());
         cursor_y += editor_yank_register.size();
         cursor_x = 0;
      } else {
         std::string& line = editor_buffer[cursor_y];
         line.insert(cursor_x + 1, editor_yank_register[0]);
         cursor_x += editor_yank_register[0].length();
         if (editor_yank_register.size() > 1) {
            editor_buffer.insert(editor_buffer.begin() + cursor_y + 1, editor_yank_register.begin() + 1, editor_yank_register.end());
            cursor_y += editor_yank_register.size() - 1;
         }
      }
      last_edit_command = "p";
      flash_status(" PASTED ");
      full_redraw = true;
   };
   auto paste_before = [&]() {
      if (editor_yank_register.empty()) return;
      save_undo();
      if (yank_linewise) {
         editor_buffer.insert(editor_buffer.begin() + cursor_y, editor_yank_register.begin(), editor_yank_register.end());
         cursor_x = 0;
      } else {
         std::string& line = editor_buffer[cursor_y];
         line.insert(cursor_x, editor_yank_register[0]);
         cursor_x += editor_yank_register[0].length();
         if (editor_yank_register.size() > 1) {
            editor_buffer.insert(editor_buffer.begin() + cursor_y + 1, editor_yank_register.begin() + 1, editor_yank_register.end());
            cursor_y += editor_yank_register.size() - 1;
         }
      }
      last_edit_command = "P";
      flash_status(" PASTED ");
      full_redraw = true;
   };
   auto handle_operator = [&](uint8_t op, uint8_t key) -> bool {
      if (key == 'i') {
         pending_text_object = 'i';
         return true;
      }
      if (key == 'd' && op == 'd') {
         std::pair<uint32_t, uint32_t> here{cursor_y, 0};
         std::pair<uint32_t, uint32_t> end{cursor_y, line_end(cursor_y)};
         bool result = apply_operator(op, here, end, true);
         if (result) last_edit_command = "dd";
         pending_operator = 0;
         return result;
      }
      if (key == 'y' && op == 'y') {
         std::pair<uint32_t, uint32_t> here{cursor_y, 0};
         std::pair<uint32_t, uint32_t> end{cursor_y, line_end(cursor_y)};
         bool result = apply_operator(op, here, end, true);
         if (result) {
            last_edit_command = "yy";
            flash_status(" YANKED ");
         }
         pending_operator = 0;
         return result;
      }
      if (key == 'c' && op == 'c') {
         std::pair<uint32_t, uint32_t> here{cursor_y, 0};
         std::pair<uint32_t, uint32_t> end{cursor_y, line_end(cursor_y)};
         bool result = apply_operator(op, here, end, true);
         if (result) last_edit_command = "cc";
         pending_operator = 0;
         return result;
      }
      if (pending_text_object == 'i' && key == 'w') {
         auto word_range = word_object_range({cursor_y, cursor_x});
         bool result = apply_operator(op, word_range.first, word_range.second, false);
         if (result) {
            if (op == 'd') last_edit_command = "diw";
            if (op == 'y') {
               last_edit_command = "yiw";
               flash_status(" YANKED ");
            }
            if (op == 'c') last_edit_command = "ciw";
         }
         pending_operator = 0;
         pending_text_object = 0;
         return result;
      }
      if (key == '0' || key == '$' || key == 'h' || key == 'j' || key == 'k' || key == 'l' || key == 'w' || key == 'b' || key == 'G') {
         std::pair<uint32_t, uint32_t> start{cursor_y, cursor_x};
         std::pair<uint32_t, uint32_t> end = move_by_motion(start, key);
         if (key == 'w' && end == start) end.second = line_end(end.first);
         if (start == end && key == 'h' && cursor_x > 0) start.second = cursor_x - 1;
         bool result = apply_operator(op, start, end, false);
         if (result) {
            last_edit_command = std::string(1, op) + std::string(1, key);
         }
         pending_operator = 0;
         pending_text_object = 0;
         return result;
      }
      pending_operator = 0;
      pending_text_object = 0;
      return false;
   };
   auto repeat_last_edit = [&]() -> bool {
      if (last_edit_command == "dd") {
         return handle_operator('d', 'd');
      }
      if (last_edit_command == "yy") {
         return handle_operator('y', 'y');
      }
      if (last_edit_command == "cc") {
         return handle_operator('c', 'c');
      }
      if (last_edit_command == "yiw") {
         pending_operator = 'y';
         pending_text_object = 'i';
         return handle_operator('y', 'w');
      }
      if (last_edit_command == "p") {
         paste_after();
         return true;
      }
      if (last_edit_command == "P") {
         paste_before();
         return true;
      }
      if (last_edit_command == "ciw") {
         pending_operator = 'c';
         pending_text_object = 'i';
         return handle_operator('c', 'w');
      }
      if (last_edit_command == "diw") {
         pending_operator = 'd';
         pending_text_object = 'i';
         return handle_operator('d', 'w');
      }
      return false;
   };
   auto handle_motion = [&](uint8_t key) -> uint8_t {
      if (key == 'g') {
         if (pending_cmd == 'g') {
            cursor_y = 0;
            cursor_x = 0;
            pending_cmd = 0;
            full_redraw = true;
            return 1;
         }
         pending_cmd = 'g';
         return 2;
      }
      if (pending_operator) {
         if (handle_operator(pending_operator, key)) {
            full_redraw = true;
            return 1;
         }
      }
      pending_operator = 0;
      pending_text_object = 0;
      if (key == 'h' || key == Key::LEFT) {
         if (cursor_x > 0) cursor_x--;
         mark_motion_redraw();
         return 1;
      }
      if (key == 'j' || key == Key::DOWN) {
         if (cursor_y + 1 < editor_buffer.size()) cursor_y++;
         mark_motion_redraw();
         return 1;
      }
      if (key == 'k' || key == Key::UP) {
         if (cursor_y > 0) cursor_y--;
         mark_motion_redraw();
         return 1;
      }
      if (key == 'l' || key == Key::RIGHT) {
         if (cursor_x < editor_buffer[cursor_y].length()) cursor_x++;
         mark_motion_redraw();
         return 1;
      }
      if (key == 'w') {
         move_word_forward();
         mark_motion_redraw();
         return 1;
      }
      if (key == 'b') {
         move_word_backward();
         mark_motion_redraw();
         return 1;
      }
      if (key == 'G') {
         cursor_y = editor_buffer.size() - 1;
         cursor_x = 0;
         full_redraw = true;
         return 1;
      }
      if (key == '0') {
         cursor_x = 0;
         mark_motion_redraw();
         return 1;
      }
      if (key == '$') {
         cursor_x = editor_buffer[cursor_y].empty() ? 0 : editor_buffer[cursor_y].length() - 1;
         mark_motion_redraw();
         return 1;
      }
      return 0;
   };
   clamp_cursor();
   if (input != Key::NONE) {
      if (input == Key::ESC) {
         if (editor_mode == EditorMode::INSERT) {
            if (cursor_x > 0) cursor_x--;
         }
         if (visual_active() || editor_mode == EditorMode::INSERT || editor_mode == EditorMode::COMMAND) full_redraw = true;
         editor_mode = EditorMode::NORMAL;
         editor_cmd_buffer.clear();
         pending_cmd = 0;
         pending_operator = 0;
         pending_text_object = 0;
         visual_anchor_x = cursor_x;
         visual_anchor_y = cursor_y;
         serial->write(Cursor::BLOCK);
      }
      else if (editor_mode == EditorMode::NORMAL || visual_active()) {
         if (input == 'v') {
            begin_visual(EditorMode::VISUAL);
         }
         else if (input == 'V') {
            begin_visual(EditorMode::VISUAL_BLOCK);
         }
         else if (editor_mode == EditorMode::NORMAL && pending_operator && input == 'i') {
            handle_operator(pending_operator, input);
         }
         else if (editor_mode == EditorMode::NORMAL && (input == 'd' || input == 'y' || input == 'c')) {
            if (pending_operator == input) {
               handle_operator(input, input);
            } else {
               pending_operator = input;
            }
         }
         else if (editor_mode == EditorMode::NORMAL && input == 'u') {
            if (restore_history(undo_stack, redo_stack)) flash_status(" UNDONE ");
            else flash_status(" NO UNDO ");
            full_redraw = true;
         }
         else if (editor_mode == EditorMode::NORMAL && input == 0x12) {
            if (restore_history(redo_stack, undo_stack)) flash_status(" REDO ");
            else flash_status(" NO REDO ");
            full_redraw = true;
         }
         else if (editor_mode == EditorMode::NORMAL && input == '.') {
            if (repeat_last_edit()) {
               flash_status(" REPEAT ");
            }
            else {
               flash_status(" NOTHING ");
            }
            full_redraw = true;
         }
         else if (visual_active() && (input == 'd' || input == 'c' || input == 'y')) {
            uint32_t start_y = visual_anchor_y < cursor_y ? visual_anchor_y : cursor_y;
            uint32_t end_y = visual_anchor_y < cursor_y ? cursor_y : visual_anchor_y;
            uint32_t start_x = visual_anchor_y < cursor_y ? visual_anchor_x : cursor_x;
            uint32_t end_x = visual_anchor_y < cursor_y ? cursor_x : visual_anchor_x;
            if (start_y == end_y) {
               if (start_x > end_x) std::swap(start_x, end_x);
            }
            else if (start_y == end_y && start_x > end_x) {
               std::swap(start_x, end_x);
            }
            if (input == 'd') {
               save_undo();
               apply_operator('d', {start_y, start_x}, {end_y, end_x}, false);
               editor_mode = EditorMode::NORMAL;
            }
            else if (input == 'c') {
               save_undo();
               apply_operator('c', {start_y, start_x}, {end_y, end_x}, false);
            }
            else {
               apply_operator('y', {start_y, start_x}, {end_y, end_x}, false);
               editor_mode = EditorMode::NORMAL;
            }
            full_redraw = true;
         }
         else if (editor_mode == EditorMode::NORMAL && (input == 'p' || input == 'P')) {
            if (input == 'p') paste_after();
            else paste_before();
         }
         else if (editor_mode == EditorMode::NORMAL && input == 'i') {
            editor_mode = EditorMode::INSERT;
            pending_cmd = 0;
            pending_operator = 0;
            pending_text_object = 0;
            serial->write(Cursor::LINE);
            full_redraw = true;
         }
         else if (editor_mode == EditorMode::NORMAL && input == 'a') {
            editor_mode = EditorMode::INSERT;
            pending_cmd = 0;
            pending_operator = 0;
            pending_text_object = 0;
            if (cursor_x < editor_buffer[cursor_y].length()) cursor_x++;
            serial->write(Cursor::LINE);
            full_redraw = true;
         }
         else if (editor_mode == EditorMode::NORMAL && input == ':') {
            editor_mode = EditorMode::COMMAND;
            editor_cmd_buffer = ":";
            pending_cmd = 0;
            pending_operator = 0;
            pending_text_object = 0;
            full_redraw = true;
         }
         else {
            uint8_t motion_result = handle_motion(input);
            if (motion_result == 2) return;
         }
      }
      else if (editor_mode == EditorMode::INSERT) {
         if (input == Key::ENTER) {
            save_undo();
            std::string remainder = editor_buffer[cursor_y].substr(cursor_x);
            editor_buffer[cursor_y] = editor_buffer[cursor_y].substr(0, cursor_x);
            editor_buffer.insert(editor_buffer.begin() + cursor_y + 1, remainder);
            cursor_y++;
            cursor_x = 0;
            full_redraw = true;
         }
         else if (input == '\b' || input == 0x7F) {
            if (cursor_x > 0) {
               save_undo();
               editor_buffer[cursor_y].erase(cursor_x - 1, 1);
               cursor_x--;
               line_redraw = true;
            }
            else if (cursor_y > 0) {
               save_undo();
               cursor_x = editor_buffer[cursor_y - 1].length();
               editor_buffer[cursor_y - 1] += editor_buffer[cursor_y];
               editor_buffer.erase(editor_buffer.begin() + cursor_y);
               cursor_y--;
               full_redraw = true;
            }
         }
         else if (input == Key::LEFT) {
            if (cursor_x > 0) cursor_x--;
            status_redraw = true;
         }
         else if (input == Key::RIGHT) {
            if (cursor_x < editor_buffer[cursor_y].length()) cursor_x++;
            status_redraw = true;
         }
         else if (input == Key::UP) {
            if (cursor_y > 0) cursor_y--;
            status_redraw = true;
         }
         else if (input == Key::DOWN) {
            if (cursor_y + 1 < editor_buffer.size()) cursor_y++;
            status_redraw = true;
         }
         else if (input >= 32 && input <= 126) {
            save_undo();
            editor_buffer[cursor_y].insert(cursor_x, 1, (char)input);
            cursor_x++;
            line_redraw = true;
         }
      }
      else if (editor_mode == EditorMode::COMMAND) {
         if (input == Key::ENTER) {
            if (editor_cmd_buffer == ":w" || editor_cmd_buffer == ":wq") {
               FsFile f = sd->open(full_target.c_str(), O_WRITE | O_CREAT | O_TRUNC);
               if (f) {
                  for (const auto& line : editor_buffer) f.println(line.c_str());
                  f.close();
               }
            }
            if (editor_cmd_buffer == ":q" || editor_cmd_buffer == ":wq") {
               current_state = EngineState::EXPLORER;
               last_known_state = EngineState::EXPLORER;
               editor_buffer.clear();
               needs_full_redraw = true;
               serial->write(Cursor::HIDE);
               serial->flush();
               return;
            }
            editor_mode = EditorMode::NORMAL;
            editor_cmd_buffer.clear();
            full_redraw = true;
         }
         else if (input == '\b' || input == 0x7F) {
            if (editor_cmd_buffer.length() > 1) editor_cmd_buffer.pop_back();
            else editor_mode = EditorMode::NORMAL;
            full_redraw = true;
         }
         else if (input >= 32 && input <= 126) {
            editor_cmd_buffer += (char)input;
            full_redraw = true;
         }
      }
      clamp_cursor();
      uint32_t old_scroll_y = editor_scroll_y;
      uint32_t old_scroll_x = editor_scroll_x;
      const uint32_t view_height = Height > 1 ? Height - 1 : 1;
      if (cursor_y < editor_scroll_y) editor_scroll_y = cursor_y;
      if (cursor_y >= editor_scroll_y + view_height) editor_scroll_y = cursor_y - view_height + 1;
      const uint32_t render_width = RenderWidth();
      if (cursor_x < editor_scroll_x) editor_scroll_x = cursor_x;
      if (cursor_x >= editor_scroll_x + render_width) editor_scroll_x = cursor_x - render_width + 1;
      if (editor_scroll_x != old_scroll_x || editor_scroll_y != old_scroll_y) full_redraw = true;
   }
   if (input == Key::NONE && !full_redraw) return;
   serial->write(Cursor::HIDE);
   const uint32_t view_height = Height > 1 ? Height - 1 : 1;
   const uint16_t safe_width = RenderWidth();
   auto editor_cell_selected = [&](uint32_t line, uint32_t column) -> bool {
      if (!visual_active()) return false;
      uint32_t start_y = visual_anchor_y;
      uint32_t end_y = cursor_y;
      uint32_t start_x = visual_anchor_x;
      uint32_t end_x = cursor_x;
      if (start_y > end_y) {
         uint32_t tmp_y = start_y;
         start_y = end_y;
         end_y = tmp_y;
         uint32_t tmp_x = start_x;
         start_x = end_x;
         end_x = tmp_x;
      }
      if (line < start_y || line > end_y) return false;
      if (editor_mode == EditorMode::VISUAL_BLOCK) {
         uint32_t min_x = visual_anchor_x < cursor_x ? visual_anchor_x : cursor_x;
         uint32_t max_x = visual_anchor_x < cursor_x ? cursor_x : visual_anchor_x;
         return column >= min_x && column <= max_x;
      }
      if (start_y == end_y) {
         uint32_t min_x = start_x < end_x ? start_x : end_x;
         uint32_t max_x = start_x < end_x ? end_x : start_x;
         return column >= min_x && column <= max_x;
      }
      if (line == start_y) return column >= start_x;
      if (line == end_y) return column <= end_x;
      return true;
   };
   auto render_editor_line = [&](uint32_t line_idx) {
      const std::string& line = editor_buffer[line_idx];
      if (!visual_active()) {
         WriteClipped(line, editor_scroll_x);
         return;
      }
      if (editor_scroll_x >= line.length()) return;
      uint32_t end_col = editor_scroll_x + safe_width;
      if (end_col > line.length()) end_col = line.length();
      bool reversed = false;
      for (uint32_t col = editor_scroll_x; col < end_col; ++col) {
         const bool selected = editor_cell_selected(line_idx, col);
         if (selected != reversed) {
            serial->write(selected ? Font::REVERSE : Font::RESET);
            if (!selected) serial->write(style.BackgroundColor);
            reversed = selected;
         }
         serial->write(reinterpret_cast<const uint8_t*>(line.data() + col), 1);
      }
      if (reversed) {
         serial->write(Font::RESET);
         serial->write(style.BackgroundColor);
      }
   };
   if (full_redraw) {
      serial->write(style.BackgroundColor);
      serial->write(Clear::ALL);
      for (uint32_t i = 0; i < view_height; ++i) {
         FillLine(i + 1, style.BackgroundColor);
         uint32_t line_idx = editor_scroll_y + i;
         if (line_idx < editor_buffer.size()) render_editor_line(line_idx);
      }
      const char* mode_badge = "𐦖 NORMAL ";
      const char* mode_background = BgColor::NVIMBLUE;
      const char* mode_foreground = Color::BLACK;
      const char* compact_mode = "NORMAL";
      uint16_t badge_width = 10;
      switch (editor_mode){
      case EditorMode::NORMAL:
         mode_background = BgColor::NVIMBLUE;
         mode_badge = "𐦖 NORMAL ";
         compact_mode = "NORMAL";
         badge_width = 10;
         break;
      case EditorMode::COMMAND:
         mode_background = BgColor::NVIMYELLOW;
         mode_badge = "𐦂 COMMAND ";
         compact_mode = "COMMAND";
         badge_width = 11;
         break;
      case EditorMode::INSERT:
         mode_background = BgColor::NVIMGREEN;
         mode_badge = "𐦉 INSERT ";
         compact_mode = "INSERT";
         badge_width = 10;
         break;
      case EditorMode::VISUAL:
         mode_background = BgColor::NVIMPURPLE;
         mode_badge = "𖠀 VISUAL ";
         compact_mode = "VISUAL";
         badge_width = 10;
         break;
      case EditorMode::VISUAL_BLOCK:
         mode_background = BgColor::NVIMPURPLE;
         mode_badge = "𐦟 V-BLOCK ";
         compact_mode = "V-BLOCK";
         badge_width = 11;
         break;
      default:
         break;
      }
      FillLine(TERMINAL_BOTTOM_ROW, mode_background);
      const uint16_t left_width = safe_width / 4;
      const uint16_t right_width = safe_width / 4;
      const uint16_t middle_width = safe_width - left_width - right_width;
      uint16_t drawn_badge_width = badge_width;
      serial->write(Cursor::TOXY(1, TERMINAL_BOTTOM_ROW));
      serial->write(Font::RESET);
      serial->write(mode_foreground);
      serial->write(mode_background);
      if (left_width >= badge_width) serial->write(mode_badge);
      else {
         const std::string compact_badge = Ellipsize(compact_mode, left_width);
         WriteClipped(compact_badge, 0, left_width);
         drawn_badge_width = compact_badge.length();
      }
      std::string command_display;
      if (editor_mode == EditorMode::COMMAND && left_width > drawn_badge_width) {
         const uint16_t command_width = left_width - drawn_badge_width;
         command_display = Ellipsize(editor_cmd_buffer, command_width);
         serial->write(Color::BLACK);
         WriteClipped(command_display, 0, command_width);
      }
      if (middle_width > 0) {
         const std::string filename = Ellipsize(target.raw_name, middle_width);
         uint16_t filename_x = (Width / 2) - (filename.length() / 2) + 1;
         uint16_t left_boundary = drawn_badge_width + command_display.length() + 2;
         if (filename_x < left_boundary) filename_x = left_boundary;
         serial->write(Cursor::TOXY(filename_x, TERMINAL_BOTTOM_ROW));
         serial->write(Color::BLACK);
         WriteClipped(filename, 0, middle_width);
      }
      needs_full_redraw = false;
      status_redraw = true;
   }
   else if (line_redraw) {
      serial->write(Cursor::TOXY(1, (cursor_y - editor_scroll_y) + 1));
      serial->write(style.BackgroundColor);
      serial->write(Clear::LNE);
      render_editor_line(cursor_y);
      status_redraw = true;
   }
   if (status_redraw) {
      char position_buffer[48];
      snprintf(position_buffer, sizeof(position_buffer), " Ln %lu, Col %lu", static_cast<unsigned long>(cursor_y + 1), static_cast<unsigned long>(cursor_x + 1));
      const uint16_t right_width = safe_width / 4;
      if (right_width > 0) {
         const std::string position = Ellipsize(position_buffer, right_width);
         const uint16_t position_x = Width - position.length() + 1;
         serial->write(Cursor::TOXY(position_x, TERMINAL_BOTTOM_ROW));
         serial->write(Color::BLACK);
         serial->write(reinterpret_cast<const uint8_t*>(position.data()), position.length());
      }
   }
   serial->write(Color::BLACK);
   if (editor_mode == EditorMode::COMMAND) {
      uint16_t left_width = safe_width / 4;
      uint16_t badge_width = 9;
      uint16_t drawn_badge_width = (left_width >= badge_width) ? badge_width : left_width;
      uint16_t command_width = left_width - drawn_badge_width;
      std::string command_display = Ellipsize(editor_cmd_buffer, command_width);
      uint16_t command_x = drawn_badge_width + command_display.length() + 1;
      if (command_x > left_width) command_x = left_width;
      if (command_x < 1) command_x = 1;
      serial->write(Cursor::TOXY(command_x, TERMINAL_BOTTOM_ROW));
   }
   else serial->write(Cursor::TOXY((cursor_x - editor_scroll_x) + 1, (cursor_y - editor_scroll_y) + 1));
   serial->write(Cursor::SHOW);
   serial->flush();
}

inline void NYXUS_FILE_EXPLORER::ExecutePaste() {
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
      FillLine(Height, style.BackgroundColor, true);
      serial->write(Font::REVERSE);
      WriteClipped(" ERROR: DIRECTORY COPY NOT SUPPORTED ");
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

inline void NYXUS_FILE_EXPLORER::RunPager(uint8_t input) {
   if (input == 'q' || input == 'Q' || input == Key::ESC) {
      current_state = EngineState::EXPLORER;
      pager_line_offsets.clear();
      needs_full_redraw = true;
      serial->write(Cursor::HIDE);
      serial->flush();
      return;
   }
   if (input == Key::DOWN || input == Key::ENTER || input == 's' || input == 'S') pager_offset++;
   else if (input == Key::UP || input == 'w' || input == 'W') {
      if (pager_offset > 0) pager_offset--;
   }
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
   serial->write(Cursor::HIDE);
   serial->write(Clear::ALL);
   if (pager_offset < pager_line_offsets.size()) f.seek(pager_line_offsets[pager_offset]);
   const uint32_t view_height = Height > 1 ? Height - 1 : 1;
   for(uint32_t i = 0; i < view_height; ++i) {
      FillLine(i + 1, style.BackgroundColor, true);
      char line[256];
      if(f.available()) {
         int n = f.fgets(line, sizeof(line));
         if(n > 0) {
            while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) line[--n] = '\0';
            WriteClipped(std::string(line, n));
         }
      }
      else break;
   }
   f.close();
   FillLine(Height, style.BackgroundColor);
   serial->write(Font::REVERSE);
   WriteClipped("𐦝 PAGER: " + target.raw_name + " | Q to exit | Up/Down to scroll ");
   serial->write(Font::RESET);
   serial->flush();
   needs_full_redraw = false;
}

inline void NYXUS_FILE_EXPLORER::Draw(uint8_t hardware_override){
   uint8_t input = (hardware_override != Key::NONE) ? hardware_override : PollKeyboard();
   if (current_state == EngineState::PAGER) {
      RunPager(input);
      return;
   }
   if (current_state == EngineState::PROMPT_DELETE) {
      RunDeletePrompt(input);
      return;
   }
   if (current_state == EngineState::PROMPT_MKDIR || current_state == EngineState::PROMPT_MKFILE || current_state == EngineState::PROMPT_RENAME) {
      RunPrompt(input, current_state);
      return;
   }
   if (current_state == EngineState::EDITOR) {
      RunEditor(input);
      return;
   }
   serial->write(Cursor::HIDE);
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
   if (input == Key::UP || input == 'w' || input == 'W') {
      if (selected_index > 0) {
         uint32_t old_index = selected_index;
         selected_index--;
         if (selected_index < top_index) {
            top_index = selected_index;
            needs_window_redraw = true;
         }
         else if (!needs_window_redraw) {
            FillLine((old_index - top_index) + 2, style.BackgroundColor);
            serial->write(view_cache[old_index - top_index].color);
            WriteClipped(view_cache[old_index - top_index].display_name);
            serial->write(Color::RESET);
            FillLine((selected_index - top_index) + 2, style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->write(view_cache[selected_index - top_index].color);
            WriteClipped(view_cache[selected_index - top_index].display_name);
            serial->write(Color::RESET);
            serial->write(Font::RESET);
            if (HardwareCallback) HardwareCallback(UIEvent::MOVEDUP);
         }
      }
   }
   else if (input == Key::DOWN || input == 's' || input == 'S') {
      if (selected_index < total_files - 1) {
         uint32_t old_index = selected_index;
         selected_index++;
         if (selected_index >= top_index + (Height - 2)) {
            top_index = selected_index - (Height - 2) + 1;
            needs_window_redraw = true;
         } else if (!needs_window_redraw) {
            FillLine((old_index - top_index) + 2, style.BackgroundColor);
            serial->write(view_cache[old_index - top_index].color);
            WriteClipped(view_cache[old_index - top_index].display_name);
            serial->write(Color::RESET);
            FillLine((selected_index - top_index) + 2, style.BackgroundColor);
            serial->write(Font::REVERSE);
            serial->write(view_cache[selected_index - top_index].color);
            WriteClipped(view_cache[selected_index - top_index].display_name);
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
      for (uint16_t row = 1; row <= Height; ++row) FillLine(row, style.BackgroundColor);
      serial->write(Cursor::TOXY(1, 1));
      serial->write(Font::REVERSE);
      WriteClipped("𐦓 PATH: " + current_path + " ");
      serial->write(Font::RESET);
      for (size_t i = 0; i < view_cache.size(); i++) {
         FillLine(i + 2, style.BackgroundColor);
         if (top_index + i == selected_index) serial->write(Font::REVERSE);
         serial->write(view_cache[i].color);
         WriteClipped(view_cache[i].display_name);
         serial->write(Color::RESET);
         serial->write(Font::RESET);
      }
      serial->flush();
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
      if (current_state == EngineState::PROMPT_DELETE){
         RunDeletePrompt(input);
         current_state = EngineState::EXPLORER;
      }
      else {
         FileNode& target = view_cache[selected_index - top_index];
         if (target.raw_name != "." && target.raw_name != "..") {
            clipboard_path = current_path;
            if (clipboard_path.back() != '/') clipboard_path += '/';
            clipboard_path += target.raw_name;
            FillLine(Height, style.BackgroundColor, true);
            serial->write(Font::REVERSE);
            WriteClipped(" YANKED: " + target.raw_name + " ");
            serial->write(Font::RESET);
            if (HardwareCallback) HardwareCallback(UIEvent::YANKED);
         }
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
   else if (input == 'o' || input == 'O') {
      FileNode& target = view_cache[selected_index - top_index];
      if (!target.is_dir) {
         std::string full_target = current_path;
         if (full_target.back() != '/') full_target += '/';
         full_target += target.raw_name;
         FsFile f = sd->open(full_target.c_str(), O_READ);
         if (f && f.fileSize() < MAX_EDITOR_FILE_SIZE) {
            editor_buffer.clear();
            char line_buf[256];
            while (f.fgets(line_buf, sizeof(line_buf)) > 0) {
               line_buf[strcspn(line_buf, "\r\n")] = 0;
               editor_buffer.push_back(line_buf);
            }
            if (editor_buffer.empty()) editor_buffer.push_back("");
            f.close();
            current_state = EngineState::EDITOR;
            editor_mode = EditorMode::NORMAL;
            cursor_x = 0; cursor_y = 0; editor_scroll_x = 0; editor_scroll_y = 0;
            visual_anchor_x = 0; visual_anchor_y = 0;
            RunEditor(Key::NONE);
            if (HardwareCallback) HardwareCallback(UIEvent::OPENED);
            return;
         } else {
            FillLine(Height, style.BackgroundColor, true);
            serial->write(Font::REVERSE);
            WriteClipped(" ERROR: FILE TOO LARGE FOR SRAM (Limit 64KB) ");
            serial->write(Font::RESET);
            if (f) f.close();
         }
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

inline void NYXUS_FILE_EXPLORER::Refresh(uint32_t ms){
   vTaskDelay(pdMS_TO_TICKS(ms + 1000 / FPS));
   if (!needs_full_redraw) serial->flush();
}

#endif
