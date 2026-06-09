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
#ifndef _NYXUS_TERMINAL_GRAPHICS_HPP_
#define _NYXUS_TERMINAL_GRAPHICS_HPP_
#include <cstdint>
#include <utility>
#include <string.h>

namespace Color{
   inline constexpr const char* RESET       = "\033[000;000;000;000;000m";
   inline constexpr const char* BLACK       = "\033[038;002;000;000;000m";
   inline constexpr const char* RED         = "\033[038;002;255;000;000m";
   inline constexpr const char* GREEN       = "\033[038;002;000;255;000m";
   inline constexpr const char* BLUE        = "\033[038;002;000;000;255m";
   inline constexpr const char* YELLOW      = "\033[038;002;255;255;000m";
   inline constexpr const char* CYAN        = "\033[038;002;000;255;255m";
   inline constexpr const char* MAGENTA     = "\033[038;002;255;000;255m";
   inline constexpr const char* WHITE       = "\033[038;002;255;255;255m";
   inline constexpr const char* ORANGE      = "\033[038;002;255;165;000m";
   inline constexpr const char* PURPLE      = "\033[038;002;128;000;128m";
   inline constexpr const char* BROWN       = "\033[038;002;165;042;042m";
   inline constexpr const char* PINK        = "\033[038;002;255;192;203m";
   inline constexpr const char* TEAL        = "\033[038;002;000;128;128m";
   inline constexpr const char* GOLD        = "\033[038;002;255;215;000m";
   inline constexpr const char* SILVER      = "\033[038;002;192;192;192m";
   inline constexpr const char* MAROON      = "\033[038;002;128;000;000m";
   inline constexpr const char* NAVY        = "\033[038;002;000;000;128m";
   inline constexpr const char* OLIVE       = "\033[038;002;128;128;000m";
   inline constexpr const char* LIME        = "\033[038;002;000;255;000m";
   inline constexpr const char* GRAY        = "\033[038;002;128;128;128m";
   inline constexpr const char* CRIMSON     = "\033[038;002;220;020;060m";
   inline constexpr const char* DARKGREEN   = "\033[038;002;000;100;000m";
   inline constexpr const char* DARKBLUE    = "\033[038;002;000;000;139m";
   inline constexpr const char* DARKCYAN    = "\033[038;002;000;139;139m";
   inline constexpr const char* DARKGRAY    = "\033[038;002;169;169;169m";
   inline constexpr const char* DARKMAGENTA = "\033[038;002;139;000;139m";
   inline constexpr const char* DARKORANGE  = "\033[038;002;255;140;000m";
   inline constexpr const char* NVIMGREEN   = "\033[038;002;195;232;141m";
   inline constexpr const char* NVIMBLUE    = "\033[038;002;130;170;255m";
   inline constexpr const char* NVIMPURPLE  = "\033[038;002;192;153;255m";
   inline constexpr const char* NVIMDARK    = "\033[038;002;034;036;054m";
   static inline const char* COLOR(const uint8_t& R, const uint8_t& G, const uint8_t& B);
   static inline const char* TOCOLR(const char* BG);
}


