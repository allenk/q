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
   struct precomputed_window
   {
      float operator[](std::size_t i) const
      {
         return _w[i];
      }

      std::vector<float>   _w;
   };

   ////////////////////////////////////////////////////////////////////////////
   // hamming_window
   ////////////////////////////////////////////////////////////////////////////
   struct hamming_window : precomputed_window
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
   // blackman_window
   ////////////////////////////////////////////////////////////////////////////
   struct blackman_window : precomputed_window
   {
      blackman_window(duration width, std::uint32_t sps)
      {
         config(width, sps);
      }

      void config(duration width, std::uint32_t sps)
      {
         _w.resize((float(width) * sps) + 1);
         for (auto i = 0; i != _w.size(); ++i)
            _w[i] = 0.42 - 0.5 * std::cos(2_pi*i/_w.size()) + 0.08 * std::cos(4_pi*i/_w.size());
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // gaussian_window
   ////////////////////////////////////////////////////////////////////////////
   struct gaussian_window : precomputed_window
   {
      gaussian_window(float sigma = 2.0)
      {
         config(sigma);
      }

      gaussian_window(duration width, std::uint32_t sps)
      {
         auto size = (float(width) * sps) + 1;
         config((size / 2) / 3);
      }

      void config(float sigma)
      {
         std::size_t size = std::ceil(sigma * 3) * 2 + 1;
         int const half = size / 2;

         double const s = 2.0 * sigma * sigma;
         float max = 0.0;
         _w.resize(size);

         for (int i = 0; i < size; i++)
         {
            auto mid = i - half;
            _w[i] = 1.0 / std::sqrt(2_pi) * sigma * std::exp(-mid*mid/s);
            max = std::max(_w[i], max);
         }

         // Normalize to 1.0
         for (int i = 0; i < size; ++i)
            _w[i] /= max;
      }
   };
}

#endif
