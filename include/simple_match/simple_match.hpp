// Copyright 2015 John R. Bandela
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SIMPLE_MATCH_HPP_JRB_2015_03_21
#  define SIMPLE_MATCH_HPP_JRB_2015_03_21

#  include <stdexcept>
#  include <tuple>

namespace simple_match
{
using std::size_t;
namespace customization
{
template<class T, class U>
struct matcher;

}  // namespace customization

// Apply adapted from http://isocpp.org/files/papers/N3915.pdf
namespace detail
{
template<typename F, typename Tuple, size_t... i>
auto apply_impl(F&& f, Tuple&& t, std::integer_sequence<size_t, i...>)
    -> decltype(auto)
{
  using std::get;
  return std::forward<F>(f)(get<i>(std::forward<Tuple>(t))...);
}
template<typename F, typename Tuple>
auto apply(F&& f, Tuple&& t) -> decltype(auto)
{
  return apply_impl(
      std::forward<F>(f),
      std::forward<Tuple>(t),
      std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value> {});
}

}  // namespace detail

// exhaustiveness
namespace detail
{
template<class TA, class TB>
struct cat_tuple
{
};

template<class... A, class... B>
struct cat_tuple<std::tuple<A...>, std::tuple<B...>>
{
  using type = std::tuple<A..., B...>;
};

template<class TA, class TB>
using cat_tuple_t = typename cat_tuple<TA, TB>::type;

template<class... A>
struct arg_types
{
};

template<class A, class F>
struct arg_types<A, F>
{
  using type = std::tuple<std::decay_t<A>>;
};

template<class A1, class F1, class A2, class F2, class... Args>
struct arg_types<A1, F1, A2, F2, Args...>
{
  using type = cat_tuple_t<std::tuple<std::decay_t<A1>, std::decay_t<A2>>,
                           typename arg_types<Args...>::type>;
};

template<>
struct arg_types<>
{
  using type = std::tuple<>;
};

}  // namespace detail
struct empty_exhaustiveness
{
  template<class ArgTypes>
  struct type
  {
    static const bool value = true;
  };
};
namespace customization
{
template<class T>
struct exhaustiveness_checker
{
  using type = empty_exhaustiveness;
};
}  // namespace customization

// end exhaustiveness

template<class T, class U>
auto match_check(T&& t, U&& u) -> bool
{
  return customization::matcher<std::decay_t<T>, std::decay_t<U>>::check(
      std::forward<T>(t), std::forward<U>(u));
}

template<class T, class U>
auto match_get(T&& t, U&& u)
{
  return customization::matcher<std::decay_t<T>, std::decay_t<U>>::get(
      std::forward<T>(t), std::forward<U>(u));
}

struct no_match : std::logic_error
{
  no_match()
      : logic_error {"simple_match did not match"}
  {
  }
};

namespace detail
{
template<class T, class A1, class F1>
auto match_helper(T&& t, A1&& a, F1&& f)
{
  if (match_check(std::forward<T>(t), std::forward<A1>(a))) {
    return detail::apply(f, match_get(std::forward<T>(t), std::forward<A1>(a)));
  }

  throw no_match {};
}

template<class T, class A1, class F1, class A2, class F2, class... Args>
auto match_helper(T&& t, A1&& a, F1&& f, A2&& a2, F2&& f2, Args&&... args)
{
  if (match_check(t, a)) {
    return detail::apply(f, match_get(std::forward<T>(t), std::forward<A1>(a)));
  }

  return match_helper(t,
                      std::forward<A2>(a2),
                      std::forward<F2>(f2),
                      std::forward<Args>(args)...);
}

}  // namespace detail

template<class T, class... Args>
auto match(T&& t, Args&&... a)
{
  using atypes = typename detail::arg_types<Args...>::type;
  using ec =
      typename customization::exhaustiveness_checker<std::decay_t<T>>::type;
  using ctypes = typename ec::template type<atypes>;
  static_assert(ctypes::value, "Not all types are tested for in match");
  return detail::match_helper(std::forward<T>(t), std::forward<Args>(a)...);
}

struct otherwise_t
{
};

namespace placeholders
{
const otherwise_t otherwise {};
const otherwise_t _ {};
}  // namespace placeholders

namespace customization
{
// Match same type
template<class T>
struct matcher<T, T>
{
  static auto check(const T& t, const T& v) -> bool
  {
    return t == v;
  }
  static auto get(const T& /*unused*/, const T& /*unused*/)
  {
    return std::tie();
  }
};
// Match string literals
template<class T>
struct matcher<T, const char*>
{
  static auto check(const T& t, const char* str) -> bool
  {
    return t == str;
  }
  static auto get(const T& /*unused*/, const T& /*unused*/)
  {
    return std::tie();
  }
};

// Match otherwise
template<class Type>
struct matcher<Type, otherwise_t>
{
  template<class T>
  static auto check(T&& /*unused*/, otherwise_t /*unused*/) -> bool
  {
    return true;
  }
  template<class T>
  static auto get(T&& /*unused*/, otherwise_t /*unused*/)
  {
    return std::tie();
  }
};

}  // namespace customization
template<class F>
struct matcher_predicate
{
  F f;
};

template<class F>
auto make_matcher_predicate(F&& f) -> matcher_predicate<F>
{
  return matcher_predicate<F> {std::forward<F>(f)};
}

namespace customization
{
template<class Type, class F>
struct matcher<Type, matcher_predicate<F>>
{
  template<class T, class U>
  static auto check(T&& t, U&& u) -> bool
  {
    return u.f(std::forward<T>(t));
  }
  template<class T, class U>
  static auto get(T&& t, U&& /*unused*/)
  {
    return std::tie(std::forward<T>(t));
  }
};

}  // namespace customization

namespace placeholders
{
const auto _u = make_matcher_predicate([](auto&& /*unused*/) { return true; });
const auto _v = make_matcher_predicate([](auto&& /*unused*/) { return true; });
const auto _w = make_matcher_predicate([](auto&& /*unused*/) { return true; });
const auto _x = make_matcher_predicate([](auto&& /*unused*/) { return true; });
const auto _y = make_matcher_predicate([](auto&& /*unused*/) { return true; });
const auto _z = make_matcher_predicate([](auto&& /*unused*/) { return true; });

// relational operators
template<class F, class T>
auto operator==(const matcher_predicate<F>& m, const T& t)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && x == t; });
}