namespace BgColor{
   inline constexpr const char* RESET       = "\033[000;000;000;000;000m";
   inline constexpr const char* RED         = "\033[048;002;255;000;000m";
   inline constexpr const char* GREEN       = "\033[048;002;000;255;000m";
   inline constexpr const char* BLUE        = "\033[048;002;000;000;255m";
   inline constexpr const char* YELLOW      = "\033[048;002;255;255;000m";
   inline constexpr const char* CYAN        = "\033[048;002;000;255;255m";
   inline constexpr const char* MAGENTA     = "\033[048;002;255;000;255m";
   inline constexpr const char* WHITE       = "\033[048;002;255;255;255m";
   inline constexpr const char* BLACK       = "\033[048;002;000;000;000m";
   inline constexpr const char* ORANGE      = "\033[048;002;255;165;000m";
   inline constexpr const char* PURPLE      = "\033[048;002;128;000;128m";
   inline constexpr const char* BROWN       = "\033[048;002;165;042;042m";
   inline constexpr const char* PINK        = "\033[048;002;255;192;203m";
   inline constexpr const char* TEAL        = "\033[048;002;000;128;128m";
   inline constexpr const char* GOLD        = "\033[048;002;255;215;000m";
   inline constexpr const char* SILVER      = "\033[048;002;192;192;192m";
   inline constexpr const char* MAROON      = "\033[048;002;128;000;000m";
   inline constexpr const char* NAVY        = "\033[048;002;000;000;128m";
   inline constexpr const char* OLIVE       = "\033[048;002;128;128;000m";
   inline constexpr const char* LIME        = "\033[048;002;000;255;000m";
   inline constexpr const char* GRAY        = "\033[048;002;128;128;128m";
   inline constexpr const char* CRIMSON     = "\033[048;002;220;020;060m";
   inline constexpr const char* DARKGREEN   = "\033[048;002;000;100;000m";
   inline constexpr const char* DARKBLUE    = "\033[048;002;000;000;139m";
   inline constexpr const char* DARKCYAN    = "\033[048;002;000;139;139m";
   inline constexpr const char* DARKGRAY    = "\033[048;002;169;169;169m";
   inline constexpr const char* DARKMAGENTA = "\033[048;002;139;000;139m";
   inline constexpr const char* DARKORANGE  = "\033[048;002;255;140;000m";
   inline constexpr const char* NVIMGREEN   = "\033[048;002;195;232;141m";
   inline constexpr const char* NVIMBLUE    = "\033[048;002;130;170;255m";
   inline constexpr const char* NVIMPURPLE  = "\033[048;002;192;153;255m";
   inline constexpr const char* NVIMDARK    = "\033[048;002;034;036;054m";
   static inline const char* COLOR(const uint8_t& R, const uint8_t& G, const uint8_t& B);
   static inline const char* TOCOLR(const char* CL);
}

/**
 * @namespace Font
 * @brief ANSI escape sequences for text formatting and typography styles.
 */
namespace Font{
   inline constexpr const char* RESET       = "\033[00m"; /**< Clears all visual formatting matrices. */
   inline constexpr const char* BOLD        = "\033[01m"; /**< Applies heavy stroke weight. */
   inline constexpr const char* ITALIC      = "\033[03m"; /**< Applies italicized slant. */
   inline constexpr const char* UNDERLINE   = "\033[04m"; /**< Draws an underscore line beneath the text. */
   inline constexpr const char* BLINK       = "\033[05m"; /**< Triggers standard terminal blink animation. */
   inline constexpr const char* REVERSE     = "\033[07m"; /**< Inverts the foreground and background colors. */
   inline constexpr const char* HIDE        = "\033[08m"; /**< Renders text invisible while maintaining layout spacing. */
   inline constexpr const char* STRIKED     = "\033[09m"; /**< Draws a line directly through the text bounds. */
   inline constexpr const char* FONT_A      = "\033[10m"; /**< Switches to terminal alternative font profile A. */
   inline constexpr const char* FONT_B      = "\033[11m"; /**< Switches to terminal alternative font profile B. */
   inline constexpr const char* FONT_C      = "\033[12m"; /**< Switches to terminal alternative font profile C. */
   inline constexpr const char* FONT_D      = "\033[13m"; /**< Switches to terminal alternative font profile D. */
}

/**
 * @namespace Cursor
 * @brief ANSI escape sequences and hardware math methods for manipulating the terminal cursor.
 */
namespace Cursor{
   inline constexpr uint8_t BUF = 0x10; /**< Standard buffer allocation size for cursor string operations. */
   inline constexpr const char* GETSIZE    = "\033[s\033[65354;65354H\033[6n\033[u"; /**< Triggers blind CPR to force the terminal to report its max dimensions. */
   inline constexpr const char* SAVEPOS    = "\033[s"; /**< Saves the cursor's current X/Y coordinate in the terminal's memory. */
   inline constexpr const char* RESTOREPOS = "\033[u"; /**< Snaps the cursor back to the last saved X/Y coordinate. */
   inline constexpr const char* HIDE       = "\033[?25l"; /**< Temporarily vanishes the cursor block to hide redraw stutters. */
   inline constexpr const char* SHOW       = "\033[?25h"; /**< Restores the visible cursor block. */
   
