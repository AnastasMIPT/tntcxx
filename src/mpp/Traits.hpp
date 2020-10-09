#pragma once
/*
 * Copyright 2010-2020, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <array>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <variant>

#include "Types.hpp"
#include "Constants.hpp"

namespace mpp {
/**
 * Delayer of static_assert evaluation.
 */
template <class>
constexpr bool always_false_v = false;

/**
 * Define a type checker @a id_name of a template class @a class_name.
 * MPP_DEFINE_TYPE_CHECKER works only if:
 *  class_name template arguments are types, like std::vector, std::tuple etc.
 * MPP_DEFINE_TYPE_CHECKER_V works only if:
 *  class_name template arguments are values of one type,
 *  like std::index_squence etc.
 * MPP_DEFINE_TYPE_CHECKER_TV works only if:
 *  class_name template arguments are one type and one value, like std::array,
 *  std::integral_constant etc.
 * For example you have a template <class T> class A {..} and want to check
 * whether a type U is of type A with unknown template parameter. Then simply:
 *
 * MPP_DEFINE_TYPE_CHECKER(is_a_v, A);
 * ...
 * if constexpr (is_a_v<U>) ...
 *
 */
#define MPP_DEFINE_TYPE_CHECKER(id_name, class_name) \
template <class T> \
struct id_name##_helper : std::false_type {}; \
\
template <class... T> \
struct id_name##_helper<class_name<T...>> : std::true_type {}; \
\
template <class T> \
constexpr bool id_name = id_name##_helper<T>::value

#define MPP_DEFINE_TYPE_CHECKER_V(id_name, class_name) \
template <class T> \
struct id_name##_helper : std::false_type {}; \
\
template <class T, T... V> \
struct id_name##_helper<class_name<V...>> : std::true_type {}; \
\
template <class T> \
constexpr bool id_name = id_name##_helper<T>::value

#define MPP_DEFINE_TYPE_CHECKER_TV(id_name, class_name) \
template <class T> \
struct id_name##_helper : std::false_type {}; \
\
template <class T, class U, U V> \
struct id_name##_helper<class_name<T, V>> : std::true_type {}; \
\
template <class T> \
constexpr bool id_name = id_name##_helper<T>::value

/** Type checkers for types defined in Types.hpp. */
MPP_DEFINE_TYPE_CHECKER(is_range_v, Range);
MPP_DEFINE_TYPE_CHECKER(is_str_v, str_holder);
MPP_DEFINE_TYPE_CHECKER(is_bin_v, bin_holder);
MPP_DEFINE_TYPE_CHECKER(is_arr_v, arr_holder);
MPP_DEFINE_TYPE_CHECKER(is_map_v, map_holder);
MPP_DEFINE_TYPE_CHECKER(is_raw_v, raw_holder);
MPP_DEFINE_TYPE_CHECKER_V(is_reserve_v, Reserve);
MPP_DEFINE_TYPE_CHECKER(is_ext_v, ext_holder);
MPP_DEFINE_TYPE_CHECKER(is_track_v, track_holder);
MPP_DEFINE_TYPE_CHECKER(is_fixed_v, fixed_holder);

MPP_DEFINE_TYPE_CHECKER_TV(is_const_v, std::integral_constant);
MPP_DEFINE_TYPE_CHECKER_V(is_constr_v, CStr);

/** Other useful type checkers. */
MPP_DEFINE_TYPE_CHECKER(is_tuple_v, std::tuple);
MPP_DEFINE_TYPE_CHECKER(is_variant_v, std::variant);
MPP_DEFINE_TYPE_CHECKER_TV(is_std_array_v, std::array);

/** Simple helpers. */
template <class T>
struct const_value_type_helper
{
	using type = void;
};
template <class T, T V>
struct const_value_type_helper<std::integral_constant<T, V>>
{
	using type = typename std::integral_constant<T, V>::value_type;
};
template <class T>
using const_value_type = typename const_value_type_helper<T>::type;

template <class T>
constexpr bool is_const_b =
	is_const_v<T> && std::is_same_v<const_value_type<T>, bool>;
template <class T>
constexpr bool is_const_f =
	is_const_v<T> && std::is_same_v<const_value_type<T>, float>;
template <class T>
constexpr bool is_const_d =
	is_const_v<T> && std::is_same_v<const_value_type<T>, double>;
template <class T>
constexpr bool is_const_s =
	is_const_v<T> && std::is_integral_v<const_value_type<T>> &&
	std::is_signed_v<const_value_type<T>> && !is_const_b<T>;
