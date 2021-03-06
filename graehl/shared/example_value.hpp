// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    for configure.hpp library - customize the printed example value - instead of
    to_string(default constructed value).

    TODO: ADL

*/

#ifndef EXAMPLE_VALUE_2012531_HPP
#define EXAMPLE_VALUE_2012531_HPP
#pragma once


#include <graehl/shared/int_types.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace graehl {

template <class T>
std::string example_value(T const&) {
  return "";  // to_string(T());
}

inline std::string example_value(std::string const&) {
  return "foo";
}

#define GRAEHL_EXAMPLE_VALUE_FLOAT(Type) \
  inline std::string example_value(Type const&) { return to_string(Type(0.5)); }
#define GRAEHL_EXAMPLE_VALUE_INT(Type) \
  inline std::string example_value(Type const&) { return to_string(Type(65)); }

GRAEHL_FOR_DISTINCT_INT_TYPES(GRAEHL_EXAMPLE_VALUE_INT)
GRAEHL_FOR_DISTINCT_FLOAT_TYPES(GRAEHL_EXAMPLE_VALUE_FLOAT)

template <class T>
std::string example_value() {
  return example_value(*(T const*)0);
}

template <class T>
std::string example_value(shared_ptr<T> const&) {
  return example_value(*(T const*)0);
}

template <class T>
std::string example_value(boost::optional<T> const&) {
  return example_value(*(T const*)0);
}


}

#endif
