/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q_io/audio_file.hpp>
#include <q/fx/note_detector.hpp>
#include <q/fx/special.hpp>
#include <q/fx/taper.hpp>
#include <vector>
#include "notes.hpp"

namespace q = cycfi::q;
using namespace q::literals;
using namespace notes;

// Commment this out for testing and diagnostics
#define ND_DEMO

void process(
   std::string name, std::vector<float> const& in
 , std::uint32_t sps, q::frequency f)
{;
   q::duration hold = f.period() * 1.1;
#if defined(ND_DEMO)
   constexpr auto n_channels = 2;
#else
   constexpr auto n_channels = 6;
#endif
   std::vector<float> out(in.size() * n_channels);

   ////////////////////////////////////////////////////////////////////////////
   // Note detection

   static constexpr auto release = 50_ms;
   static constexpr auto gate_attack_threshold = -20_dB;
   static constexpr auto gate_release_threshold = -36_dB;

   auto i = out.begin();

   auto _note = q::note_detector{hold, sps};
   auto _main_env = q::peak_envelope_follower{release, sps};
   auto _gate = q::window_comparator{gate_release_threshold, gate_attack_threshold};
   auto _gate_env = q::envelope_follower{500_us, 1_ms, sps};
   auto _dc_blk = q::dc_block{f, sps};

   auto _edge = q::rising_edge{};
   auto _taper = q::blackman_window{100_ms, sps};
   auto _taper1 = q::window_iterator{_taper};
   auto _taper2 = q::window_iterator{_taper};

   for (auto s : in)
   {
      auto raw = s;
      auto main_env = _main_env(s);

      // Noise gate
      auto gate = _gate(main_env);
      s *= _gate_env(gate);

      // DC Block
      s = _dc_blk(s);

      // Note detection
      auto note = _note(s, gate);

      if (note.attack > 1.0f)
         note.attack = 1.0f;
      if (note.decay > 1.0f)
         note.decay = 1.0f;

      *i++ = raw;
      if (_edge(_note.onset()))
      {
         if (!_taper1.running())
            _taper1.start();
         else
            _taper2.start();
      }
      *i++ = (_taper1() + _taper2()) * raw;

#if !defined(ND_DEMO)
      *i++ = _note._integ_env1() / 3;
      *i++ = _note._integ_env2() / 3;
      *i++ = _note._hp_env() / 3;
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