template <class T>
constexpr bool is_const_u =
	is_const_v<T> && std::is_integral_v<const_value_type<T>> &&
	std::is_unsigned_v<const_value_type<T>> && !is_const_b<T>;
template <class T>
constexpr bool is_const_e =
	is_const_v<T> && std::is_enum_v<const_value_type<T>>;

/**
 * Group checker of str, bin, arr and map.
 */
template <class T>
constexpr bool is_simple_spec_v = is_str_v<T> || is_bin_v<T> || is_arr_v<T> || is_map_v<T>;

template <class T>
constexpr compact::Type get_simple_type()
{
	if constexpr (is_str_v<T>)
		return compact::MP_STR;
	else if constexpr (is_bin_v<T>)
		return compact::MP_BIN;
	else if constexpr (is_arr_v<T>)
		return compact::MP_ARR;
	else if constexpr (is_map_v<T>)
		return compact::MP_MAP;
	else
		static_assert(always_false_v<T>);
}

/** Char type checker. */
template <class T>
using is_cvref_char = std::is_same<char, std::decay_t<T>>;

template <class T>
constexpr bool is_cvref_char_v = is_cvref_char<T>::value;

/**
 * Type checker of an array-like type - the thing to which are invokable
 * std::cbegin(), std::cend() add std::size()
 */
template <class T, class _ = void>
struct looks_like_arr : std::false_type {};

template <class T>
struct looks_like_arr<
	T,
	std::void_t<
		decltype(*std::cbegin(std::declval<T>())),
		decltype(*std::cend(std::declval<T>())),
		decltype(std::size(std::declval<T>()))
	>
> : std::true_type {};

template <class T>
constexpr bool looks_like_arr_v = looks_like_arr<T>::value;

/**
 * Type checker of an maps-like type - the thing to which are invokable
 * std::cbegin(), std::cend() add std::size(), and *std::cbegin() has
 * .first and .second member.
 * Note that everything that looks like map also looks like array.
 */
template <class T, class _ = void>
struct looks_like_map : std::false_type {};

template <class T>
struct looks_like_map<
	T,
	std::void_t<
		decltype(std::cbegin(std::declval<T>())->first),
		decltype(std::cbegin(std::declval<T>())->second),
		decltype(std::cend(std::declval<T>())),
		decltype(std::size(std::declval<T>()))
	>
> : std::true_type {};

template <class T>
constexpr bool looks_like_map_v = looks_like_map<T>::value;

/**
 * Type checker of an string-like type - the thing to which are invokable
 * std::cbegin(), std::cend() add std::size(), and *std::cbegin() has
 * .first and .second member.
 * Note that everything that looks like string also possibly looks like array.
 * Note that C style [const] char * is not detected by this checker.
 */
template <class T, class _ = void>
struct looks_like_str : std::false_type {};

template <class T>
struct looks_like_str<
	T,
	std::void_t<
		decltype(*std::data(std::declval<T&>())),
		decltype(std::size(std::declval<T&>()))
	>
> : is_cvref_char<decltype(*std::data(std::declval<T&>()))> {};

template <class T>
constexpr bool looks_like_str_v = looks_like_str<T>::value;

/**
 * Type checker that detects C-like string - char* or const char*.
 */
template <class T>
constexpr bool is_c_str_v =
	std::is_same_v<char *, T> || std::is_same_v<const char *, T>;

/**
 * Check that a type is container-type class with fixed size.
 */
template <class T>
struct has_fixed_size : std::false_type {};

template <class... T>
struct has_fixed_size<std::tuple<T...>> : std::true_type
{ static constexpr size_t size = sizeof...(T); };

template <class T, size_t N>
struct has_fixed_size<std::array<T, N>> : std::true_type
{ static constexpr size_t size = N; };

template <class T, size_t N>
struct has_fixed_size<T[N]> : std::true_type
{ static constexpr size_t size = N; };

template <class T, size_t N>
struct has_fixed_size<Range <T , N>> : Range<T, N>::is_fixed_size
{ static constexpr size_t size = N; };

template <class T>
constexpr bool has_fixed_size_v = has_fixed_size<T>::value;

template <class T>
constexpr size_t get_fixed_size_v = has_fixed_size<T>::size;

/**
 * Get
 */
template <class T>
constexpr size_t power_v()
{
	if constexpr (sizeof(T) == 1)
		return 0;
	else if constexpr (sizeof(T) == 2)
		return 1;
	else if constexpr (sizeof(T) == 4)
		return 2;
	else if constexpr (sizeof(T) == 8)
		return 3;
	else
		static_assert(always_false_v<T>, "wrong type");
}

} // namespace mpp {
