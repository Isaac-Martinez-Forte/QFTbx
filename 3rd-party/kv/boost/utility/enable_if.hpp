// The two Boost utilities kv uses, provided over the standard library so that
// kv builds without Boost installed: kv reads them from <boost/...>, and this
// directory comes before any system Boost in the include path.
#ifndef QFTBX_KV_BOOST_ENABLE_IF_HPP
#define QFTBX_KV_BOOST_ENABLE_IF_HPP

#include <type_traits>

namespace boost {

template <bool B, class T = void> struct enable_if_c : std::enable_if<B, T> {};
template <bool B, class T = void> struct disable_if_c : std::enable_if<!B, T> {};
template <class Cond, class T = void> struct enable_if : std::enable_if<Cond::value, T> {};
template <class Cond, class T = void> struct disable_if : std::enable_if<!Cond::value, T> {};

} // namespace boost

#endif
