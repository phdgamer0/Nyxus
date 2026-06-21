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

#ifndef _NYXUS_AUDIO_DSP_HPP_
#define _NYXUS_AUDIO_DSP_HPP_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <complex>
#include <vector>

namespace Convert{
   
   // --- Resolution & Type --- 

   /** @brief Converts 16-bit integer audio to 32-bit floating point (-1.0f to 1.0f) for high-precision math. */
   inline void ToFloat32(const int16_t* __restrict in, float* __restrict out, size_t length);

   /** @brief Converts 32-bit floating point back to 16-bit integer, applying hard-clipping to prevent overflow. */
   inline void ToInt16(const float* __restrict in, int16_t* __restrict out, size_t length);

   /** @brief Truncates 16-bit audio down to 8-bit signed audio (lo-fi effect or saving memory). */
   inline void To8BitSigned(const int16_t* __restrict in, int8_t* __restrict out, size_t length);

   /** @brief Converts 16-bit signed audio to 8-bit unsigned (often required for cheap PWM speakers). */
   inline void To8BitUnsigned(const int16_t* __restrict in, uint8_t* __restrict out, size_t length);

   /** @brief Extracts 24-bit audio (3 bytes per sample) from high-res WAVs and packs it into standard 16-bit ints. */
   inline void From24Bit(const uint8_t* __restrict in24, int16_t* __restrict out16, size_t frames);

   // --- Memory Layout ---

   /** @brief Splits an interleaved stereo buffer (LRLR) into two separate, isolated arrays (LLLL, RRRR). */
   inline void Deinterleave(const int16_t* __restrict interleaved_in, int16_t* __restrict left_out, int16_t* __restrict right_out, size_t frames);

   /** @brief Combines two isolated arrays (LLLL, RRRR) back into an interleaved format (LRLR) for I2S transmission. */
   inline void Interleave(const int16_t* __restrict left_in, const int16_t* __restrict right_in, int16_t* __restrict interleaved_out, size_t frames);

   // --- Hardware & Phase ---

   /** @brief Flips the phase of the audio (multiplies by -1). Used to fix wiring issues or cancel noise. */
   inline void InvertPhase(int16_t* __restrict buffer, size_t length);

   /** @brief Swaps the byte-endianness (Little Endian to Big Endian). Crucial if reading network audio streams. */
   inline void SwapEndian16(int16_t* __restrict buffer, size_t length);

   /** @brief Shifts unsigned audio (0 to 65535) into signed audio (-32768 to 32767). */
   inline void UnsignedToSigned16(const uint16_t* __restrict in, int16_t* __restrict out, size_t length);

   /** @brief Shifts signed audio (-32768 to 32767) into unsigned audio (0 to 65535). */
   inline void SignedToUnsigned16(const int16_t* __restrict in, uint16_t* __restrict out, size_t length);

   // --- Mid/Side Processing ---

   /** @brief Converts standard L/R stereo into Mid/Side stereo format (Mid = L+R, Side = L-R). */
   inline void EncodeMidSide(const int16_t* __restrict lr_in, int16_t* __restrict ms_out, size_t frames);

   /** @brief Converts Mid/Side stereo back into standard L/R stereo for playback. */
   inline void DecodeMidSide(const int16_t* __restrict ms_in, int16_t* __restrict lr_out, size_t frames);

   // --- Sample Rate Conversion ---

   /** @brief Zero-Order Hold (Nearest Neighbor) Resampling. Fast and computationally cheap, but introduces aliasing artifacts. Best for low-fi effects. */
   inline void ResampleNearest(const int16_t* __restrict in, int16_t* __restrict out, size_t in_frames, size_t out_frames);

   /** @brief First-Order Interpolation (Linear) Resampling. Calculates the mathematical midpoint between samples. Standard for general audio stretching. */
   inline void ResampleLinear(const int16_t* __restrict in, int16_t* __restrict out, size_t in_frames, size_t out_frames);

   // --- Signal Conditioning ---

   /** @brief Scans a static buffer for its average mean and subtracts it to center the waveform at 0. Essential before applying distortion or FFT analysis. */
   inline void RemoveDCOffset(int16_t* __restrict buffer, size_t length);

   /** @brief Scans a Real-Time buffer for its average mean and subtracts it to center the waveform at 0. Essential before applying distortion or FFT analysis. */
   inline void RemoveDCOffsetRT(int16_t* __restrict buffer, size_t length, int16_t& prev_x, float& prev_y);

   /** @brief Finds the highest peak in the buffer and scales the entire array so the peak hits the target maximum. Target peak is typically 1.0f (0dBFS). */
   inline void NormalizePeak(int16_t* __restrict buffer, size_t length, float target_peak = 1.0f);

   // --- Dithering ---

   /** @brief Reduces 16-bit to 8-bit while applying Triangular Probability Density Function (TPDF) dither. Requires a pseudo-random number generator (PRNG) state to continuously generate the noise floor. */
   inline void To8BitDithered(const int16_t* __restrict in, int8_t* __restrict out, size_t length, uint32_t& prng_state);

   // --- Companding (Data Compression) ---

   /** @brief Compresses 16-bit linear PCM into 8-bit mu-law (Standard for North American/Japanese telephony). */
   inline void PcmToMuLaw(const int16_t* __restrict pcm_in, uint8_t* __restrict mulaw_out, size_t length);

   /** @brief Expands 8-bit mu-law back into 16-bit linear PCM for playback. */
   inline void MuLawToPcm(const uint8_t* __restrict mulaw_in, int16_t* __restrict pcm_out, size_t length);

   /** @brief Compresses 16-bit linear PCM into 8-bit A-law (Standard for European telephony). */
   inline void PcmToALaw(const int16_t* __restrict pcm_in, uint8_t* __restrict alaw_out, size_t length);

   /** @brief Expands 8-bit A-law back into 16-bit linear PCM for playback. */
   inline void ALawToPcm(const uint8_t* __restrict alaw_in, int16_t* __restrict pcm_out, size_t length);

   // --- Buffer Formatting ---

   /** @brief Fills the remainder of a buffer with silence (0) if the SD card read returns a partial chunk. Prevents playing randomized garbage memory at the end of a file. */
   inline void ZeroPad(int16_t* __restrict buffer, size_t current_length, size_t target_length);

   /** @brief Applies a linear amplitude ramp from 0 to 1 at the start, and 1 to 0 at the end. Prevents speaker "popping" when starting or stopping playback abruptly. */
   inline void ApplyFadeInOut(int16_t* __restrict buffer, size_t length, size_t fade_frames);
}

namespace Effects {

   // --- State Structures ---

   /** @brief Standard circular buffer for time-based effects. */
   template <size_t MAX_DELAY_SAMPLES>
   struct DelayState {
      std::array<int16_t, MAX_DELAY_SAMPLES> ring_buffer = {0};
      uint32_t write_index = 0;
      constexpr size_t size() const { return MAX_DELAY_SAMPLES; }
   };

   /** @brief Generates sweeping math to automate pedals (Chorus, Flanger, Vibrato, Wah). */
   struct LFOState {
      float phase = 0.0f;
      float phase_increment = 0.0f; // Calculated based on frequency and sample rate
   };

   /** @brief The industry standard infinite-impulse-response (IIR) filter memory. */
   struct BiquadState {
      float x1 = 0.0f, x2 = 0.0f; // Past inputs
      float y1 = 0.0f, y2 = 0.0f; // Past outputs
   };

   /** @brief Biquad coefficient matrix (calculated once when you change EQ knobs). */
   struct BiquadCoeffs {
      float b0, b1, b2, a1, a2;
   };

   // --- LFO Generator ---

   /** @brief Advances the LFO and returns a value between -1.0 and 1.0. */
   [[nodiscard]] inline float ProcessLFO(LFOState& lfo);

   namespace Amplitude{
      // --- Linear Processing ---
      /** @brief Applies standard linear volume scaling. */
      [[nodiscard]] inline float ApplyVolume(float input, float multiplier);

      /** @brief Mutes the signal completely if it falls below a specific threshold (Noise Gate). */
      [[nodiscard]] inline float NoiseGate(float input, float threshold);

      // --- Non-Linear Saturation (Distortion) ---

      /** @brief Hard digital clipping. Creates harsh, squared-off harmonics (Heavy Metal). */
      [[nodiscard]] inline float HardClip(float input, float threshold);

      /** @brief Soft analog clipping using hyperbolic tangent (Tube Overdrive). */
      [[nodiscard]] inline float SoftClipTanh(float input, float drive);

      /** @brief Polynomial soft clipping. Faster than tanh() but slightly less smooth. */
      [[nodiscard]] inline float SoftClipPoly(float input, float drive);

      /** @brief Asymmetric clipping. Compresses the positive wave harder than the negative (Vintage Fuzz). */
      [[nodiscard]] inline float AsymmetricFuzz(float input, float drive, float bias);

      // --- Lo-Fi Processing ---

      /** @brief Reduces the vertical resolution of the waveform (Bitcrushing). */
      [[nodiscard]] inline float BitCrush(float input, float steps);

      // --- Block Processing Overloads ---

      /** @brief Applies volume scaling across an entire block of audio samples. */
      inline void ApplyVolumeBlock(float* __restrict buffer, size_t length, float multiplier);

      /** @brief Silences all samples in a block that fall below the specified amplitude threshold. */
      inline void NoiseGateBlock(float* __restrict buffer, size_t length, float threshold);

      /** @brief Applies hard digital clipping to a block of samples, clamping them to the threshold. */
      inline void HardClipBlock(float* __restrict buffer, size_t length, float threshold);

      /** @brief Applies smooth analog-style tanh saturation across a block of samples. */
      inline void SoftClipTanhBlock(float* __restrict buffer, size_t length, float drive);

      /** @brief Applies fast polynomial soft clipping to an entire block of audio. */
      inline void SoftClipPolyBlock(float* __restrict buffer, size_t length, float drive);

      /** @brief Applies asymmetric overdrive/fuzz to a block of samples for vintage saturation. */
      inline void AsymmetricFuzzBlock(float* __restrict buffer, size_t length, float drive, float bias);

      /** @brief Reduces the resolution of a block of samples to create a lo-fi bitcrushed effect. */
      inline void BitCrushBlock(float* __restrict buffer, size_t length, float steps);
   }

   namespace Time{
      // --- Delay Line Operations ---

      /** @brief Writes a sample to the circular buffer and advances the head. */
      template <size_t N>
      inline void WriteDelay(DelayState<N>& state, float input);

      /** @brief Reads a sample from the buffer using an exact integer offset (Standard Echo). */
      template <size_t N>
      [[nodiscard]] inline float ReadDelayInteger(const DelayState<N>& state, uint32_t delay_samples);

      /** @brief Reads a sample using fractional offsets via Linear Interpolation (Required for Chorus/Flanger). */
      template <size_t N>
      [[nodiscard]] inline float ReadDelayInterpolated(const DelayState<N>& state, float delay_samples);

      // --- Reverb Building Blocks ---

      /** @brief Comb Filter. Delays the signal and feeds it back into itself (creates metallic ringing/reverb tails). */
      template <size_t N>
      [[nodiscard]] inline float CombFilter(DelayState<N>& state, float input, float delay_samples, float feedback);

      /** @brief All-Pass Filter. Shifts the phase of the signal without altering volume (creates Reverb diffusion). */
      template <size_t N>
      [[nodiscard]] inline float AllPassFilter(DelayState<N>& state, float input, float delay_samples, float gain);

      /** @brief Hermite Interpolation. The standard for Pitch Shifting and pristine Chorus. */
      template <size_t N>
      [[nodiscard]] inline float ReadDelayHermite(const DelayState<N>& state, float delay_samples);

   }

   namespace Filter{
      // --- The Universal Audio Atom ---

      /** @brief Processes a single sample through an Infinite Impulse Response (IIR) filter. */
      [[nodiscard]] inline float ProcessBiquad(float input, BiquadState& state, const BiquadCoeffs& coeffs);

      // --- Coefficient Calculators (Run these ONLY when a knob is turned) ---

      /** @brief Calculates coefficients for a Lowpass Filter (cuts high frequencies). */
      inline void CalcLowpass(BiquadCoeffs& coeffs, float sample_rate, float cutoff_freq, float q_factor);

      /** @brief Calculates coefficients for a Highpass Filter (cuts low frequencies). */
      inline void CalcHighpass(BiquadCoeffs& coeffs, float sample_rate, float cutoff_freq, float q_factor);

      /** @brief Calculates coefficients for a Bandpass Filter (isolates a specific frequency, used for Wah pedals). */
      inline void CalcBandpass(BiquadCoeffs& coeffs, float sample_rate, float center_freq, float q_factor);

      // --- Shelving EQ Coefficients (Bass / Treble Knobs) ---

      /** @brief Low Shelf (Bass Knob). Boosts or cuts frequencies below the cutoff. */
      inline void CalcLowShelf(BiquadCoeffs& coeffs, float sample_rate, float cutoff_freq, float gain_db);

      // --- Phase Shifting ---

      /** @brief All-Pass Filter. Changes the phase of frequencies without changing volume. Chain 4 or 8 of these together and sweep them with an LFO to create a Phaser pedal. */
      inline void CalcAllPass(BiquadCoeffs& coeffs, float sample_rate, float center_freq, float q_factor);

