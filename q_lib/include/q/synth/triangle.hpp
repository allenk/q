/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TRIANGLE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_TRIANGLE_HPP_DECEMBER_24_2015

#include <q/phase.hpp>
#include <q/detail/antialiasing.hpp>

namespace cycfi { namespace q
{
  ////////////////////////////////////////////////////////////////////////////
   // basic triangle-wave synthesizer (not bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct basic_triangle_synth
   {
      constexpr float operator()(phase p) const
      {
         constexpr float x = 4.0f / phase::one_cyc;
         return (abs(std::int32_t(p.val)) * x) - 1.0;
      }
   };

   constexpr auto basic_triangle = basic_triangle_synth{};

   ////////////////////////////////////////////////////////////////////////////
   // triangle-wave synthesizer (bandwidth limited)
   ////////////////////////////////////////////////////////////////////////////
   struct triangle_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto end = phase::max();
         constexpr auto edge1 = end/4;
         constexpr auto edge2 = end-edge1;
         constexpr float x = 4.0f / phase::one_cyc;

         auto r = (abs(std::int32_t((p + edge1).val)) * x) - 1.0;

         // Correct falling discontinuity
         r += detail::poly_blamp(p + edge1, dt, 4);

         // Correct rising discontinuity
         r -= detail::poly_blamp(p + edge2, dt, 4);

         return r;
      }
   };

   constexpr auto triangle = triangle_synth{};
}}

#endif