template<class F, class T>
auto operator!=(const matcher_predicate<F>& m, const T& t)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && x != t; });
}

template<class F, class T>
auto operator<=(const matcher_predicate<F>& m, const T& t)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && x <= t; });
}
template<class F, class T>
auto operator>=(const matcher_predicate<F>& m, const T& t)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && x >= t; });
}
template<class F, class T>
auto operator<(const matcher_predicate<F>& m, const T& t)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && x < t; });
}
template<class F, class T>
auto operator>(const matcher_predicate<F>& m, const T& t)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && x > t; });
}
template<class F>
auto operator!(const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m](const auto& x) { return !m.f(x); });
}

template<class F, class F2>
auto operator&&(const matcher_predicate<F>& m, const matcher_predicate<F2>& m2)
{
  return make_matcher_predicate([m, m2](const auto& x)
                                { return m.f(x) && m2.f(x); });
}

template<class F, class F2>
auto operator||(const matcher_predicate<F>& m, const matcher_predicate<F2>& m2)
{
  return make_matcher_predicate([m, m2](const auto& x)
                                { return m.f(x) || m2.f(x); });
}

template<class F, class T>
auto operator==(const T& t, const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && t == x; });
}

template<class F, class T>
auto operator!=(const T& t, const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && t != x; });
}

template<class F, class T>
auto operator<=(const T& t, const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && t <= x; });
}
template<class F, class T>
auto operator>=(const T& t, const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && t >= x; });
}
template<class F, class T>
auto operator<(const T& t, const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && t < x; });
}
template<class F, class T>
auto operator>(const T& t, const matcher_predicate<F>& m)
{
  return make_matcher_predicate([m, &t](const auto& x)
                                { return m.f(x) && t > x; });
}

}  // namespace placeholders

namespace detail
{
// We use this class, so we can differentiate between matcher <T,T> and matchter
// <T,std::tuple<T...>
struct tuple_ignorer
{
};
}  // namespace detail

namespace customization
{
template<class... A>
auto simple_match_get_tuple(const std::tuple<A...>& t)
    -> const std::tuple<A...>&
{
  return t;
}

template<class... A>
auto simple_match_get_tuple(std::tuple<A...>& t) -> std::tuple<A...>&
{
  return t;
}

template<class Type>
struct tuple_adapter
{
  template<size_t i, class T>
  static auto get(T&& t) -> decltype(auto)
  {
    using simple_match::customization::simple_match_get_tuple;
    return std::get<i>(simple_match_get_tuple(std::forward<T>(t)));
  }
};

template<class Type, class... Args>
struct matcher<Type, std::tuple<Args...>>
{
  using tu = tuple_adapter<Type>;
  enum
  {
    tuple_len = sizeof...(Args) - 1
  };
  template<size_t pos, size_t last>
  struct helper
  {
    template<class T, class A>
    static auto check(T&& t, A&& a) -> bool
    {
      return match_check(tu::template get<pos>(std::forward<T>(t)),
                         std::get<pos>(std::forward<A>(a)))
          && helper<pos + 1, last>::check(std::forward<T>(t),
                                          std::forward<A>(a));
    }

    template<class T, class A>
    static auto get(T&& t, A&& a)
    {
      return std::tuple_cat(
          match_get(tu::template get<pos>(std::forward<T>(t)),
                    std::get<pos>(std::forward<A>(a))),
          helper<pos + 1, last>::get(std::forward<T>(t), std::forward<A>(a)));
    }
  };

  template<size_t pos>
  struct helper<pos, pos>
  {
    template<class T, class A>
    static auto check(T&& t, A&& a) -> bool
    {
      return match_check(tu::template get<pos>(std::forward<T>(t)),
                         std::get<pos>(std::forward<A>(a)));
    }
    template<class T, class A>
    static auto get(T&& t, A&& a)
    {
      return match_get(tu::template get<pos>(std::forward<T>(t)),
                       std::get<pos>(std::forward<A>(a)));
    }
  };

  template<class T, class A>
  static auto check(T&& t, A&& a) -> bool
  {
    return helper<0, tuple_len - 1>::check(std::forward<T>(t),
                                           std::forward<A>(a));
  }
  template<class T, class A>
  static auto get(T&& t, A&& a)
  {
    return helper<0, tuple_len - 1>::get(std::forward<T>(t),
                                         std::forward<A>(a));
  }
};

}  // namespace customization

// destructure a tuple or other adapted structure
template<class... A>
auto ds(A&&... a)
{
  return std::make_tuple(std::forward<A>(a)..., detail::tuple_ignorer {});
}

}  // namespace simple_match

#  include "implementation/some_none.hpp"

#endif