      // --- The Universal Audio Atom ---

      /** @brief Processes a single sample through an Infinite Impulse Response (IIR) filter. */
      [[nodiscard]] inline float ProcessBiquad(float input, BiquadState& state, const BiquadCoeffs& coeffs);

      /** @brief Processes a block of samples through a Biquad filter, heavily optimized for CPU SIMD pipelining. */
      inline void ProcessBiquadBlock(float* __restrict buffer, size_t length, BiquadState& state, const BiquadCoeffs& coeffs);
   }

   namespace Pitch{

      /** @brief Generates an LFO value between -1.0 and 1.0. */
      [[nodiscard]] inline float StepLFO(LFOState& lfo);

      /** @brief Vibrato (Pitch Bending). Modulates the read head of a delay line using an LFO. */
      template <size_t N>
      [[nodiscard]] inline float ProcessVibrato(float input, DelayState<N>& delay, LFOState& lfo, float depth_samples);

      /** @brief Chorus. Mixes the dry signal 50/50 with a Vibrato signal. */
      template <size_t N>
      [[nodiscard]] inline float ProcessChorus(float input, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay);

      /** @brief Flanger. Similar to chorus, but with extremely short delay times and heavy feedback. */
      template <size_t N>
      [[nodiscard]] inline float ProcessFlanger(float input, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay, float feedback);

      // --- Block Processing Overloads ---

      /** @brief Applies LFO-driven pitch vibrato across an entire block of audio. */
      template <size_t N>
      inline void ProcessVibratoBlock(float* __restrict buffer, size_t length, DelayState<N>& delay, LFOState& lfo, float depth_samples);

      /** @brief Processes a block of samples through a chorus effect using an interpolated delay line. */
      template <size_t N>
      inline void ProcessChorusBlock(float* __restrict buffer, size_t length, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay);

      /** @brief Processes a block of samples through a flanger effect with heavy feedback. */
      template <size_t N>
      inline void ProcessFlangerBlock(float* __restrict buffer, size_t length, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay, float feedback);
   }

}

namespace FFT {

   // --- State Memory ---

   /** @brief Holds pre-computed twiddle factors and bit-reversal indices for zero-allocation FFT execution. */
   template <size_t FFT_SIZE>
   struct FFTState {
      std::array<std::complex<float>, FFT_SIZE / 2> twiddles;
      std::array<uint32_t, FFT_SIZE> bit_reversed_indices;
   };

   // --- Initialization ---

   /** @brief Pre-calculates the trigonometric tables and bit-reversal map for a specific power-of-2 FFT size. */
   template <size_t N>
   inline void Initialize(FFTState<N>& state, size_t fft_size);

   // --- Pre-Processing (Windowing) ---

   /** @brief Applies a Hann window to the time-domain buffer to prevent spectral leakage (audio edge-clicking) before FFT analysis. */
   inline void ApplyHannWindow(float* __restrict buffer, size_t length);

   // --- Core Execution ---

   /** @brief Executes an in-place, Radix-2 Forward Fast Fourier Transform (Time Domain -> Frequency Domain). */
   template <size_t N>
   inline void Forward(std::complex<float>* __restrict buffer, const FFTState<N>& state, size_t fft_size);

   /** @brief Executes an in-place, Radix-2 Inverse Fast Fourier Transform (Frequency Domain -> Time Domain). */
   template <size_t N>
   inline void Inverse(std::complex<float>* __restrict buffer, const FFTState<N>& state, size_t fft_size);

   // --- Post-Processing (Analysis) ---

   /** @brief Atom: Calculates the absolute amplitude (magnitude) of a single complex frequency bin. */
   [[nodiscard]] inline float GetMagnitude(const std::complex<float>& bin);

   /** @brief Converts an array of complex frequency bins into an array of real magnitudes for spectrum visualizers. */
   inline void CalculateMagnitudes(const std::complex<float>* __restrict fft_buffer, float* __restrict mag_buffer, size_t half_fft_size);

}

namespace Spatial {

   // --- 3D Positioning ---

   /** @brief Calculates the Left/Right volume distribution based on an angle (-90 to +90 degrees) using Constant-Power panning. */
   inline void CalcStereoPan(float angle_degrees, float& out_left_gain, float& out_right_gain);

   /** @brief Calculates volume dropoff based on the Inverse Square Law for 3D spatial distance. */
   [[nodiscard]] inline float CalcDistanceAttenuation(float distance, float min_distance, float max_distance, float rolloff_factor);

   /** @brief Applies a Doppler pitch-shift coefficient based on the relative velocity between the listener and the audio source. */
   [[nodiscard]] inline float CalcDopplerPitch(float source_velocity, float listener_velocity, float speed_of_sound = 343.0f);

   // --- Execution ---

   /** @brief Takes a Mono sample and writes it to a Stereo buffer (Left and Right) using calculated Spatial gains. */
   inline void Apply3DPanning(float mono_in, float& stereo_left_out, float& stereo_right_out, float left_gain, float right_gain);

   /** @brief Mixes a Mono sample into a Stereo buffer (Left and Right) using calculated Spatial gains. */
   inline void Mix3DPanning(float mono_in, float& stereo_left_out, float& stereo_right_out, float left_gain, float right_gain);

   // --- Psychoacoustics (Head Simulation) ---

   /** @brief Calculates Interaural Time Difference (ITD) using Woodworth's formula. 
    * Calculates the exact microsecond delay between the left and right ear based on the angle and head radius. 
    */
   inline void CalcITD(float angle_radians, float head_radius_meters, float speed_of_sound, float& out_left_delay_sec, float& out_right_delay_sec);

   /** @brief Calculates Head Shadowing (ILD). 
    * Returns the Lowpass Filter cutoff frequencies for each ear. The skull physically blocks high frequencies from reaching the far ear.
    */
   inline void CalcHeadShadowCutoffs(float angle_radians, float max_cutoff_hz, float min_cutoff_hz, float& out_left_cutoff, float& out_right_cutoff);

   // --- Advanced Distance Attenuation ---

   /** @brief Logarithmic Distance Rolloff. 
    * The AAA game industry standard. Volume drops exponentially as it moves away, mirroring real-world acoustics much better than linear math. 
    */
   [[nodiscard]] inline float CalcLogarithmicAttenuation(float distance, float min_distance, float max_distance);

   // --- Surround / Multi-Speaker Routing ---

   /** @brief Quadraphonic (4-Channel) Vector Panning. 
    * Routes a mono signal into Front-Left, Front-Right, Rear-Left, and Rear-Right speakers based on a 360-degree azimuth angle.
    */
   inline void CalcQuadraphonicPan(float azimuth_radians, float& out_fl, float& out_fr, float& out_rl, float& out_rr);

}

namespace Generators {

   // --- State Memory ---

   /** @brief Memory for cyclical waveforms. Tracks the exact playback position (phase). */
   struct OscillatorState {
      float phase = 0.0f;
      float phase_increment = 0.0f;
   };

   // --- Frequency Control ---

   /** * @brief Translates a human-readable frequency (Hz) into a mathematical phase increment.
    * Call this whenever a synth key is pressed, or run it continuously if doing FM/Pitch-Bending.
    */
   inline void SetFrequency(OscillatorState& state, float frequency_hz, float sample_rate);

   // --- Waveform Oscillators ---

   /** @brief Pure Sine wave. Smooth, contains only the fundamental frequency. */
   [[nodiscard]] inline float SineOsc(OscillatorState& state);

   /** @brief Square wave. Contains only odd harmonics. Classic Nintendo/Chiptune sound. */
   [[nodiscard]] inline float SquareOsc(OscillatorState& state);

   /** @brief Sawtooth wave. Contains both even and odd harmonics. The richest wave for Subtractive Synthesis. */
   [[nodiscard]] inline float SawOsc(OscillatorState& state);

   /** @brief Triangle wave. Softer than a square, contains odd harmonics that roll off very quickly. */
   [[nodiscard]] inline float TriangleOsc(OscillatorState& state);

   // --- Noise Generators ---

   /** * @brief White Noise. Equal energy across all frequencies. 
    * Uses a high-speed XOR-shift PRNG to avoid the heavy CPU cost of standard rand(). 
    */
   [[nodiscard]] inline float WhiteNoise(uint32_t& prng_state);

   /** * @brief Pink Noise (-3dB/octave). Equal energy per octave. 
    * Simulates the human ear's natural hearing curve. Uses a 1-pole lowpass filter approximation.
    */
   [[nodiscard]] inline float PinkNoise(uint32_t& prng_state, float& filter_state);

   /** * @brief Brownian / Brown Noise (-6dB/octave). Heavy low-end rumble.
    * Created using a leaky integrator (to prevent DC offset clipping) on White Noise.
    */
   [[nodiscard]] inline float BrownNoise(uint32_t& prng_state, float& last_out);

}

namespace Convolution {

   // --- State Memory ---

   /** @brief Compile-time bounded memory for Direct Time-Domain FIR Convolution. */
   template <size_t IR_LENGTH>
   struct DirectFIRState {
      std::array<float, IR_LENGTH> ir_buffer = {0.0f};
      std::array<float, IR_LENGTH> history_buffer = {0.0f};
      uint32_t head = 0;
      constexpr size_t size() const { return IR_LENGTH; }
   };

   /** @brief Flattened, Zero-Latency Partitioned Convolution memory. */
   template <size_t PARTITION_SIZE, size_t PARTITION_COUNT>
   struct PartitionedConvState {
      static constexpr size_t FFT_SIZE = PARTITION_SIZE * 2;
      FFT::FFTState<FFT_SIZE> fft_state;
      std::array<std::complex<float>, PARTITION_COUNT * FFT_SIZE> ir_partitions_fft;
      std::array<std::complex<float>, PARTITION_COUNT * FFT_SIZE> audio_history_fft = {};
      std::array<std::complex<float>, FFT_SIZE> staging_buffer = {};
      std::array<std::complex<float>, FFT_SIZE> accumulator_buffer = {};
      std::array<float, PARTITION_SIZE> overlap_buffer = {0.0f};
      size_t current_partition = 0;
   };

   // --- IR Preparation & Sanitization ---

   /** @brief Trims absolute silence from the start of an Impulse Response loaded from the SD card. Essential for preventing artificial latency caused by poorly edited third-party IR files. */
   inline void TrimIRSilence(std::vector<float>& ir_buffer, float silence_threshold = 0.0001f);

   /** @brief Normalizes the total energy of the Impulse Response to 1.0 (0dBFS). Prevents the Convolution engine from drastically blowing out the volume or burying the signal. */
   inline void NormalizeIR(std::vector<float>& ir_buffer);

   /** @brief Applies a Half-Hann fade-out to the tail of the IR. Prevents unnatural clicking or buzzing when the IR array abruptly ends. */
   inline void ApplyIRTaper(std::vector<float>& ir_buffer, size_t taper_length_samples);

   // --- Direct FIR Convolution (Zero Latency, High CPU) ---

   /** @brief Initializes a Direct FIR filter. Best used for short EQ-matching IRs. */
   template <size_t N>
   inline void InitDirectFIR(DirectFIRState<N>& state, const float* __restrict ir_data, size_t ir_length);

   /** @brief Executes Direct Time-Domain Convolution for a single sample. Computationally heavy for long Reverbs, but mathematically instantaneous. */
   template <size_t N>
   [[nodiscard]] inline float ProcessDirectFIRSample(float input, DirectFIRState<N>& state);

   // --- Partitioned Convolution (Low Latency, Low CPU) ---

   /** 
    * @brief Slices a long IR (e.g., 2048 samples) into smaller uniform chunks (e.g., 64 samples) and calculates the FFT for each slice.
    * @note Run this once when loading a new Cabinet IR from the SD card.
    */
   template <size_t N, size_t C>
   inline void InitPartitionedConv(PartitionedConvState<N, C>& state, const float* __restrict ir_data, size_t ir_length, size_t partition_size);

   /**
    * @brief Executes Overlap-Save Partitioned Convolution on an incoming block of audio.
    * @paragraph Execution
    * Takes a block of audio equal to `partition_size`, runs a single FFT, multiplies it across the 
    * historical IR partitions, runs an IFFT, and returns the mathematically perfect Reverb/Cab-Sim tail.
    */
   template <size_t N, size_t C>
   inline void ProcessPartitionedBlock(float* __restrict io_buffer, PartitionedConvState<N, C>& state);

}

namespace Security {
   struct AES_GCM_Context {
      std::array<uint8_t, 240> rk;
      std::array<uint8_t, 16> H;
   };
   // Global constant stored in .rodata
   static constexpr uint8_t SBOX[256] = {
      0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
      0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
      0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
      0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
      0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
      0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
      0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
      0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
      0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
      0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
      0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
      0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
      0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
      0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
      0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
      0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
   };

   // --- Key Management & Derivation ---

   /** @brief Derives a cryptographically secure 256-bit key from a user password and salt. Utilizes PBKDF2-HMAC-SHA256. Required to turn human-readable passwords into valid cipher keys. */
   inline void DeriveKey(const char* __restrict password, const uint8_t* __restrict salt, size_t salt_len, uint8_t* __restrict out_key_256);

   // --- Authenticated Encryption with Associated Data (AEAD) ---

