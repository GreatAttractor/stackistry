/*
Stackistry - astronomical image stacking
Copyright (C) 2016-2018 Filip Szczerek <ga.software@yahoo.com>

This file is part of Stackistry.

Stackistry is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Stackistry is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stackistry.  If not, see <http://www.gnu.org/licenses/>.

File description:
    Fix for versions of libsigc++ older than 2.2.11 to work with lambdas having a return value.
*/

#ifndef STACKISTRY_SIGNAL_FUNCTORS_HEADER
#define STACKISTRY_SIGNAL_FUNCTORS_HEADER

#include <sigc++/sigc++.h>
#include <sigc++config.h>

#if SIGCXX_MAJOR_VERSION < 2 || \
    SIGCXX_MAJOR_VERSION == 2 && SIGCXX_MINOR_VERSION <= 2 || \
    SIGCXX_MAJOR_VERSION == 2 && SIGCXX_MINOR_VERSION == 2 && SIGCXX_MICRO_VERSION < 11

#define SIGC_FUNCTORS_DEDUCE_RESULT_TYPE_WITH_DECLTYPE                 \
    template <typename Functor>                                        \
    struct functor_trait<Functor, false>                               \
    {                                                                  \
        typedef decltype (::sigc::mem_fun (std::declval<Functor&> (),  \
                    &Functor::operator())) _intermediate;              \
                                                                       \
        typedef typename _intermediate::result_type result_type;       \
        typedef Functor functor_type;                                  \
    };

#endif

#ifdef SIGC_FUNCTORS_DEDUCE_RESULT_TYPE_WITH_DECLTYPE
namespace sigc
{
    SIGC_FUNCTORS_DEDUCE_RESULT_TYPE_WITH_DECLTYPE
}
#endif

#endif  // STACKISTRY_SIGNAL_FUNCTORS_HEADER
