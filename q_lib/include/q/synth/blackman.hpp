/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BLACKMAN_HPP_JANUARY_16_2021)
#define CYCFI_Q_BLACKMAN_HPP_JANUARY_16_2021

#include <q/support/phase.hpp>
#include <q/detail/blackman_table.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // blackman_synth: Synthesizes a blackman window.
   ////////////////////////////////////////////////////////////////////////////
   struct blackman_synth
   {
      constexpr float operator()(phase p) const
      {
         return detail::blackman_gen(p);
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase);
      }
   };

   constexpr auto blackman = blackman_synth{};
}

#endif