   /** @brief ChaCha20-Poly1305 AEAD Encryption. The modern standard for secure streams. Encrypts the PCM buffer and generates a MAC tag to ensure the audio file has not been maliciously tampered with. */
   inline void EncryptChaCha20Poly1305(uint8_t* __restrict buffer, size_t length, const uint8_t* key_256, const uint8_t* __restrict nonce_96, uint8_t* __restrict out_mac_tag_128);

   /** 
    * @brief Decrypts ChaCha20 audio and verifies the Poly1305 MAC tag. 
    * @paragraph Execution
    * If the MAC tag fails verification, the audio has been corrupted or altered. The buffer is immediately zeroed 
    * out to prevent playing dangerous digital noise through the I2S hardware.
    * @return bool True if authentic and decrypted, False if verification fails.
    */
   [[nodiscard]] inline bool DecryptChaCha20Poly1305(uint8_t* __restrict buffer, size_t length, const uint8_t* __restrict key_256, const uint8_t* __restrict nonce_96, const uint8_t* __restrict mac_tag_128);

   // --- Hardware-Accelerated Block Ciphers ---

   /** * @brief Pre-calculates the key schedule and the GCM Hash key (H). */
   inline void InitAES_GCM(const uint8_t* key_256, AES_GCM_Context& ctx);

   /** * @brief Pure software implementation of AES-256-GCM.
    * A hardware-agnostic, zero-allocation implementation that calculates the AES matrix 
    * transformations (Rijndael S-Box) and Galois Field GF(2^128) multipliers natively in C++. 
    */
   [[nodiscard]] inline bool ProcessAES_GCM(uint8_t* __restrict buffer, size_t length, const AES_GCM_Context& ctx, const uint8_t* __restrict iv_96, uint8_t* __restrict in_out_mac_tag, bool is_encrypting);

   // --- Audio Steganography (Data Concealment) ---

   /** 
    * @brief Embeds an encrypted binary payload invisibly into the audio waveform.
    * @paragraph Mechanism
    * Modifies the Least Significant Bits (LSB) of the 16-bit PCM audio samples. The resulting file 
    * will sound exactly like normal music to the human ear, but acts as a secure data carrier.
    */
   [[nodiscard]] inline bool EmbedPayloadLSB(int16_t* __restrict audio_buffer, size_t audio_frames, const uint8_t* __restrict payload, size_t payload_bytes);

   /** @brief Extracts a hidden binary payload from the LSBs of an audio buffer. Typically followed by passing the extracted payload into the decryption matrix. */
   [[nodiscard]] inline bool ExtractPayloadLSB(const int16_t* __restrict audio_buffer, size_t audio_frames, uint8_t* __restrict out_payload, size_t expected_bytes);

   // --- Cryptographic Hashing ---

   /** @brief Generates a SHA-256 checksum of an audio block. Used to verify the structural integrity of WAV headers or to fingerprint specific audio files. */
   inline void CalculateSHA256(const uint8_t* __restrict buffer, size_t length, uint8_t* __restrict out_hash_256);

}

/* IMPLEMENTATIONS - NOTHING HERE */


// CONVERT START:

inline void Convert::ToFloat32(const int16_t* __restrict in, float* __restrict out, size_t length){
   const float inv_max = 3.0517578125e-5f; 
   for (size_t i = 0; i < length; i++) out[i] = static_cast<float>(in[i]) * inv_max;
}

inline void Convert::ToInt16(const float* __restrict in, int16_t* __restrict out, size_t length){
   for (size_t i = 0; i < length; i++) {
      float clamped = std::clamp(in[i], -1.0f, 1.0f);
      out[i] = static_cast<int16_t>(clamped * 32767.0f);
   }
}

inline void Convert::To8BitSigned(const int16_t* __restrict in, int8_t* __restrict out, size_t length){
   for (size_t i = 0; i < length; i++) out[i] = static_cast<int8_t>(in[i] >> 8);
}

inline void Convert::To8BitUnsigned(const int16_t* __restrict in, uint8_t* __restrict out, size_t length){
   for (size_t i = 0; i < length; ++i) out[i] = static_cast<uint8_t>((in[i] >> 8) + 128); 
}

inline void Convert::From24Bit(const uint8_t* __restrict in24, int16_t* __restrict out16, size_t frames){
   for (size_t i = 0; i < frames; ++i){
      size_t byte_idx = i * 3;
      out16[i] = static_cast<int16_t>(in24[byte_idx + 1] | (static_cast<int8_t>(in24[byte_idx + 2]) << 8));
   }
}

inline void Convert::Deinterleave(const int16_t* __restrict interleaved_in, int16_t* __restrict left_out, int16_t* __restrict right_out, size_t frames){
   for (size_t i = 0; i < frames; i++){
      left_out[i]  = interleaved_in[i * 2];
      right_out[i] = interleaved_in[i * 2 + 1];
   }
}

inline void Convert::Interleave(const int16_t* __restrict left_in, const int16_t* __restrict right_in, int16_t* __restrict interleaved_out, size_t frames){
   for (size_t i = 0; i < frames; i++){
      interleaved_out[i * 2] = left_in[i];
      interleaved_out[i * 2 + 1] = right_in[i];
   }
}

inline void Convert::InvertPhase(int16_t* __restrict buffer, size_t length){
   for (size_t i = 0; i < length; i++) buffer[i] = buffer[i] == INT16_MIN ? INT16_MAX : -buffer[i];
}

inline void Convert::SwapEndian16(int16_t* __restrict buffer, size_t length){
   for (size_t i = 0; i < length; i++) {
      uint16_t val = static_cast<uint16_t>(buffer[i]);
      buffer[i] = static_cast<int16_t>((val << 8) | (val >> 8));
   }
}

inline void Convert::UnsignedToSigned16(const uint16_t* __restrict in, int16_t* __restrict out, size_t length){
   for (size_t i = 0; i < length; i++) out[i] = static_cast<int16_t>(in[i] ^ 0x8000);
}

inline void Convert::SignedToUnsigned16(const int16_t* __restrict in, uint16_t* __restrict out, size_t length){
   for (size_t i = 0; i < length; i++) out[i] = static_cast<uint16_t>(in[i]) ^ 0x8000;
}

inline void Convert::EncodeMidSide(const int16_t* __restrict lr_in, int16_t* __restrict ms_out, size_t frames){
   for (size_t i = 0; i < frames; i++){
      ms_out[i * 2] = static_cast<int16_t>((static_cast<int32_t>(lr_in[i * 2]) + static_cast<int32_t>(lr_in[i * 2 + 1])) >> 1);
      ms_out[i * 2 + 1] = static_cast<int16_t>((static_cast<int32_t>(lr_in[i * 2]) - static_cast<int32_t>(lr_in[i * 2 + 1])) >> 1);
   }
}

inline void Convert::DecodeMidSide(const int16_t* __restrict ms_in, int16_t* __restrict lr_out, size_t frames){
   for (size_t i = 0; i < frames; i++){
      lr_out[i * 2] = static_cast<int16_t>(static_cast<int32_t>(ms_in[i * 2]) + static_cast<int32_t> (ms_in[i * 2 + 1]));
      lr_out[i * 2 + 1] = static_cast<int16_t>(static_cast<int32_t>(ms_in[i * 2]) - static_cast<int32_t>(ms_in[i * 2 + 1]));
   }
}

inline void Convert::ResampleNearest(const int16_t* __restrict in, int16_t* __restrict out, size_t in_frames, size_t out_frames){
   for (size_t i = 0; i < out_frames; i++){
      size_t in_idx = static_cast<size_t>((static_cast<uint64_t>(i) * in_frames) / out_frames);
      out[i * 2] = in[in_idx * 2];
      out[i * 2 + 1] = in[in_idx * 2 + 1];
   }
}

inline void Convert::ResampleLinear(const int16_t* __restrict in, int16_t* __restrict out, size_t in_frames, size_t out_frames) {
   for (size_t i = 0; i < out_frames; i++) {
      int64_t in_idx = ((static_cast<int64_t>(i) * static_cast<int64_t>(in_frames)) << 15) / static_cast<int64_t>(out_frames);
      int64_t whole = in_idx >> 15;
      int64_t fl = in_idx & 0x7FFFULL;
      int16_t y0l = in[whole * 2];
      int16_t y0r = in[whole * 2 + 1];
      int64_t next_whole = whole + 1;
      if (next_whole >= in_frames) next_whole = whole;
      int16_t y1l = in[next_whole * 2];
      int16_t y1r = in[next_whole * 2 + 1];
      out[i * 2]     = static_cast<int16_t>(y0l + ((static_cast<int32_t>(fl) * (y1l - y0l)) >> 15));
      out[i * 2 + 1] = static_cast<int16_t>(y0r + ((static_cast<int32_t>(fl) * (y1r - y0r)) >> 15));
   }
}

inline void Convert::RemoveDCOffset(int16_t* __restrict buffer, size_t length) {
   int64_t sum = 0LL;
   for (size_t i = 0; i < length; i++) sum += buffer[i];
   int64_t signed_len = static_cast<int64_t>(length);
   int16_t mean = sum < 0 ? static_cast<int16_t>((sum - (signed_len / 2)) / signed_len) : static_cast<int16_t>((sum + (signed_len / 2)) / signed_len);
   for (size_t i = 0; i < length; i++) buffer[i] = static_cast<int16_t>(std::clamp(static_cast<int32_t>(buffer[i]) - mean, INT16_MIN, INT16_MAX));
}

inline void Convert::RemoveDCOffsetRT(int16_t* __restrict buffer, size_t length, int16_t& prev_x, float& prev_y) {
   const float R = 0.995f;
   for (size_t i = 0; i < length; i++) {
      int16_t current_x = buffer[i];
      prev_y = static_cast<float>(current_x - prev_x) + (R * prev_y);
      prev_x = current_x;
      buffer[i] = static_cast<int16_t>(std::clamp(static_cast<int32_t>(prev_y), INT16_MIN, INT16_MAX));
   }
}

inline void Convert::NormalizePeak(int16_t* __restrict buffer, size_t length, float target_peak) {
   if (length == 0) return;
   int32_t peak32 = std::abs(static_cast<int32_t>(buffer[0]));
   for (size_t i = 1; i < length; i++) peak32 = std::max(std::abs(static_cast<int32_t>(buffer[i])), peak32);
   if (!peak32) return;
   const int32_t Gain = (static_cast<int32_t>(target_peak * 32767.0f) << 15) / peak32;
   for (size_t i = 0; i < length; i++) {
      int32_t scaled_sample = (static_cast<int32_t>(buffer[i]) * Gain) >> 15;
      buffer[i] = static_cast<int16_t>(std::clamp(scaled_sample, INT16_MIN, INT16_MAX));
   }
}

inline void Convert::To8BitDithered(const int16_t* __restrict in, int8_t* __restrict out, size_t length, uint32_t& prng_state){
   for (size_t i = 0; i < length; i++) {
      prng_state = prng_state * 1103515245 + 12345;
      const int32_t __ran1 = static_cast<int32_t>(prng_state >> 24) - 0x80;
      prng_state = prng_state * 1103515245 + 12345;
      const int32_t __ran2 = static_cast<int32_t>(prng_state >> 24) - 0x80;
      out[i] = static_cast<int8_t>(std::clamp((static_cast<int32_t>(in[i]) + __ran1 + __ran2) >> 8, INT8_MIN, INT8_MAX));
   }
}

inline void Convert::PcmToMuLaw(const int16_t* __restrict pcm_in, uint8_t* __restrict mulaw_out, size_t length) {
   for (size_t i = 0; i < length; i++) {
      int32_t num = std::abs(static_cast<int32_t>(pcm_in[i]));
      if (num > 32635) num = 32635;
      num += 132;
      const uint8_t msb = static_cast<uint8_t>(32 - __builtin_clz(static_cast<uint32_t>(num)));
      mulaw_out[i] = ~((static_cast<uint8_t>(pcm_in[i] >= 0) << 7) | ((msb - 7) << 4) | ((num >> (msb - 4)) & 0x0F));
   }
}

inline void Convert::MuLawToPcm(const uint8_t* __restrict mulaw_in, int16_t* __restrict pcm_out, size_t length) {
   for (size_t i = 0; i < length; i++) {
      const uint8_t num = ~mulaw_in[i];
      const uint8_t msb = ((num >> 4) & 0x07) + 7;
      const int32_t res = ((1 << msb) | ((num & 0x0F) << (msb - 4)) | (1 << (msb - 5))) - 132;
      pcm_out[i] = static_cast<int16_t>((num & 0x80) ? res : -res);
   }
}

inline void Convert::PcmToALaw(const int16_t* __restrict pcm_in, uint8_t* __restrict alaw_out, size_t length) {
   for (size_t i = 0; i < length; i++) {
      int32_t num = std::abs(static_cast<int32_t>(pcm_in[i]));
      if (num > 32767) num = 32767;
      const uint8_t seg = (num < 256) ? 0 : static_cast<uint8_t>(32 - __builtin_clz(static_cast<uint32_t>(num))) - 7;
      alaw_out[i] = ((static_cast<uint8_t>(pcm_in[i] >= 0) << 7) | (seg << 4) | ((num < 256) ? static_cast<uint8_t>((num >> 4) & 0x0F) : static_cast<uint8_t>((num >> (seg + 3)) & 0x0F))) ^ 0x55;
   }
}

