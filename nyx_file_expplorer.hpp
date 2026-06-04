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
#ifndef _NEXUS_FILE_EXPLORER_HPP_
#define _NEXUS_FILE_EXPLORER_HPP_

#include <cstdint>
#include <SdFat.h>
#include <unordered_map>
#include <optional>
#include <string>
#include <pinout.hpp>
#include <HardwareSerial.h>
#include <nyx_terminal_graphics.hpp>

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

   private:
   /**
    * @brief Active directory path string.
    */
   const char* path = nullptr;

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
   NYXUS_FILE_EXPLORER(HWCDC* Serialptr, SdFat* SDptr, const StyleConfig& Style = {}, uint8_t Fps = 30, const char* Path = "/") : serial(Serialptr), sd(SDptr), style(Style), FPS(Fps), path(Path){
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
    * @param Path The absolute directory path to evaluate.
    * @paragraph Path Validation
    * Primes the UI to fetch and render the contents of the specified path on the next drawing cycle.
    * @return void
    */
   void SetTargetPath(const char* Path){
      path = Path;
      serial->printf("[%sEXPLORER%s] Path set to: %s\n", Color::GREEN, Color::RESET, path);
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
    * @param terminator The character required to trigger a successful capture array.
    * @param bufferSize The maximum byte length expected from the terminal.
    * @paragraph Memory Safety Architecture
    * Securely traps the thread using `readBytesUntil` and formats the captured payload into a safe C++ string. 
    * Completely eliminates the Use-After-Free heap crash associated with standard stack char arrays.
    * @return The sanitized string response extracted from the terminal interface.
    */
   std::string GetInputResponse(const char* prompt = nullptr, const char terminator = '\n', const size_t bufferSize = 64){
      if(serial->availableForWrite()){
         if(prompt) serial->write(prompt, strlen(prompt));
      }
      std::string result;
      result.resize(bufferSize);
      size_t ret = serial->readBytesUntil(terminator, &result[0], bufferSize);
      result.resize(ret);
      return result;
   }

   public:
   /**
    * @brief Executes the primary ANSI rendering matrix, drawing the active directory structure.
    * @paragraph Rendering Pipeline
    * Fetches the filesystem contents, applies the styled ANSI formatting map, and flushes it down the serial line.
    * @return void
    */
   void Draw();

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
      serial->flush();
   }
};
#endif