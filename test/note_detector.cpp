/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/note_detector.hpp>
#include <q/fx/special.hpp>
#include <q/synth/blackman.hpp>
#include <vector>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

// Commment this out for testing and diagnostics
//#define NO_DIAGNOSTICS

void process(
   std::string name, std::vector<float> const& in
 , std::uint32_t sps, q::frequency f)
{;
   q::duration hold = f.period() * 1.1;
#if defined(NO_DIAGNOSTICS)
   constexpr auto n_channels = 2;
#else
   constexpr auto n_channels = 6;
#endif
   std::vector<float> out(in.size() * n_channels);

   ////////////////////////////////////////////////////////////////////////////
   // Note detection

   static constexpr auto release = 50_ms;
   static constexpr auto gate_attack_threshold = -33_dB;
   static constexpr auto gate_release_threshold = -36_dB;

   auto i = out.begin();

   auto _note = q::note_detector{hold, sps};
   auto _edge = q::rising_edge{};
   auto _taper = q::blackman;
   auto _i1 = q::one_shot_phase_iterator{15_Hz, sps};
   auto _i2 = q::one_shot_phase_iterator{15_Hz, sps};

   for (auto s : in)
   {
      auto raw = s;

      // Note detection
      auto note = _note(s);

      if (note.attack > 1.0f)
         note.attack = 1.0f;
      if (note.decay > 1.0f)
         note.decay = 1.0f;

      *i++ = raw;

      if (!_note._gate())
      {
         _i1.reset_to_back();
         _i2.reset_to_back();
      }
      else if (_edge(_note.onset()))
      {
         if (_i1.last())
            _i1.reset();
         else if (_i2.last())
            _i2.reset();
      }

      auto t1 = _taper(_i1++);
      auto t2 = _taper(_i2++);
      *i++ = (t1 + t2) * raw;

#if !defined(NO_DIAGNOSTICS)

      // Inspect the gate and output envelopes
      *i++ = t1;
      *i++ = t2;
      auto gate_env = _note._gate_env();
      *i++ = gate_env * 0.8;

      // Inspect the analysis envelopes
      // *i++ = _note._integ_env1() / 3; // upper integrator
      // *i++ = _note._integ_env2() / 3; // lower integrator
      // *i++ = _note._hp_env() / 3;     // differentiator

      *i++ = _note.onset() * 0.8;
#endif
   }

   ////////////////////////////////////////////////////////////////////////////
   // Write to a wav file

   q::wav_writer wav(
      "results/onset_" + name + ".wav", n_channels, sps
   );
   wav.write(out);
}

void process(std::string name, q::frequency f)
{
   ////////////////////////////////////////////////////////////////////////////
   // Read audio file

   q::wav_reader src{"audio_files/" + name + ".wav"};
   std::uint32_t const sps = src.sps();

   std::vector<float> in(src.length());
   src.read(in);

   ////////////////////////////////////////////////////////////////////////////
   process(name, in, sps, f);
}

int main()
{
   process("1a-Low-E", low_e);
   process("1b-Low-E-12th", low_e);
   process("Tapping D", d);
   process("Hammer-Pull High E", high_e);
   process("Bend-Slide G", g);
   process("GStaccato", g);

   return 0;
}