inline void Convert::ALawToPcm(const uint8_t* __restrict alaw_in, int16_t* __restrict pcm_out, size_t length) {
   for (size_t i = 0; i < length; i++) {
      const uint8_t num = alaw_in[i] ^ 0x55;
      const uint8_t seg = (num >> 4) & 0x07;
      const int32_t res = seg == 0 ? ((num & 0x0F) << 4) | 8 : (1 << (seg + 7)) | ((num & 0x0F) << (seg + 3)) | (1 << (seg + 2));
      pcm_out[i] = static_cast<int16_t>((num & 0x80) ? res : -res);
   }
}

inline void Convert::ZeroPad(int16_t* __restrict buffer, size_t current_length, size_t target_length) {
   for (size_t i = current_length; i < target_length; i++) buffer[i] = 0;
}

inline void Convert::ApplyFadeInOut(int16_t* __restrict buffer, size_t length, size_t fade_frames) {
   const size_t fade = fade_frames > length / 2 ? length / 2 : fade_frames;
   if (!fade) return;
   for (size_t i = 0; i < fade; i++) {
      buffer[i] = static_cast<int16_t>((static_cast<int64_t>(buffer[i]) * static_cast<int64_t>(i)) / static_cast<int64_t>(fade));
      buffer[length - 1 - i] = static_cast<int16_t>((static_cast<int64_t>(buffer[length - 1 - i]) * static_cast<int64_t>(i)) / static_cast<int64_t>(fade));
   }
}

[[nodiscard]] inline float Effects::ProcessLFO(LFOState& lfo) {
   float out = std::sin(lfo.phase * 2.0f * M_PI);
   lfo.phase += lfo.phase_increment;
   if (lfo.phase >= 1.0f) lfo.phase -= 1.0f;
   return out;
}

[[nodiscard]] inline float Effects::Amplitude::ApplyVolume(float input, float multiplier) {
   return input * multiplier;
}

[[nodiscard]] inline float Effects::Amplitude::NoiseGate(float input, float threshold) {
   return (std::abs(input) < threshold) ? 0.0f : input;
}

[[nodiscard]] inline float Effects::Amplitude::HardClip(float input, float threshold) {
   return std::clamp(input, -threshold, threshold);
}

[[nodiscard]] inline float Effects::Amplitude::SoftClipTanh(float input, float drive) {
   return std::tanh(input * drive);
}

[[nodiscard]] inline float Effects::Amplitude::SoftClipPoly(float input, float drive) {
   float x = input * drive;
   if (x <= -1.0f) return -0.66667f;
   if (x >= 1.0f) return 0.66667f;
   return x - (x * x * x) / 3.0f;
}

[[nodiscard]] inline float Effects::Amplitude::AsymmetricFuzz(float input, float drive, float bias) {
   float driven = input * drive + bias;
   if (driven > 0.0f) return std::tanh(driven);
   return std::max(driven, -1.0f);
}

[[nodiscard]] inline float Effects::Amplitude::BitCrush(float input, float steps) {
   return std::round(input * steps) / steps;
}

inline void Effects::Amplitude::ApplyVolumeBlock(float* __restrict buffer, size_t length, float multiplier) {
   for (size_t i = 0; i < length; i++) buffer[i] *= multiplier;
}

inline void Effects::Amplitude::NoiseGateBlock(float* __restrict buffer, size_t length, float threshold) {
   for (size_t i = 0; i < length; i++) buffer[i] = (std::abs(buffer[i]) < threshold) ? 0.0f : buffer[i];
}

inline void Effects::Amplitude::HardClipBlock(float* __restrict buffer, size_t length, float threshold) {
   for (size_t i = 0; i < length; i++) buffer[i] = std::clamp(buffer[i], -threshold, threshold);
}

inline void Effects::Amplitude::SoftClipTanhBlock(float* __restrict buffer, size_t length, float drive) {
   for (size_t i = 0; i < length; i++) buffer[i] = std::tanh(buffer[i] * drive);
}

inline void Effects::Amplitude::SoftClipPolyBlock(float* __restrict buffer, size_t length, float drive) {
   for (size_t i = 0; i < length; i++) {
      float x = buffer[i] * drive;
      if (x <= -1.0f) buffer[i] = -0.66667f;
      else if (x >= 1.0f) buffer[i] = 0.66667f;
      else buffer[i] = x - (x * x * x) / 3.0f;
   }
}

inline void Effects::Amplitude::AsymmetricFuzzBlock(float* __restrict buffer, size_t length, float drive, float bias) {
   for (size_t i = 0; i < length; i++) {
      float driven = buffer[i] * drive + bias;
      if (driven > 0.0f) buffer[i] = std::tanh(driven);
      else buffer[i] = std::max(driven, -1.0f);
   }
}

inline void Effects::Amplitude::BitCrushBlock(float* __restrict buffer, size_t length, float steps) {
   for (size_t i = 0; i < length; i++) buffer[i] = std::round(buffer[i] * steps) / steps;
}

template <size_t N>
inline void Effects::Time::WriteDelay(DelayState<N>& state, float input) {
   state.ring_buffer[state.write_index] = static_cast<int16_t>(input * 32767.0f);
   state.write_index = (state.write_index + 1) % state.ring_buffer.size();
}

template <size_t N>
[[nodiscard]] inline float Effects::Time::ReadDelayInteger(const DelayState<N>& state, uint32_t delay_samples) {
   int32_t read_idx = state.write_index - delay_samples;
   if (read_idx < 0) read_idx += state.ring_buffer.size();
   return state.ring_buffer[read_idx] / 32768.0f;
}

template <size_t N>
[[nodiscard]] inline float Effects::Time::ReadDelayInterpolated(const DelayState<N>& state, float delay_samples) {
   uint32_t delay_int = static_cast<uint32_t>(delay_samples);
   float frac = delay_samples - delay_int;
   int32_t idx1 = state.write_index - delay_int;
   if (idx1 < 0) idx1 += state.ring_buffer.size();
   int32_t idx2 = idx1 - 1;
   if (idx2 < 0) idx2 += state.ring_buffer.size();
   float s1 = state.ring_buffer[idx1] / 32768.0f;
   float s2 = state.ring_buffer[idx2] / 32768.0f;
   return s1 + frac * (s2 - s1);
}

template <size_t N>
[[nodiscard]] inline float Effects::Time::CombFilter(DelayState<N>& state, float input, float delay_samples, float feedback) {
   float delayed = ReadDelayInteger(state, delay_samples);
   WriteDelay(state, input + (delayed * feedback));
   return delayed;
}

template <size_t N>
[[nodiscard]] inline float Effects::Time::AllPassFilter(DelayState<N>& state, float input, float delay_samples, float gain) {
   float delayed = ReadDelayInteger(state, delay_samples);
   float feed_forward = input + (delayed * gain);
   WriteDelay(state, feed_forward);
   return delayed - (gain * feed_forward);
}

template <size_t N>
[[nodiscard]] inline float Effects::Time::ReadDelayHermite(const DelayState<N>& state, float delay_samples) {
   uint32_t delay_int = static_cast<uint32_t>(delay_samples);
   float frac = delay_samples - delay_int;
   int32_t idx1 = state.write_index - delay_int;
   int32_t idx0 = idx1 + 1;
   int32_t idx2 = idx1 - 1;
   int32_t idx3 = idx1 - 2;
   if (idx0 >= static_cast<int32_t>(state.ring_buffer.size())) idx0 -= state.ring_buffer.size();
   if (idx1 < 0) idx1 += state.ring_buffer.size();
   if (idx2 < 0) idx2 += state.ring_buffer.size();
   if (idx3 < 0) idx3 += state.ring_buffer.size();
   float y0 = state.ring_buffer[idx0] / 32768.0f;
   float y1 = state.ring_buffer[idx1] / 32768.0f;
   float y2 = state.ring_buffer[idx2] / 32768.0f;
   float y3 = state.ring_buffer[idx3] / 32768.0f;
   float c0 = y1;
   float c1 = 0.5f * (y2 - y0);
   float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
   float c3 = 1.5f * (y1 - y2) + 0.5f * (y3 - y0);
   return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

[[nodiscard]] inline float Effects::Filter::ProcessBiquad(float input, BiquadState& state, const BiquadCoeffs& coeffs) {
   float out = (coeffs.b0 * input) + (coeffs.b1 * state.x1) + (coeffs.b2 * state.x2) - (coeffs.a1 * state.y1) - (coeffs.a2 * state.y2);
   state.x2 = state.x1; state.x1 = input;
   state.y2 = state.y1; state.y1 = out;
   return out;
}

inline void Effects::Filter::CalcLowpass(BiquadCoeffs& coeffs, float sample_rate, float cutoff_freq, float q_factor) {
   float w0 = 2.0f * M_PI * cutoff_freq / sample_rate;
   float alpha = std::sin(w0) / (2.0f * q_factor);
   float cos_w0 = std::cos(w0);
   float a0 = 1.0f + alpha;
   coeffs.b0 = ((1.0f - cos_w0) / 2.0f) / a0;
   coeffs.b1 = (1.0f - cos_w0) / a0;
   coeffs.b2 = ((1.0f - cos_w0) / 2.0f) / a0;
   coeffs.a1 = (-2.0f * cos_w0) / a0;
   coeffs.a2 = (1.0f - alpha) / a0;
}

inline void Effects::Filter::CalcHighpass(BiquadCoeffs& coeffs, float sample_rate, float cutoff_freq, float q_factor) {
   float w0 = 2.0f * M_PI * cutoff_freq / sample_rate;
   float alpha = std::sin(w0) / (2.0f * q_factor);
   float cos_w0 = std::cos(w0);
   float a0 = 1.0f + alpha;
   coeffs.b0 = ((1.0f + cos_w0) / 2.0f) / a0;
   coeffs.b1 = -(1.0f + cos_w0) / a0;
   coeffs.b2 = ((1.0f + cos_w0) / 2.0f) / a0;
   coeffs.a1 = (-2.0f * cos_w0) / a0;
   coeffs.a2 = (1.0f - alpha) / a0;
}

inline void Effects::Filter::CalcBandpass(BiquadCoeffs& coeffs, float sample_rate, float center_freq, float q_factor) {
   float w0 = 2.0f * M_PI * center_freq / sample_rate;
   float alpha = std::sin(w0) / (2.0f * q_factor);
   float a0 = 1.0f + alpha;
   coeffs.b0 = alpha / a0;
   coeffs.b1 = 0.0f;
   coeffs.b2 = -alpha / a0;
   coeffs.a1 = (-2.0f * std::cos(w0)) / a0;
   coeffs.a2 = (1.0f - alpha) / a0;
}

inline void Effects::Filter::CalcLowShelf(BiquadCoeffs& coeffs, float sample_rate, float cutoff_freq, float gain_db) {
   float A = std::pow(10.0f, gain_db / 40.0f);
   float w0 = 2.0f * M_PI * cutoff_freq / sample_rate;
   float alpha = std::sin(w0) / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / 0.707f) - 1.0f);
   float cos_w0 = std::cos(w0);
   float a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * std::sqrt(A) * alpha;
   coeffs.b0 = (A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * std::sqrt(A) * alpha)) / a0;
   coeffs.b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0)) / a0;
   coeffs.b2 = (A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * std::sqrt(A) * alpha)) / a0;
   coeffs.a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0)) / a0;
   coeffs.a2 = ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * std::sqrt(A) * alpha) / a0;
}

inline void Effects::Filter::CalcAllPass(BiquadCoeffs& coeffs, float sample_rate, float center_freq, float q_factor) {
   float w0 = 2.0f * M_PI * center_freq / sample_rate;
   float alpha = std::sin(w0) / (2.0f * q_factor);
   float a0 = 1.0f + alpha;
   coeffs.b0 = (1.0f - alpha) / a0;
   coeffs.b1 = (-2.0f * std::cos(w0)) / a0;
   coeffs.b2 = (1.0f + alpha) / a0;
   coeffs.a1 = coeffs.b1;
   coeffs.a2 = coeffs.b0;
}

inline void Effects::Filter::ProcessBiquadBlock(float* __restrict buffer, size_t length, BiquadState& state, const BiquadCoeffs& coeffs) {
   float x1 = state.x1, x2 = state.x2;
   float y1 = state.y1, y2 = state.y2;
   const float b0 = coeffs.b0, b1 = coeffs.b1, b2 = coeffs.b2, a1 = coeffs.a1, a2 = coeffs.a2;
   for (size_t i = 0; i < length; i++) {
      float in = buffer[i];
      float out = (b0 * in) + (b1 * x1) + (b2 * x2) - (a1 * y1) - (a2 * y2);
      x2 = x1; x1 = in;
      y2 = y1; y1 = out;
      buffer[i] = out;
   }
   state.x1 = x1; state.x2 = x2;
   state.y1 = y1; state.y2 = y2;
}

[[nodiscard]] inline float Effects::Pitch::StepLFO(LFOState& lfo) {
   float out = std::sin(lfo.phase * 2.0f * M_PI);
   lfo.phase += lfo.phase_increment;
   if (lfo.phase >= 1.0f) lfo.phase -= 1.0f;
   return out;
}

