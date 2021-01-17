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
#include <q/fx/median.hpp>

namespace cycfi::q
{
   struct note_detector
   {
      static constexpr auto compressor_makeup_gain = float(12_dB);
      static constexpr auto compressor_threshold = -12_dB;
      static constexpr auto compressor_slope = 1.0/8;
      static constexpr auto attack = 2_ms;

      struct info
      {
         float    attack;
         float    decay;
         bool     ready;
      };

      struct post_processor
      {
         static constexpr auto blank_duration = 50_ms;
         static constexpr auto min_threshold = float(-27_dB);
         static constexpr auto decay_threshold = float(-36_dB);
         static constexpr auto high_freq_weight = float(-3_dB);

         post_processor(std::uint32_t sps)
          : _blank{blank_duration, sps}
          , _peak_env{150_ms, sps}
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
               if (decay > -decay_threshold)
                  decay = 0.0f;
               if (attack < _threshold)
                  attack = 0.0f;
            }
         }

         void process_attack(float& attack, bool gate)
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
               }
               else
               {
                  if (start_attack)   // Start of attack
                  {
                     _blank.start();
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

         info operator()(float attack, float decay, float high_freq, bool sync, bool gate)
         {
            // Have the decay factor in the attack to take into account
            // onset on legatos.
            attack = std::max(attack, -decay);

            // Have the high frequency content factor in the attack to take
            // into account sudden changes.
            attack = std::max(attack, high_freq * high_freq_weight);

            process_thresholds(attack, decay, gate);
            process_attack(attack, gate);
            return {attack, -decay, sync};
         }

         using pulse = monostable;

         pulse                   _blank;
         float                   _peak = 0.0f;
         peak_envelope_follower  _peak_env;
         float                   _threshold;
      };

      note_detector(duration hold, std::uint32_t sps)
       : _range_lp{frequency{4 / float(hold) }, sps}
       , _top_lp{6_kHz, sps}
       , _integ{0.004f/float(hold) }
       , _integ_env1{hold, sps}
       , _integ_env2{hold, sps}
       , _integ_comp_env{attack, hold * 30, sps}
       , _integ_comp{compressor_threshold, compressor_slope}
       , _hp_env{hold, sps}
       , _hp_comp_env{attack, hold * 30, sps}
       , _hp_comp{compressor_threshold, compressor_slope}
       , _pp{sps}
       , _gate{sps}
       , _gate_env{500_us, 1_ms, sps}
       , _dc_blk{frequency{1.0f/float(hold)}, sps}
      {}

      info operator()(float s)
      {
         // Noise gate
         auto gate = _gate(s);
         s *= _gate_env(gate);

         // DC Block
         s = _dc_blk(s);

         // Separate the low frequency and high frequency components
         auto top_lp = _top_lp(s);           // 6 kHz lowpass
         auto hp = s - top_lp;               // 6 kHz highpass
         auto range_lp = _range_lp(top_lp);  // Note range

         // Integrate
         auto integ = _integ(range_lp);

         // Integrator Compressor
         auto integ_comp_env = decibel{_integ_comp_env(std::abs(integ))};
         auto integ_comp_gain = float(_integ_comp(integ_comp_env)) * compressor_makeup_gain;
         integ *= integ_comp_gain;

         // Highpass Compressor
         auto hp_comp_env = decibel{_hp_comp_env(std::abs(hp))};
         auto hp_comp_gain = float(_hp_comp(hp_comp_env)) * compressor_makeup_gain;
         hp *= hp_comp_gain;

         // Peak Holds
         auto e1 = _integ_env1(integ);
         auto e2 = _integ_env2(-integ);
         auto e3 = _hp_env(hp);
         auto sync = e1.second;

         // Differentiators
         auto d1 = _diff1(e1.first);
         auto d2 = _diff2(e2.first);
         auto d3 = _diff3(e3.first);

         auto pos_d1 = std::max(0.0f, d1);
         auto pos_d2 = std::max(0.0f, d2);
         auto neg_d1 = std::min(0.0f, d1);
         auto neg_d2 = std::min(0.0f, d2);

         auto attack = gate ? pos_d1 + pos_d2 : 0.0f;
         auto decay = gate ? neg_d1 + neg_d2 : 0.0f;
         auto high_freq = gate ? std::abs(d3) : 0.0f;

         return _pp(attack, decay, high_freq, sync, gate);
      }

      bool onset() const
      {
         return _pp._blank();
      }

      bool attack_envelope() const
      {
         return _pp._peak_env();
      }

      lowpass                 _range_lp;
      lowpass                 _top_lp;
      integrator              _integ;
      peak_hold               _integ_env1;
      peak_hold               _integ_env2;

      envelope_follower       _integ_comp_env;
      compressor              _integ_comp;
      envelope_follower       _hp_comp_env;
      compressor              _hp_comp;
      peak_hold               _hp_env;

      central_difference      _diff1{};
      central_difference      _diff2{};
      central_difference      _diff3{};

      post_processor          _pp;
      noise_gate              _gate;
      envelope_follower       _gate_env;
      dc_block                _dc_blk;
   };
}

#endif
