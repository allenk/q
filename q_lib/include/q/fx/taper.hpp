/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TAPER_DECEMBER_24_2020)
#define CYCFI_Q_TAPER_DECEMBER_24_2020

#include <q/support/base.hpp>
#include <vector>

namespace cycfi::q
{
   struct taper_window
   {
      void reset()
      {
         _i = 0;
      }

      bool done() const
      {
         return _i >= _w.size();
      }

      float operator()(float s)
      {
         if (done())
            return 0.0f;
         return _w[_i++] * s;
      }

      std::vector<float>   _w;
      std::size_t          _i = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // hamming_window: Hamming window taper:
   ////////////////////////////////////////////////////////////////////////////
   struct hamming_window : taper_window
   {
      hamming_window(duration width, std::uint32_t sps)
      {
         config(width, sps);
      }

      void config(duration width, std::uint32_t sps)
      {
         _w.resize((float(width) * sps) + 1);
         for (auto i = 0; i != _w.size(); ++i)
            _w[i] = 0.54 - 0.46 * std::cos(2_pi*i/_w.size());
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // blackman_window: Blackman window taper:
   ////////////////////////////////////////////////////////////////////////////
   struct blackman_window : taper_window
   {
      blackman_window(duration width, std::uint32_t sps)
      {
         config(width, sps);
      }

      void config(duration width, std::uint32_t sps)
      {
         _w.resize((float(width) * sps) + 1);
         for (auto i = 0; i != _w.size(); ++i)
            _w[i] = 0.42 - 0.5 * std::cos(2_pi*i/_w.size()) + 0.08 * cos(4_pi*i/_w.size());
      }
   };
}

#endif