   /**
    * @brief Core ANSI constructor utilizing a thread-safe circular buffer.
    * @param dir The ANSI directional character command.
    * @param N The mathematical offset.
    * @paragraph Zero-Allocation Engine
    * Replaces the flawed static array which broke when multiple cursor macros were injected into 
    * a single printf statement. Cycles through 4 isolated SRAM buffers on each call, ensuring complete memory safety 
    * while simultaneously replacing heavy `snprintf` calls with a raw base-10 calculation.
    * @return A C-string containing the constructed escape sequence.
    */
   static inline const char* FORMAT_CURSOR(char dir, const uint16_t N);

   /**
    * @brief Injects an ANSI code to shift the cursor left.
    * @param N Number of columns to traverse.
    * @paragraph Operation execution
    * Defers calculation directly to the optimized `FORMAT_CURSOR` circular buffer sequence.
    * @return A safe C-string payload ready for hardware serial transmission.
    */
   static inline const char* LEFT(const uint16_t N);

   /**
    * @brief Injects an ANSI code to shift the cursor right.
    * @param N Number of columns to traverse.
    * @paragraph Operation execution
    * Defers calculation directly to the optimized `FORMAT_CURSOR` circular buffer sequence.
    * @return A safe C-string payload ready for hardware serial transmission.
    */
   static inline const char* RIGHT(const uint16_t N);

   /**
    * @brief Injects an ANSI code to shift the cursor upwards.
    * @param N Number of rows to traverse.
    * @paragraph Operation execution
    * Defers calculation directly to the optimized `FORMAT_CURSOR` circular buffer sequence.
    * @return A safe C-string payload ready for hardware serial transmission.
    */
   static inline const char* UP(const uint16_t N);

   /**
    * @brief Injects an ANSI code to shift the cursor downwards.
    * @param N Number of rows to traverse.
    * @paragraph Operation execution
    * Defers calculation directly to the optimized `FORMAT_CURSOR` circular buffer sequence.
    * @return A safe C-string payload ready for hardware serial transmission.
    */
   static inline const char* DOWN(const uint16_t N);

   /**
    * @brief Snaps the cursor to an absolute target coordinate on the terminal grid.
    * @param X Target column index.
    * @param Y Target row index.
    * @paragraph Matrix Override
    * Completely bypasses relative movements. Utilizes a widened circular buffer to support the 
    * dual-variable base-10 math operations required for absolute terminal coordinate parsing.
    * @return A safe C-string payload ready for hardware serial transmission.
    */
   static inline const char* TOXY(const uint16_t X, const uint16_t Y);

   /**
    * @brief Parses an incoming terminal Cursor Position Report string to extract screen boundaries.
    * @param str Raw ANSI string response caught from the terminal interface.
    * @paragraph Fast Extraction
    * Strips away standard C-string parsing libraries (like `sscanf`) and manually steps through 
    * the char pointer memory to rapidly calculate the window size in extreme low-latency environments.
    * @return A pair of uint16_t variables representing terminal Width and Height.
    */
   static inline std::pair<uint16_t, uint16_t> PARSERSIZE(const char* str);
}

/**
 * @namespace Clear
 * @brief Hardware ANSI sequences designed to rapidly purge text data from the terminal.
 */
namespace Clear{
   inline constexpr const char* EOS = "\033[0J"; /**< Purges all characters from the cursor down to the end of the terminal screen. */
   inline constexpr const char* COL = "\033[1J"; /**< Purges all characters from the cursor upwards to the top of the terminal screen. */
   inline constexpr const char* ALL = "\033[2J"; /**< Vaporizes all characters on the terminal and completely blanks the screen. */
   inline constexpr const char* EOL = "\033[0K"; /**< Sweeps text from the cursor to the far right end of the current line. */
   inline constexpr const char* SOL = "\033[1K"; /**< Sweeps text from the cursor to the far left start of the current line. */
   inline constexpr const char* LNE = "\033[2K"; /**< Completely obliterates the entire current line regardless of cursor placement. */
}