template <size_t N>
[[nodiscard]] inline float Effects::Pitch::ProcessVibrato(float input, DelayState<N>& delay, LFOState& lfo, float depth_samples) {
   float lfo_val = StepLFO(lfo); 
   float current_delay = depth_samples + (lfo_val * depth_samples); // Sweep delay time
   float output = Effects::Time::ReadDelayInterpolated(delay, current_delay);
   Effects::Time::WriteDelay(delay, input);
   return output;
}

template <size_t N>
[[nodiscard]] inline float Effects::Pitch::ProcessChorus(float input, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay) {
   float lfo_val = StepLFO(lfo); 
   float current_delay = base_delay + (lfo_val * depth_samples);
   float wet = Effects::Time::ReadDelayInterpolated(delay, current_delay);
   Effects::Time::WriteDelay(delay, input);
   return (input * 0.5f) + (wet * 0.5f);
}

template <size_t N>
[[nodiscard]] inline float Effects::Pitch::ProcessFlanger(float input, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay, float feedback) {
   float lfo_val = StepLFO(lfo); 
   float current_delay = base_delay + (lfo_val * depth_samples);
   float wet = Effects::Time::ReadDelayInterpolated(delay, current_delay);
   Effects::Time::WriteDelay(delay, input + (wet * feedback));
   return (input * 0.5f) + (wet * 0.5f);
}

template <size_t N>
inline void Effects::Pitch::ProcessVibratoBlock(float* __restrict buffer, size_t length, DelayState<N>& delay, LFOState& lfo, float depth_samples) {
   for (size_t i = 0; i < length; i++) buffer[i] = ProcessVibrato(buffer[i], delay, lfo, depth_samples);
}

template <size_t N>
inline void Effects::Pitch::ProcessChorusBlock(float* __restrict buffer, size_t length, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay) {
   for (size_t i = 0; i < length; i++) buffer[i] = ProcessChorus(buffer[i], delay, lfo, depth_samples, base_delay);
}

template <size_t N>
inline void Effects::Pitch::ProcessFlangerBlock(float* __restrict buffer, size_t length, DelayState<N>& delay, LFOState& lfo, float depth_samples, float base_delay, float feedback) {
   for (size_t i = 0; i < length; i++) buffer[i] = ProcessFlanger(buffer[i], delay, lfo, depth_samples, base_delay, feedback);
}

template <size_t N>
inline void FFT::Initialize(FFTState<N>& state, size_t fft_size){
   state.bit_reversed_indices.clear();
   state.twiddles.clear();
   state.bit_reversed_indices.reserve(fft_size);
   state.twiddles.reserve(fft_size / 2);
   const uint8_t size = static_cast<uint8_t>(__builtin_ctz(fft_size));
   for (size_t i = 0; i < fft_size; i++){
      size_t k = 0;
      for (uint8_t j = 0; j < size; j++) k = (k << 1) | ((i >> j) & 1);
      state.bit_reversed_indices.push_back(k);
   }
   for (size_t i = 0; i < fft_size / 2; i++){
      float angle = -2.0f * static_cast<float>(M_PI) * i / fft_size;
      state.twiddles.push_back({std::cos(angle), std::sin(angle)});
   }
}

inline void FFT::ApplyHannWindow(float* buffer, size_t length){
   const float period = 2.0f * static_cast<float>(M_PI) / static_cast<float>(length);
   for (size_t i = 0; i < length; i++) buffer[i] *= (1.0f - std::cos(period * static_cast<float>(i))) * 0.5f;
}

template <size_t N>
inline void FFT::Forward(std::complex<float>* __restrict buffer, const FFTState<N>& state, size_t fft_size){
   for (size_t i = 0; i < fft_size; i++) if (i < state.bit_reversed_indices[i]) std::swap(buffer[i], buffer[state.bit_reversed_indices[i]]);
   for (size_t stage_size = 2; stage_size <= fft_size; stage_size *= 2){
      const auto step = fft_size / stage_size;
      for (size_t w = 0; w < stage_size / 2; w++){
         for (size_t t = w; t < fft_size; t += stage_size){
            const auto A = buffer[t];
            const auto B = buffer[t + stage_size / 2];
            const auto T = B * state.twiddles[w * step];
            buffer[t] = A + T;
            buffer[t + stage_size / 2] = A - T;
         }
      }
   }
}

template <size_t N>
inline void FFT::Inverse(std::complex<float>* __restrict buffer, const FFTState<N>& state, size_t fft_size){
   for (size_t i = 0; i < fft_size; i++) if (i < state.bit_reversed_indices[i]) std::swap(buffer[i], buffer[state.bit_reversed_indices[i]]);
   for (size_t stage_size = 2; stage_size <= fft_size; stage_size *= 2){
      const auto step = fft_size / stage_size;
      for (size_t w = 0; w < stage_size / 2; w++){
         for (size_t t = w; t < fft_size; t += stage_size){
            const auto A = buffer[t];
            const auto B = buffer[t + stage_size / 2];
            const auto T = B * std::conj(state.twiddles[w * step]);
            buffer[t] = A + T;
            buffer[t + stage_size / 2] = A - T;
         }
      }
   }
   for (size_t i = 0; i < fft_size; i++) buffer[i] *= 1.0f / fft_size;
}

[[nodiscard]] inline float FFT::GetMagnitude(const std::complex<float>& bin){
   return std::abs(bin);
}

inline void FFT::CalculateMagnitudes(const std::complex<float>* __restrict fft_buffer, float* __restrict mag_buffer, size_t half_fft_size){
   for (size_t i = 0; i < half_fft_size; i++) mag_buffer[i] = FFT::GetMagnitude(fft_buffer[i]) * (i ? 1.0f / half_fft_size : 0.5f / half_fft_size);
}

inline void Spatial::CalcStereoPan(float angle_degrees, float& out_left_gain, float& out_right_gain){
   const float rad = angle_degrees * static_cast<float>(M_PI) / 360.0f;
   out_left_gain = std::cos(rad);
   out_right_gain = std::sin(rad);

}

[[nodiscard]] inline float Spatial::CalcDistanceAttenuation(float distance, float min_distance, float max_distance, float rolloff_factor){
   distance = std::clamp(distance, min_distance, max_distance);
   return min_distance / (min_distance + rolloff_factor * (distance - min_distance));
}

[[nodiscard]] inline float Spatial::CalcDopplerPitch(float source_velocity, float listener_velocity, float speed_of_sound){
   return (speed_of_sound - listener_velocity) / (speed_of_sound - source_velocity);
}

inline void Spatial::Apply3DPanning(float mono_in, float& stereo_left_out, float& stereo_right_out, float left_gain, float right_gain){
   stereo_left_out = mono_in * left_gain;
   stereo_right_out = mono_in * right_gain;
}

inline void Spatial::Mix3DPanning(float mono_in, float& stereo_left_out, float& stereo_right_out, float left_gain, float right_gain) {
   stereo_left_out += mono_in * left_gain;
   stereo_right_out += mono_in * right_gain;
}

inline void Spatial::CalcITD(float angle_radians, float head_radius_meters, float speed_of_sound, float& out_left_delay_sec, float& out_right_delay_sec) {
   float theta = std::abs(angle_radians);
   float delay = (head_radius_meters / speed_of_sound) * (theta + std::sin(theta));
   if (angle_radians > 0.0f) {
      out_left_delay_sec = delay;
      out_right_delay_sec = 0.0f;
   }
   else {
      out_left_delay_sec = 0.0f;
      out_right_delay_sec = delay;
   }
}

inline void Spatial::CalcHeadShadowCutoffs(float angle_radians, float max_cutoff_hz, float min_cutoff_hz, float& out_left_cutoff, float& out_right_cutoff) {
   float normalized_angle = std::abs(angle_radians) / static_cast<float>(M_PI);
   float shadow_freq = max_cutoff_hz - (normalized_angle * (max_cutoff_hz - min_cutoff_hz));
   if (angle_radians > 0.0f) {
      out_left_cutoff = shadow_freq;
      out_right_cutoff = max_cutoff_hz;
   }
   else {
      out_left_cutoff = max_cutoff_hz;
      out_right_cutoff = shadow_freq;
   }
}

[[nodiscard]] inline float Spatial::CalcLogarithmicAttenuation(float distance, float min_distance, float max_distance) {
   if (distance <= min_distance) return 1.0f;
   if (distance >= max_distance) return 0.0f;
   float ratio = distance / min_distance;
   float attenuation_db = -20.0f * std::log10(ratio); 
   return std::pow(10.0f, attenuation_db / 20.0f);
}

inline void Spatial::CalcQuadraphonicPan(float azimuth_radians, float& out_fl, float& out_fr, float& out_rl, float& out_rr) {
   while (azimuth_radians < 0) azimuth_radians += 2.0f * M_PI;
   while (azimuth_radians >= 2.0f * M_PI) azimuth_radians -= 2.0f * M_PI;
   out_fl = std::max(0.0, std::cos(azimuth_radians - (M_PI / 4.0f)));
   out_fr = std::max(0.0, std::cos(azimuth_radians - (7.0f * M_PI / 4.0f)));
   out_rl = std::max(0.0, std::cos(azimuth_radians - (3.0f * M_PI / 4.0f)));
   out_rr = std::max(0.0, std::cos(azimuth_radians - (5.0f * M_PI / 4.0f)));
}

inline void Generators::SetFrequency(OscillatorState& state, float frequency_hz, float sample_rate) {
   state.phase_increment = frequency_hz / sample_rate;
}

[[nodiscard]] inline float Generators::SineOsc(OscillatorState& state) {
   float out = std::sin(state.phase * 2.0f * M_PI);
   state.phase += state.phase_increment;
   if (state.phase >= 1.0f) state.phase -= 1.0f;
   return out;
}

[[nodiscard]] inline float Generators::SquareOsc(OscillatorState& state) {
   float out = (state.phase < 0.5f) ? 1.0f : -1.0f;
   state.phase += state.phase_increment;
   if (state.phase >= 1.0f) state.phase -= 1.0f;
   return out;
}

[[nodiscard]] inline float Generators::SawOsc(OscillatorState& state) {
   float out = 2.0f * state.phase - 1.0f;
   state.phase += state.phase_increment;
   if (state.phase >= 1.0f) state.phase -= 1.0f;
   return out;
}

[[nodiscard]] inline float Generators::TriangleOsc(OscillatorState& state) {
   float out = 2.0f * std::abs(2.0f * state.phase - 1.0f) - 1.0f;
   state.phase += state.phase_increment;
   if (state.phase >= 1.0f) state.phase -= 1.0f;
   return out;
}

[[nodiscard]] inline float Generators::WhiteNoise(uint32_t& prng_state) {
   prng_state ^= prng_state << 13;
   prng_state ^= prng_state >> 17;
   prng_state ^= prng_state << 5;
   return (static_cast<float>(prng_state) / 2147483648.0f) - 1.0f;
}

[[nodiscard]] inline float Generators::PinkNoise(uint32_t& prng_state, float& filter_state) {
   float white = Generators::WhiteNoise(prng_state);
   filter_state = (0.99886f * filter_state) + (white * 0.0555179f);
   return filter_state;
}

[[nodiscard]] inline float Generators::BrownNoise(uint32_t& prng_state, float& last_out) {
   float white = Generators::WhiteNoise(prng_state);
   last_out = (last_out * 0.98f) + (white * 0.05f);
   return last_out;
}

inline void Convolution::TrimIRSilence(std::vector<float>& ir_buffer, float silence_threshold){
   size_t start_idx = 0;
   for (size_t i = 0; i < ir_buffer.size(); ++i) {
      if (std::abs(ir_buffer[i]) > silence_threshold) {
         start_idx = i;
         break;
      }
   }
   if (start_idx > 0) ir_buffer.erase(ir_buffer.begin(), ir_buffer.begin() + start_idx);
}

inline void Convolution::NormalizeIR(std::vector<float>& ir_buffer) {
   float energy = 0.0f;
   for (float sample : ir_buffer) energy += sample * sample;
   if (energy > 0.0f) {
      const float gain = 1.0f / std::sqrt(energy);
      for (float& sample : ir_buffer) sample *= gain;
   }
}

inline void Convolution::ApplyIRTaper(std::vector<float>& ir_buffer, size_t taper_length_samples) {
   if (ir_buffer.size() < taper_length_samples) return;
   const size_t start_taper = ir_buffer.size() - taper_length_samples;
   for (size_t i = 0; i < taper_length_samples; ++i) {
      float multiplier = 0.5f * (1.0f + std::cos(M_PI * static_cast<float>(i) / static_cast<float>(taper_length_samples)));
      ir_buffer[start_taper + i] *= multiplier;
   }
}

template <size_t N>
inline void Convolution::InitDirectFIR(DirectFIRState<N>& state, const float* __restrict ir_data, size_t ir_length) {
   state.ir_buffer.assign(ir_data, ir_data + ir_length);
   state.history_buffer.assign(ir_length, 0.0f);
   state.head = 0;
}

template <size_t N>
[[nodiscard]] inline float Convolution::ProcessDirectFIRSample(float input, DirectFIRState<N>& state) {
   state.history_buffer[state.head] = input;
   float output = 0.0f;
   const size_t ir_len = state.ir_buffer.size();
   for (size_t i = 0; i < ir_len; ++i) {
      int32_t read_idx = state.head - i;
      if (read_idx < 0) read_idx += ir_len;
      output += state.ir_buffer[i] * state.history_buffer[read_idx];
   }
   state.head = (state.head + 1) % ir_len;
   return output;
}

