// The type traits kv uses, provided over the standard library so that kv
// builds without Boost installed (see boost/utility/enable_if.hpp).
#ifndef QFTBX_KV_BOOST_TYPE_TRAITS_HPP
#define QFTBX_KV_BOOST_TYPE_TRAITS_HPP

#include <type_traits>

namespace boost {

using std::is_convertible;
using std::is_integral;
using std::is_same;

} // namespace boost

#endif