/* IMPLEMENTATIONS - NOTHING HERE */

inline const char* Color::COLOR(const uint8_t& R, const uint8_t& G, const uint8_t& B){
   return  "\033[038;002;" + ('0' + R / 100) + ('0' + (R / 10) % 10) + ('0' + R % 10) + ';' + ('0' + G / 100) + ('0' + (G / 10) % 10) + ('0' + G % 10) + ';' + ('0' + B / 100) + ('0' + (B / 10) % 10) + ('0' + B % 10) + 'm';
}

inline const char* Color::TOCOLR(const char* BG){
   char* __BGDUP = strdup(BG);
   __BGDUP[3] = '3';
   return __BGDUP;
}

inline const char* BgColor::COLOR(const uint8_t& R, const uint8_t& G, const uint8_t& B){
   return  "\033[048;002;" + ('0' + R / 100) + ('0' + (R / 10) % 10) + ('0' + R % 10) + ';' + ('0' + G / 100) + ('0' + (G / 10) % 10) + ('0' + G % 10) + ';' + ('0' + B / 100) + ('0' + (B / 10) % 10) + ('0' + B % 10) + 'm';
}

inline const char* BgColor::TOCOLR(const char* CL){
   char* __CLDUP = strdup(CL);
   __CLDUP[3] = '4';
   return __CLDUP;
}

inline const char* Cursor::FORMAT_CURSOR(char dir, const uint16_t N) {
   static char bufs[4][BUF];
   static uint8_t idx = 0;
   char* p = bufs[idx];
   idx = (idx + 1) & 3;
   *p++ = '\033'; *p++ = '[';
   uint16_t v = N;
   char pad = 0, c = '0';
   while (v >= 10000) { v -= 10000; c++; }
   if (c > '0') { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 1000) { v -= 1000; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 100) { v -= 100; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 10) { v -= 10; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   *p++ = '0' + (char)v;
   
   *p++ = dir; *p = '\0';
   return bufs[(idx - 1) & 3];
}

inline const char* Cursor::LEFT(const uint16_t N) { 
   return FORMAT_CURSOR('D', N); 
}

inline const char* Cursor::RIGHT(const uint16_t N) {
   return FORMAT_CURSOR('C', N);
}

inline const char* Cursor::UP(const uint16_t N) {
   return FORMAT_CURSOR('A', N);
}

inline const char* Cursor::DOWN(const uint16_t N) {
   return FORMAT_CURSOR('B', N);
}

inline const char* Cursor::TOXY(const uint16_t X, const uint16_t Y){
   static char bufs[4][BUF + 8];
   static uint8_t idx = 0;
   char* p = bufs[idx];
   idx = (idx + 1) & 3;
   *p++ = '\033'; *p++ = '[';
   uint16_t v = Y;
   char pad = 0, c = '0';
   while (v >= 10000) { v -= 10000; c++; }
   if (c > '0') { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 1000) { v -= 1000; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 100) { v -= 100; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 10) { v -= 10; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   *p++ = '0' + (char)v;
   *p++ = ';';
   v = X; pad = 0; c = '0';
   while (v >= 10000) { v -= 10000; c++; }
   if (c > '0') { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 1000) { v -= 1000; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 100) { v -= 100; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   c = '0';
   while (v >= 10) { v -= 10; c++; }
   if (c > '0' || pad) { *p++ = c; pad = 1; }
   *p++ = '0' + (char)v;
   *p++ = 'H'; *p = '\0';
   return bufs[(idx - 1) & 3];
}

inline std::pair<uint16_t, uint16_t> Cursor::PARSERSIZE(const char* str){
   uint16_t x = 0;
   uint16_t y = 0;
   const char* i = str + 2;
   for (; *i != 59; i++){
      x *= 10;
      x += *i - 48;
   }
   if (*i == 59) i++;
   for (; *i != 82; i++){
      y *= 10;
      y += *i - 48;
   }
   return std::make_pair(y ,x);
}

#endif