template <size_t N, size_t C>
inline void Convolution::InitPartitionedConv(PartitionedConvState<N, C>& state, const float* __restrict ir_data, size_t ir_length, size_t partition_size){
   const size_t fft_size = partition_size * 2;
   const size_t partition_count = (ir_length + partition_size - 1) / partition_size;
   FFT::Initialize(state.fft_state, fft_size);
   std::fill(state.audio_history_fft.begin(), state.audio_history_fft.end(), std::complex<float>(0.0f, 0.0f));
   std::fill(state.overlap_buffer.begin(), state.overlap_buffer.end(), 0.0f);
   state.current_partition = 0;
   for (size_t i = 0; i < partition_count; i++){
      const size_t start_idx = i * partition_size;
      const size_t copy_length = std::min(partition_size, ir_length - start_idx);
      for (size_t j = 0; j < copy_length; j++) state.ir_partitions_fft[i][j].real(ir_data[start_idx + j]);
      FFT::Forward(&state.ir_partitions_fft[i * fft_size], state.fft_state, fft_size);
   }
}

template <size_t N, size_t C>
inline void Convolution::ProcessPartitionedBlock(float* __restrict io_buffer, PartitionedConvState<N, C>& state) {
   constexpr size_t fft_size = N * 2;
   constexpr size_t partition_count = C;
   std::fill(state.staging_buffer.begin(), state.staging_buffer.end(), std::complex<float>{0.0f, 0.0f});
   for (size_t i = 0; i < state.partition_size; i++) state.staging_buffer[i].real(io_buffer[i]);
   FFT::Forward(state.staging_buffer.data(), state.state, fft_size);
   for (size_t i = 0; i < fft_size; i++) state.audio_history_fft[state.current_partition * fft_size + i] = state.staging_buffer[i];
   std::fill(state.accumulator_buffer.begin(), state.accumulator_buffer.end(), std::complex<float>{0.0f, 0.0f});
   for (size_t step = 0; step < partition_count; step++) {
      const size_t history_idx = (state.current_partition + partition_count - step) % partition_count;
      for (size_t bin = 0; bin < fft_size; bin++) state.accumulator_buffer[bin] += state.audio_history_fft[history_idx][bin] * state.ir_partitions_fft[step][bin];
   }
   FFT::Inverse(state.accumulator_buffer.data(), state.state, fft_size);
   for (size_t i = 0; i < state.partition_size; i++) {
      io_buffer[i] = state.accumulator_buffer[i].real() + state.overlap_buffer[i];
      state.overlap_buffer[i] = state.accumulator_buffer[i + state.partition_size].real();
   }
   state.current_partition = (state.current_partition + 1) % partition_count;
}

inline void Security::DeriveKey(const char* __restrict password, const uint8_t* __restrict salt, size_t salt_len, uint8_t* __restrict out_key_256){
   const uint8_t ipad = 0x36, opad = 0x5C;
   uint8_t pass64[100]{};
   uint8_t U_copy[32]{};
   size_t len = strlen(password);
   if (len > 64) {
      len = 32;
      Security::CalculateSHA256(reinterpret_cast<const uint8_t*>(password), strlen(password), pass64);
   }
   else std::memcpy(pass64, password, len);
   for (size_t i = 0; i < 64; i++) pass64[i] ^= ipad;
   size_t inner_hash_len = 64;
   if (salt_len > 32) {
      Security::CalculateSHA256(salt, salt_len, pass64 + 64);
      inner_hash_len += 32;
   }
   else {
      std::memcpy(pass64 + 64, salt, salt_len);
      inner_hash_len += salt_len;
   }
   pass64[inner_hash_len + 0] = 0;
   pass64[inner_hash_len + 1] = 0;
   pass64[inner_hash_len + 2] = 0;
   pass64[inner_hash_len + 3] = 1;
   inner_hash_len += 4;
   Security::CalculateSHA256(pass64, inner_hash_len, U_copy);
   for (size_t i = 0; i < 64; i++) pass64[i] ^= ipad ^ opad;
   std::memcpy(pass64 + 64, U_copy, 32);
   Security::CalculateSHA256(pass64, 96, out_key_256);
   std::memcpy(U_copy, out_key_256, 32);
   for (size_t iter = 1; iter < 100000; iter++){
      for (size_t i = 0; i < 64; i++) pass64[i] ^= ipad ^ opad;
      std::memcpy(pass64 + 64, U_copy, 32);
      Security::CalculateSHA256(pass64, 96, U_copy);
      for (size_t i = 0; i < 64; i++) pass64[i] ^= (ipad ^ opad);
      std::memcpy(pass64 + 64, U_copy, 32);
      Security::CalculateSHA256(pass64, 96, U_copy);
      for (size_t i = 0; i < 32; i++) out_key_256[i] ^= U_copy[i];
   }
}

inline void EncryptChaCha20Poly1305(uint8_t* __restrict buffer, size_t length, const uint8_t* __restrict key_256, const uint8_t* __restrict nonce_96, uint8_t* __restrict out_mac_tag_128) {
   auto LOAD32LE = [](const uint8_t* p) -> uint32_t { return (static_cast<uint32_t>(p[0]) << 0) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24); };
   auto STORE32LE = [](uint8_t* p, uint32_t v) { p[0] = static_cast<uint8_t>(v >> 0); p[1] = static_cast<uint8_t>(v >> 8); p[2] = static_cast<uint8_t>(v >> 16); p[3] = static_cast<uint8_t>(v >> 24); };
   auto ROTL = [](uint32_t x, size_t n) -> uint32_t { return (x << n) | (x >> (32 - n)); };
   auto QROUND = [&](uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
      a += b; d ^= a; d = ROTL(d, 16); c += d; b ^= c; b = ROTL(b, 12);
      a += b; d ^= a; d = ROTL(d, 8); c += d; b ^= c; b = ROTL(b, 7);
   };
   uint32_t state[16] = {
      0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
      LOAD32LE(key_256 + 0), LOAD32LE(key_256 + 4), LOAD32LE(key_256 + 8), LOAD32LE(key_256 + 12),
      LOAD32LE(key_256 + 16), LOAD32LE(key_256 + 20), LOAD32LE(key_256 + 24), LOAD32LE(key_256 + 28),
      0, LOAD32LE(nonce_96 + 0), LOAD32LE(nonce_96 + 4), LOAD32LE(nonce_96 + 8)
   };
   uint32_t work[16];
   auto CHACHA_BLOCK = [&]() {
      for (size_t i = 0; i < 16; i++) work[i] = state[i];
      for (size_t i = 0; i < 10; i++) {
         QROUND(work[0], work[4], work[8], work[12]); QROUND(work[1], work[5], work[9], work[13]);
         QROUND(work[2], work[6], work[10], work[14]); QROUND(work[3], work[7], work[11], work[15]);
         QROUND(work[0], work[5], work[10], work[15]); QROUND(work[1], work[6], work[11], work[12]);
         QROUND(work[2], work[7], work[8], work[13]); QROUND(work[3], work[4], work[9], work[14]);
      }
      for (size_t i = 0; i < 16; i++) work[i] += state[i];
      state[12]++;
   };
   CHACHA_BLOCK();
   uint32_t r0 = work[0] & 0x0fffffff, r1 = work[1] & 0x0ffffffc, r2 = work[2] & 0x0ffffffc, r3 = work[3] & 0x0ffffffc;
   uint32_t s0 = work[4], s1 = work[5], s2 = work[6], s3 = work[7];
   uint32_t h0 = 0, h1 = 0, h2 = 0, h3 = 0, h4 = 0;
   auto POLY_BLOCK = [&](const uint8_t* blk, size_t blk_len) {
      uint32_t c0 = 0, c1 = 0, c2 = 0, c3 = 0, pad = 0;
      if (blk_len == 16) {
         c0 = LOAD32LE(blk); c1 = LOAD32LE(blk + 4); c2 = LOAD32LE(blk + 8); c3 = LOAD32LE(blk + 12); pad = 1;
      }
      else {
         uint8_t tmp[16] = {0};
         for (size_t i = 0; i < blk_len; i++) tmp[i] = blk[i];
         tmp[blk_len] = 1;
         c0 = LOAD32LE(tmp); c1 = LOAD32LE(tmp + 4); c2 = LOAD32LE(tmp + 8); c3 = LOAD32LE(tmp + 12);
      }
      uint64_t t0 = static_cast<uint64_t>(h0) + c0; h0 = static_cast<uint32_t>(t0);
      uint64_t t1 = static_cast<uint64_t>(h1) + c1 + (t0 >> 32); h1 = static_cast<uint32_t>(t1);
      uint64_t t2 = static_cast<uint64_t>(h2) + c2 + (t1 >> 32); h2 = static_cast<uint32_t>(t2);
      uint64_t t3 = static_cast<uint64_t>(h3) + c3 + (t2 >> 32); h3 = static_cast<uint32_t>(t3);
      uint64_t t4 = static_cast<uint64_t>(h4) + pad + (t3 >> 32); h4 = static_cast<uint32_t>(t4);
      uint64_t d0 = static_cast<uint64_t>(h0) * r0 + static_cast<uint64_t>(h1) * (5 * r3) + static_cast<uint64_t>(h2) * (5 * r2) + static_cast<uint64_t>(h3) * (5 * r1);
      uint64_t d1 = static_cast<uint64_t>(h0) * r1 + static_cast<uint64_t>(h1) * r0 + static_cast<uint64_t>(h2) * (5 * r3) + static_cast<uint64_t>(h3) * (5 * r2) + static_cast<uint64_t>(h4) * (5 * r1);
      uint64_t d2 = static_cast<uint64_t>(h0) * r2 + static_cast<uint64_t>(h1) * r1 + static_cast<uint64_t>(h2) * r0 + static_cast<uint64_t>(h3) * (5 * r3) + static_cast<uint64_t>(h4) * (5 * r2);
      uint64_t d3 = static_cast<uint64_t>(h0) * r3 + static_cast<uint64_t>(h1) * r2 + static_cast<uint64_t>(h2) * r1 + static_cast<uint64_t>(h3) * r0 + static_cast<uint64_t>(h4) * (5 * r3);
      uint64_t d4 = static_cast<uint64_t>(h4) * r0;
      uint64_t c = d0 >> 32; h0 = static_cast<uint32_t>(d0);
      d1 += c; c = d1 >> 32; h1 = static_cast<uint32_t>(d1);
      d2 += c; c = d2 >> 32; h2 = static_cast<uint32_t>(d2);
      d3 += c; c = d3 >> 32; h3 = static_cast<uint32_t>(d3);
      d4 += c; c = d4 >> 32; h4 = static_cast<uint32_t>(d4);
      uint64_t h0_c = static_cast<uint64_t>(h0) + (c * 5); 
      h0 = static_cast<uint32_t>(h0_c); 
      h1 += static_cast<uint32_t>(h0_c >> 32);
   };
   for (size_t i = 0; i < length; i += 64) {
      CHACHA_BLOCK();
      size_t chunk = length - i < 64 ? length - i : 64;
      for (size_t j = 0; j < chunk; j++) buffer[i + j] ^= static_cast<uint8_t>(work[j / 4] >> ((j % 4) * 8));
   }
   for (size_t i = 0; i < length; i += 16) POLY_BLOCK(buffer + i, length - i < 16 ? length - i : 16);
   uint8_t len_block[16] = {0};
   uint64_t byte_len = static_cast<uint64_t>(length);
   len_block[8] = static_cast<uint8_t>(byte_len >> 0); len_block[9] = static_cast<uint8_t>(byte_len >> 8);
   len_block[10] = static_cast<uint8_t>(byte_len >> 16); len_block[11] = static_cast<uint8_t>(byte_len >> 24);
   len_block[12] = static_cast<uint8_t>(byte_len >> 32); len_block[13] = static_cast<uint8_t>(byte_len >> 40);
   len_block[14] = static_cast<uint8_t>(byte_len >> 48); len_block[15] = static_cast<uint8_t>(byte_len >> 56);
   POLY_BLOCK(len_block, 16);
   uint32_t c_fin = h4 >> 2; 
   h4 &= 3;
   uint64_t sum0 = static_cast<uint64_t>(h0) + (c_fin * 5); h0 = static_cast<uint32_t>(sum0);
   uint64_t sum1 = static_cast<uint64_t>(h1) + (sum0 >> 32); h1 = static_cast<uint32_t>(sum1);
   uint64_t sum2 = static_cast<uint64_t>(h2) + (sum1 >> 32); h2 = static_cast<uint32_t>(sum2);
   uint64_t sum3 = static_cast<uint64_t>(h3) + (sum2 >> 32); h3 = static_cast<uint32_t>(sum3);
   h4 += static_cast<uint32_t>(sum3 >> 32);
   uint64_t g0 = static_cast<uint64_t>(h0) + 5;
   uint64_t g1 = static_cast<uint64_t>(h1) + (g0 >> 32); g0 &= 0xFFFFFFFF;
   uint64_t g2 = static_cast<uint64_t>(h2) + (g1 >> 32); g1 &= 0xFFFFFFFF;
   uint64_t g3 = static_cast<uint64_t>(h3) + (g2 >> 32); g2 &= 0xFFFFFFFF;
   uint64_t g4 = static_cast<uint64_t>(h4) + (g3 >> 32); g3 &= 0xFFFFFFFF;
   uint32_t mask = ~(static_cast<uint32_t>(g4 >> 2) - 1);
   h0 = (h0 & ~mask) | (static_cast<uint32_t>(g0) & mask);
   h1 = (h1 & ~mask) | (static_cast<uint32_t>(g1) & mask);
   h2 = (h2 & ~mask) | (static_cast<uint32_t>(g2) & mask);
   h3 = (h3 & ~mask) | (static_cast<uint32_t>(g3) & mask);
   uint64_t f0 = static_cast<uint64_t>(h0) + s0; h0 = static_cast<uint32_t>(f0);
   uint64_t f1 = static_cast<uint64_t>(h1) + s1 + (f0 >> 32); h1 = static_cast<uint32_t>(f1);
   uint64_t f2 = static_cast<uint64_t>(h2) + s2 + (f1 >> 32); h2 = static_cast<uint32_t>(f2);
   uint64_t f3 = static_cast<uint64_t>(h3) + s3 + (f2 >> 32); h3 = static_cast<uint32_t>(f3);
   STORE32LE(out_mac_tag_128 + 0, h0);
   STORE32LE(out_mac_tag_128 + 4, h1);
   STORE32LE(out_mac_tag_128 + 8, h2);
   STORE32LE(out_mac_tag_128 + 12, h3);
}

