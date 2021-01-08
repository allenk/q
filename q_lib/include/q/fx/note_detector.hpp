/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ONSET_DETECTOR_NOVEMBER_9_2019)
#define CYCFI_Q_ONSET_DETECTOR_NOVEMBER_9_2019

#include <q/fx/envelope.hpp>
#include <q/fx/special.hpp>
#include <q/fx/feature_detection.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/biquad.hpp>
#include <q/fx/taper.hpp>
#include <q/fx/median.hpp>

namespace cycfi::q
{
   struct note_detector
   {
      static constexpr auto compressor_makeup_gain = float(12_dB);
      static constexpr auto compressor_threshold = -12_dB;
      static constexpr auto compressor_slope = 1.0/8;
      static constexpr auto attack = 2_ms;

      struct post_processor
      {
         static constexpr auto blank_duration = 50_ms;
         static constexpr auto min_threshold = float(-27_dB);

         post_processor(std::uint32_t sps)
          : _taper{ 80_ms, sps }
          , _blank{ blank_duration, sps }
          , _peak_env{150_ms, sps }
         {
         }

         void process_thresholds(float& attack, float& decay, bool gate)
         {
            if (!gate)
            {
               attack = decay = 0.0f;
            }
            else
            {
               _threshold = std::max(min_threshold, _peak_env() * float(-21_dB));
               if (decay > -_threshold)
                  decay = 0.0f;
               if (attack < _threshold)
                  attack = 0.0f;
            }
         }

         void process_attack(float& attack, float& decay, bool gate)
         {
            if (!gate)
            {
               _blank.stop();
               _peak_env.y = _peak = attack = 0;
               _threshold = float(-15_dB);
            }
            else
            {
               auto post_attack_blank = _blank();
               auto start_attack = _blank(attack > _threshold);

               if (post_attack_blank)  // Are we already blanked, post-attack?
               {
                  if (attack > _peak * float(3_dB))
                  {
                     // If we got a much stronger pulse, restart blank
                     _blank.start();
                     _peak = attack;
                  }
                  else
                  {
                     attack = 0.0f;
                  }
                  decay = 0.0f;
               }
               else
               {
                  if (start_attack)   // Start of attack
                  {
                     _blank.start();
                     decay = 0.0f;
                  }
                  else
                  {
                     _peak = 0.0f;
                  }
               }
               // Update peak hold
               _peak_env(_peak);
            }
         }

         std::pair<float, bool>
         operator()(float attack, float decay, bool sync, bool gate)
         {
            attack = std::max(attack, -decay);
            process_thresholds(attack, decay, gate);
            process_attack(attack, decay, gate);
            return { attack + decay, sync };
         }

         using pulse = monostable;

         blackman_window         _taper;
         pulse                   _blank;
         float                   _peak = 0.0f;
         peak_envelope_follower  _peak_env;
         float                   _threshold;
      };

      note_detector(duration hold, std::uint32_t sps)
       : _lp{ frequency{ 4/float(hold) }, sps }
       , _integ{ 0.004f/float(hold) }
       , _integ_env1{ hold, sps }
       , _integ_env2{ hold, sps }
       , _comp_env{ attack, hold * 30, sps }
       , _comp{ compressor_threshold, compressor_slope }
       , _pp{ sps }
      {}

      std::pair<float, bool> operator()(float s, bool gate)
      {
         s = _integ(_lp(s));

         // Compressor
         auto comp_env = decibel{_comp_env(std::abs(s)) };
         auto comp_gain = float(_comp(comp_env)) * compressor_makeup_gain;
         s *= comp_gain;

         // Envelope Follower
         auto e1 = _integ_env1(s);
         auto e2 = _integ_env2(-s);
         auto sync = e1.second;

         // Differentiator
         auto d1 = _diff1(e1.first);
         auto d2 = _diff2(e2.first);

         auto pos_d1 = std::max(0.0f, d1);
         auto pos_d2 = std::max(0.0f, d2);
         auto neg_d1 = std::min(0.0f, d1);
         auto neg_d2 = std::min(0.0f, d2);

         auto attack = gate ? pos_d1 + pos_d2 : 0.0f;
         auto decay = gate ? neg_d1 + neg_d2 : 0.0f;
         return _pp(attack, decay, sync, gate);
      }

      lowpass                 _lp;
      integrator              _integ;
      peak_hold               _integ_env1;
      peak_hold               _integ_env2;

      envelope_follower       _comp_env;
      compressor              _comp;

      central_difference      _diff1{};
      central_difference      _diff2{};

      post_processor          _pp;
   };
}

#endif