[[nodiscard]] inline bool DecryptChaCha20Poly1305(uint8_t* __restrict buffer, size_t length, const uint8_t* __restrict key_256, const uint8_t* __restrict nonce_96, const uint8_t* __restrict mac_tag_128) {
   auto LOAD32LE = [](const uint8_t* p) -> uint32_t { return (static_cast<uint32_t>(p[0]) << 0) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24); };
   auto STORE32LE = [](uint8_t* p, uint32_t v) -> void { p[0] = static_cast<uint8_t>(v >> 0); p[1] = static_cast<uint8_t>(v >> 8); p[2] = static_cast<uint8_t>(v >> 16); p[3] = static_cast<uint8_t>(v >> 24); };
   auto ROTL = [](uint32_t x, size_t n) -> uint32_t { return (x << n) | (x >> (32 - n)); };
   auto QROUND = [&](uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) -> void {
      a += b; d ^= a; d = ROTL(d, 16); c += d; b ^= c; b = ROTL(b, 12);
      a += b; d ^= a; d = ROTL(d, 8); c += d; b ^= c; b = ROTL(b, 7);
   };
   uint32_t state[16] = {
      0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
      LOAD32LE(key_256 + 0), LOAD32LE(key_256 + 4), LOAD32LE(key_256 + 8), LOAD32LE(key_256 + 12),
      LOAD32LE(key_256 + 16), LOAD32LE(key_256 + 20), LOAD32LE(key_256 + 24), LOAD32LE(key_256 + 28),
      0, LOAD32LE(nonce_96 + 0), LOAD32LE(nonce_96 + 4), LOAD32LE(nonce_96 + 8)
   };
   uint32_t work[16];
   auto CHACHA_BLOCK = [&]() -> void {
      for (size_t i = 0; i < 16; i++) work[i] = state[i];
      for (size_t i = 0; i < 10; i++) {
         QROUND(work[0], work[4], work[8], work[12]); QROUND(work[1], work[5], work[9], work[13]);
         QROUND(work[2], work[6], work[10], work[14]); QROUND(work[3], work[7], work[11], work[15]);
         QROUND(work[0], work[5], work[10], work[15]); QROUND(work[1], work[6], work[11], work[12]);
         QROUND(work[2], work[7], work[8], work[13]); QROUND(work[3], work[4], work[9], work[14]);
      }
      for (size_t i = 0; i < 16; i++) work[i] += state[i];
      state[12]++;
   };
   CHACHA_BLOCK();
   uint32_t r0 = work[0] & 0x0fffffff, r1 = work[1] & 0x0ffffffc, r2 = work[2] & 0x0ffffffc, r3 = work[3] & 0x0ffffffc;
   uint32_t s0 = work[4], s1 = work[5], s2 = work[6], s3 = work[7];
   uint32_t h0 = 0, h1 = 0, h2 = 0, h3 = 0, h4 = 0;
   auto POLY_BLOCK = [&](const uint8_t* blk, size_t blk_len) -> void {
      uint32_t c0 = 0, c1 = 0, c2 = 0, c3 = 0, pad = 0;
      if (blk_len == 16) {
         c0 = LOAD32LE(blk); c1 = LOAD32LE(blk + 4); c2 = LOAD32LE(blk + 8); c3 = LOAD32LE(blk + 12); pad = 1;
      } else {
         uint8_t tmp[16] = {0};
         for (size_t i = 0; i < blk_len; i++) tmp[i] = blk[i];
         tmp[blk_len] = 1;
         c0 = LOAD32LE(tmp); c1 = LOAD32LE(tmp + 4); c2 = LOAD32LE(tmp + 8); c3 = LOAD32LE(tmp + 12);
      }
      uint64_t t0 = static_cast<uint64_t>(h0) + c0; h0 = static_cast<uint32_t>(t0);
      uint64_t t1 = static_cast<uint64_t>(h1) + c1 + (t0 >> 32); h1 = static_cast<uint32_t>(t1);
      uint64_t t2 = static_cast<uint64_t>(h2) + c2 + (t1 >> 32); h2 = static_cast<uint32_t>(t2);
      uint64_t t3 = static_cast<uint64_t>(h3) + c3 + (t2 >> 32); h3 = static_cast<uint32_t>(t3);
      uint64_t t4 = static_cast<uint64_t>(h4) + pad + (t3 >> 32); h4 = static_cast<uint32_t>(t4);
      uint64_t d0 = static_cast<uint64_t>(h0) * r0 + static_cast<uint64_t>(h1) * (5 * r3) + static_cast<uint64_t>(h2) * (5 * r2) + static_cast<uint64_t>(h3) * (5 * r1);
      uint64_t d1 = static_cast<uint64_t>(h0) * r1 + static_cast<uint64_t>(h1) * r0 + static_cast<uint64_t>(h2) * (5 * r3) + static_cast<uint64_t>(h3) * (5 * r2) + static_cast<uint64_t>(h4) * (5 * r1);
      uint64_t d2 = static_cast<uint64_t>(h0) * r2 + static_cast<uint64_t>(h1) * r1 + static_cast<uint64_t>(h2) * r0 + static_cast<uint64_t>(h3) * (5 * r3) + static_cast<uint64_t>(h4) * (5 * r2);
      uint64_t d3 = static_cast<uint64_t>(h0) * r3 + static_cast<uint64_t>(h1) * r2 + static_cast<uint64_t>(h2) * r1 + static_cast<uint64_t>(h3) * r0 + static_cast<uint64_t>(h4) * (5 * r3);
      uint64_t d4 = static_cast<uint64_t>(h4) * r0;
      uint64_t c = d0 >> 32; h0 = static_cast<uint32_t>(d0);
      d1 += c; c = d1 >> 32; h1 = static_cast<uint32_t>(d1);
      d2 += c; c = d2 >> 32; h2 = static_cast<uint32_t>(d2);
      d3 += c; c = d3 >> 32; h3 = static_cast<uint32_t>(d3);
      d4 += c; c = d4 >> 32; h4 = static_cast<uint32_t>(d4);
      uint64_t h0_c = static_cast<uint64_t>(h0) + (c * 5); 
      h0 = static_cast<uint32_t>(h0_c); 
      h1 += static_cast<uint32_t>(h0_c >> 32);
   };
   for (size_t i = 0; i < length; i += 16) POLY_BLOCK(buffer + i, length - i < 16 ? length - i : 16);
   uint8_t len_block[16] = {0};
   uint64_t byte_len = static_cast<uint64_t>(length);
   len_block[8] = static_cast<uint8_t>(byte_len >> 0); len_block[9] = static_cast<uint8_t>(byte_len >> 8);
   len_block[10] = static_cast<uint8_t>(byte_len >> 16); len_block[11] = static_cast<uint8_t>(byte_len >> 24);
   len_block[12] = static_cast<uint8_t>(byte_len >> 32); len_block[13] = static_cast<uint8_t>(byte_len >> 40);
   len_block[14] = static_cast<uint8_t>(byte_len >> 48); len_block[15] = static_cast<uint8_t>(byte_len >> 56);
   POLY_BLOCK(len_block, 16);
   uint32_t c_fin = h4 >> 2; 
   h4 &= 3;
   uint64_t sum0 = static_cast<uint64_t>(h0) + (c_fin * 5); h0 = static_cast<uint32_t>(sum0);
   uint64_t sum1 = static_cast<uint64_t>(h1) + (sum0 >> 32); h1 = static_cast<uint32_t>(sum1);
   uint64_t sum2 = static_cast<uint64_t>(h2) + (sum1 >> 32); h2 = static_cast<uint32_t>(sum2);
   uint64_t sum3 = static_cast<uint64_t>(h3) + (sum2 >> 32); h3 = static_cast<uint32_t>(sum3);
   h4 += static_cast<uint32_t>(sum3 >> 32);
   uint64_t g0 = static_cast<uint64_t>(h0) + 5;
   uint64_t g1 = static_cast<uint64_t>(h1) + (g0 >> 32); g0 &= 0xFFFFFFFF;
   uint64_t g2 = static_cast<uint64_t>(h2) + (g1 >> 32); g1 &= 0xFFFFFFFF;
   uint64_t g3 = static_cast<uint64_t>(h3) + (g2 >> 32); g2 &= 0xFFFFFFFF;
   uint64_t g4 = static_cast<uint64_t>(h4) + (g3 >> 32); g3 &= 0xFFFFFFFF;
   uint32_t mask = ~(static_cast<uint32_t>(g4 >> 2) - 1);
   h0 = (h0 & ~mask) | (static_cast<uint32_t>(g0) & mask);
   h1 = (h1 & ~mask) | (static_cast<uint32_t>(g1) & mask);
   h2 = (h2 & ~mask) | (static_cast<uint32_t>(g2) & mask);
   h3 = (h3 & ~mask) | (static_cast<uint32_t>(g3) & mask);
   uint64_t f0 = static_cast<uint64_t>(h0) + s0; h0 = static_cast<uint32_t>(f0);
   uint64_t f1 = static_cast<uint64_t>(h1) + s1 + (f0 >> 32); h1 = static_cast<uint32_t>(f1);
   uint64_t f2 = static_cast<uint64_t>(h2) + s2 + (f1 >> 32); h2 = static_cast<uint32_t>(f2);
   uint64_t f3 = static_cast<uint64_t>(h3) + s3 + (f2 >> 32); h3 = static_cast<uint32_t>(f3);
   uint8_t computed_mac[16];
   STORE32LE(computed_mac + 0, h0); 
   STORE32LE(computed_mac + 4, h1);
   STORE32LE(computed_mac + 8, h2); 
   STORE32LE(computed_mac + 12, h3);
   uint32_t diff = 0;
   for (size_t i = 0; i < 16; i++) diff |= computed_mac[i] ^ mac_tag_128[i];
   if (diff != 0) {
      for (size_t i = 0; i < length; i++) buffer[i] = 0;
      return false;
   }
   for (size_t i = 0; i < length; i += 64) {
      CHACHA_BLOCK();
      size_t chunk = length - i < 64 ? length - i : 64;
      for (size_t j = 0; j < chunk; j++) buffer[i + j] ^= static_cast<uint8_t>(work[j >> 2] >> ((j % 4) << 3));
   }
   return true;
}

inline void Security::InitAES_GCM(const uint8_t* key_256, AES_GCM_Context& ctx) {
   static constexpr uint8_t RCON[7] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40 };
   for (size_t i = 0; i < 32; i++) ctx.rk[i] = key_256[i];
   for (size_t i = 32; i < 240; i += 4) {
      uint8_t temp[4] = { ctx.rk[i - 4], ctx.rk[i - 3], ctx.rk[i - 2], ctx.rk[i - 1] };
      if (i % 32 == 0) {
            uint8_t t = temp[0];
            temp[0] = SBOX[temp[1]] ^ RCON[(i / 32) - 1];
            temp[1] = SBOX[temp[2]]; temp[2] = SBOX[temp[3]]; temp[3] = SBOX[t];
      } else if (i % 32 == 16) {
            temp[0] = SBOX[temp[0]]; temp[1] = SBOX[temp[1]];
            temp[2] = SBOX[temp[2]]; temp[3] = SBOX[temp[3]];
      }
      for (size_t j = 0; j < 4; j++) ctx.rk[i + j] = ctx.rk[i - 32 + j] ^ temp[j];
   }
   auto MUL2 = [](uint8_t x) -> uint8_t { return (x << 1) ^ ((x & 0x80) ? 0x1B : 0x00); };
   uint8_t state[16] = {0};
   for (size_t i = 0; i < 16; i++) state[i] ^= ctx.rk[i];
   for (size_t round = 1; round < 14; round++) {
      for (size_t i = 0; i < 16; i++) state[i] = SBOX[state[i]];
      uint8_t t = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = t;
      t = state[2]; state[2] = state[10]; state[10] = t; t = state[6]; state[6] = state[14]; state[14] = t;
      t = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = t;
      for (size_t i = 0; i < 16; i += 4) {
            uint8_t s0 = state[i], s1 = state[i + 1], s2 = state[i + 2], s3 = state[i + 3];
            state[i]     = MUL2(s0 ^ s1) ^ s1 ^ s2 ^ s3;
            state[i + 1] = MUL2(s1 ^ s2) ^ s0 ^ s2 ^ s3;
            state[i + 2] = MUL2(s2 ^ s3) ^ s0 ^ s1 ^ s3;
            state[i + 3] = MUL2(s3 ^ s0) ^ s0 ^ s1 ^ s2;
      }
      for (size_t i = 0; i < 16; i++) state[i] ^= ctx.rk[(round * 16) + i];
   }
   for (size_t i = 0; i < 16; i++) state[i] = SBOX[state[i]];
   uint8_t t = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = t;
   t = state[2]; state[2] = state[10]; state[10] = t; t = state[6]; state[6] = state[14]; state[14] = t;
   t = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = t;
   for (size_t i = 0; i < 16; i++) ctx.H[i] = state[i] ^ ctx.rk[224 + i];
}

[[nodiscard]] inline bool Security::ProcessAES_GCM(uint8_t* __restrict buffer, size_t length, const AES_GCM_Context& ctx, const uint8_t* __restrict iv_96, uint8_t* __restrict in_out_mac_tag, bool is_encrypting) {
   auto MUL2 = [](uint8_t x) -> uint8_t { return (x << 1) ^ ((x & 0x80) ? 0x1B : 0x00); };
   auto AES_BLOCK = [&](const uint8_t* in, uint8_t* out) {
      uint8_t state[16];
      for (size_t i = 0; i < 16; i++) state[i] = in[i] ^ ctx.rk[i];
      for (size_t round = 1; round < 14; round++) {
         for (size_t i = 0; i < 16; i++) state[i] = SBOX[state[i]];
         uint8_t t = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = t;
         t = state[2]; state[2] = state[10]; state[10] = t; t = state[6]; state[6] = state[14]; state[14] = t;
         t = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = t;
         for (size_t i = 0; i < 16; i += 4) {
            uint8_t s0 = state[i], s1 = state[i + 1], s2 = state[i + 2], s3 = state[i + 3];
            state[i]     = MUL2(s0 ^ s1) ^ s1 ^ s2 ^ s3;
            state[i + 1] = MUL2(s1 ^ s2) ^ s0 ^ s2 ^ s3;
            state[i + 2] = MUL2(s2 ^ s3) ^ s0 ^ s1 ^ s3;
            state[i + 3] = MUL2(s3 ^ s0) ^ s0 ^ s1 ^ s2;
         }
         for (size_t i = 0; i < 16; i++) state[i] ^= ctx.rk[(round * 16) + i];
      }
      for (size_t i = 0; i < 16; i++) state[i] = SBOX[state[i]];
      uint8_t t = state[1]; state[1] = state[5]; state[5] = state[9]; state[9] = state[13]; state[13] = t;
      t = state[2]; state[2] = state[10]; state[10] = t; t = state[6]; state[6] = state[14]; state[14] = t;
      t = state[15]; state[15] = state[11]; state[11] = state[7]; state[7] = state[3]; state[3] = t;
      for (size_t i = 0; i < 16; i++) out[i] = state[i] ^ ctx.rk[224 + i];
   };
   auto GF_MULT = [](const uint8_t* x, const uint8_t* y, uint8_t* out) -> void {
      uint8_t Z[16] = {0}, V[16];
      for (size_t i = 0; i < 16; i++) V[i] = y[i];
      for (size_t i = 0; i < 128; i++) {
         if (x[i / 8] & (1 << (7 - (i % 8)))) for (size_t j = 0; j < 16; j++) Z[j] ^= V[j];
         uint8_t lsb = V[15] & 1;
         for (size_t j = 15; j > 0; j--) V[j] = (V[j] >> 1) | ((V[j - 1] & 1) << 7);
         V[0] >>= 1;
         if (lsb) V[0] ^= 0xE1;
      }
      for (size_t i = 0; i < 16; i++) out[i] = Z[i];
   };
   uint8_t J[16] = {0};
   for (size_t i = 0; i < 12; i++) J[i] = iv_96[i];
   J[15] = 1;
   uint8_t tag_state[16] = {0};
   auto GHASH_UPDATE = [&](const uint8_t* block, size_t blk_len) -> void {
      uint8_t temp[16] = {0};
      for (size_t i = 0; i < blk_len; i++) temp[i] = block[i];
      for (size_t i = 0; i < 16; i++) tag_state[i] ^= temp[i];
      uint8_t next_state[16];
      GF_MULT(tag_state, ctx.H.data(), next_state);
      for (size_t i = 0; i < 16; i++) tag_state[i] = next_state[i];
   };
   if (!is_encrypting) for (size_t i = 0; i < length; i += 16) GHASH_UPDATE(buffer + i, (length - i < 16) ? length - i : 16);
   uint8_t counter[16];
   for (size_t i = 0; i < 16; i++) counter[i] = J[i];
   for (size_t i = 0; i < length; i += 16) {
      for (int j = 15; j >= 12; j--) if (++counter[j]) break;
      uint8_t keystream[16];
      AES_BLOCK(counter, keystream);
      size_t chunk = (length - i < 16) ? length - i : 16;
      for (size_t j = 0; j < chunk; j++) buffer[i + j] ^= keystream[j];
   }
   if (is_encrypting) for (size_t i = 0; i < length; i += 16) GHASH_UPDATE(buffer + i, (length - i < 16) ? length - i : 16);
   uint8_t len_block[16] = {0};
   uint64_t bit_len = static_cast<uint64_t>(length) << 3;
   for (size_t i = 0; i < 8; i++) len_block[15 - i] = static_cast<uint8_t>((bit_len >> (i << 3)) & 0xFF);
   GHASH_UPDATE(len_block, 16);
   uint8_t J0_enc[16];
   AES_BLOCK(J, J0_enc);
   uint8_t computed_mac[16];
   for (size_t i = 0; i < 16; i++) computed_mac[i] = tag_state[i] ^ J0_enc[i];
   if (is_encrypting) {
      for (size_t i = 0; i < 16; i++) in_out_mac_tag[i] = computed_mac[i];
      return true;
   }
   else {
      uint32_t diff = 0;
      for (size_t i = 0; i < 16; i++) diff |= computed_mac[i] ^ in_out_mac_tag[i];
      if (diff != 0) {
         for (size_t i = 0; i < length; i++) buffer[i] = 0;
         return false;
      }
      return true;
   }
}

[[nodiscard]] inline bool Security::EmbedPayloadLSB(int16_t* __restrict audio_buffer, size_t audio_frames, const uint8_t* __restrict payload, size_t payload_bytes){
   if (payload_bytes * 8 > audio_frames) return false;
   for (size_t i = 0; i < payload_bytes; i++) for (uint8_t bit = 0; bit < 8; bit++) audio_buffer[i * 8 + bit] = (audio_buffer[i * 8 + bit] & ~1) | (1 & (payload[i] >> bit));
   return true;
}

[[nodiscard]] inline bool Security::ExtractPayloadLSB(const int16_t* __restrict audio_buffer, size_t audio_frames, uint8_t* __restrict out_payload, size_t expected_bytes){
   if (expected_bytes * 8 > audio_frames) return false;
   for (size_t i = 0; i < expected_bytes; i++) {
      out_payload[i] = 0;
      for (uint8_t bit = 0; bit < 8; bit++) out_payload[i] |= (audio_buffer[i * 8 + bit] & 1) << bit;
   }
   return true;
}

inline void Security::CalculateSHA256(const uint8_t* __restrict buffer, size_t length, uint8_t* __restrict out_hash_256) {
   uint32_t H0 = 0x6a09e667; uint32_t H1 = 0xbb67ae85; uint32_t H2 = 0x3c6ef372; uint32_t H3 = 0xa54ff53a;
   uint32_t H4 = 0x510e527f; uint32_t H5 = 0x9b05688c; uint32_t H6 = 0x1f83d9ab; uint32_t H7 = 0x5be0cd19;
   const uint32_t K[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
   };
   size_t total_len = length + 9;
   size_t pad_zeros = (64 - (total_len % 64)) % 64;
   size_t extra_bytes = 1 + pad_zeros + 8;
   uint8_t tail[128] = {0x80};
   uint64_t bit_len = static_cast<uint64_t>(length) << 3;
   for (int i = 0; i < 8; i++) tail[extra_bytes - 1 - i] = (bit_len >> (i << 3)) & 0xFF;
   auto ROTR = [](uint32_t x, size_t n) -> uint32_t { return (x >> n) | (x << (32 - n)); };
   for (size_t i = 0; i < length + extra_bytes; i += 64) {
      uint32_t W[64] = {0};
      for (size_t j = 0; j < 16; j++) {
         size_t base = i + (j * 4);
         const uint8_t b0 = base + 0 < length ? buffer[base + 0] : tail[base + 0 - length];
         const uint8_t b1 = base + 1 < length ? buffer[base + 1] : tail[base + 1 - length];
         const uint8_t b2 = base + 2 < length ? buffer[base + 2] : tail[base + 2 - length];
         const uint8_t b3 = base + 3 < length ? buffer[base + 3] : tail[base + 3 - length];
         W[j] = (static_cast<uint32_t>(b0) << 24) | (static_cast<uint32_t>(b1) << 16) | (static_cast<uint32_t>(b2) << 8) | (static_cast<uint32_t>(b3) << 0);
      }
      for (size_t j = 16; j < 64; j++) {
         uint32_t s0 = ROTR(W[j - 15], 7) ^ ROTR(W[j - 15], 18) ^ (W[j - 15] >> 3);
         uint32_t s1 = ROTR(W[j - 2], 17) ^ ROTR(W[j - 2], 19) ^ (W[j - 2] >> 10);
         W[j] = W[j - 16] + s0 + W[j - 7] + s1;
      }
      auto a = H0, b = H1, c = H2, d = H3, e = H4, f = H5, g = H6, h = H7;
      for (size_t t = 0; t < 64; t++) {
         auto S1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
         auto ch = (e & f) ^ (~e & g);
         auto tmp1 = h + S1 + ch + K[t] + W[t];
         auto S0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
         auto maj = (a & b) ^ (a & c) ^ (b & c);
         auto tmp2 = S0 + maj;
         h = g; g = f; f = e; e = d + tmp1;
         d = c; c = b; b = a; a = tmp1 + tmp2;
      }
      H0 += a; H1 += b; H2 += c; H3 += d; H4 += e; H5 += f; H6 += g; H7 += h;
   }
   out_hash_256[0x00] = (H0 >> 0x18) & 0xFF; out_hash_256[0x01] = (H0 >> 0x10) & 0xFF; out_hash_256[0x02] = (H0 >> 0x08) & 0xFF; out_hash_256[0x03] = (H0 >> 0x00) & 0xFF;
   out_hash_256[0x04] = (H1 >> 0x18) & 0xFF; out_hash_256[0x05] = (H1 >> 0x10) & 0xFF; out_hash_256[0x06] = (H1 >> 0x08) & 0xFF; out_hash_256[0x07] = (H1 >> 0x00) & 0xFF;
   out_hash_256[0x08] = (H2 >> 0x18) & 0xFF; out_hash_256[0x09] = (H2 >> 0x10) & 0xFF; out_hash_256[0x0A] = (H2 >> 0x08) & 0xFF; out_hash_256[0x0B] = (H2 >> 0x00) & 0xFF;
   out_hash_256[0x0C] = (H3 >> 0x18) & 0xFF; out_hash_256[0x0D] = (H3 >> 0x10) & 0xFF; out_hash_256[0x0E] = (H3 >> 0x08) & 0xFF; out_hash_256[0x0F] = (H3 >> 0x00) & 0xFF;
   out_hash_256[0x10] = (H4 >> 0x18) & 0xFF; out_hash_256[0x11] = (H4 >> 0x10) & 0xFF; out_hash_256[0x12] = (H4 >> 0x08) & 0xFF; out_hash_256[0x13] = (H4 >> 0x00) & 0xFF;
   out_hash_256[0x14] = (H5 >> 0x18) & 0xFF; out_hash_256[0x15] = (H5 >> 0x10) & 0xFF; out_hash_256[0x16] = (H5 >> 0x08) & 0xFF; out_hash_256[0x17] = (H5 >> 0x00) & 0xFF;
   out_hash_256[0x18] = (H6 >> 0x18) & 0xFF; out_hash_256[0x19] = (H6 >> 0x10) & 0xFF; out_hash_256[0x1A] = (H6 >> 0x08) & 0xFF; out_hash_256[0x1B] = (H6 >> 0x00) & 0xFF;
   out_hash_256[0x1C] = (H7 >> 0x18) & 0xFF; out_hash_256[0x1D] = (H7 >> 0x10) & 0xFF; out_hash_256[0x1E] = (H7 >> 0x08) & 0xFF; out_hash_256[0x1F] = (H7 >> 0x00) & 0xFF;
}

#endif
