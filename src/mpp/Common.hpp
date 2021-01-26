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
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <tuple>

namespace mpp {
/**
 * Delayer of static_assert evaluation.
 */
template <class>
constexpr bool always_false_v = false;

/**
 * Constexpr log2(x)
 */
template <size_t X>
constexpr size_t log2_v()
{
	static_assert(X > 0 && (X & (X - 1)) == 0, "must be power of 2");
	if constexpr (X == 1)
		return 0;
	else
		return 1 + log2_v<X / 2>();
}

/**
 * Constexpr log2(sizeof(T)) where T is simple type (of size 1,2,4,8).
 */
template <class T>
constexpr size_t type_power_v()
{
	static_assert(sizeof(T) <= 8 && (sizeof(T) & (sizeof(T) - 1)) == 0, "bad type");
	return log2_v<sizeof(T)>();
}


/**
 * Getter of unsigned integer type with the size as given type.
 */
using uint_types = std::tuple<uint8_t, uint16_t, uint32_t, uint64_t>;
template <class T>

using under_uint_t = typename std::tuple_element_t<type_power_v<T>(), uint_types>;

/**
 * Getter of signed integer type with the size as given type.
 */
using int_types = std::tuple<int8_t, int16_t, int32_t, int64_t>;

template <class T>
using under_int_t = typename std::tuple_element_t<type_power_v<T>(), int_types>;

/**
 * bswap overloads.
 */
inline uint8_t  bswap(uint8_t x)  { return x; }
inline uint16_t bswap(uint16_t x) { return __builtin_bswap16(x); }
inline uint32_t bswap(uint32_t x) { return __builtin_bswap32(x); }
inline uint64_t bswap(uint64_t x) { return __builtin_bswap64(x); }

/**
 * Standard unreachable.
 */
[[noreturn]] void unreachable() { assert(false); __builtin_unreachable(); }

/**
 * Info about C and std arrays.
 */
template <bool IS, size_t SIZE, class TYPE, bool DATA_IS_CONST>
struct any_arr_info {
	static constexpr bool is = IS;
	static constexpr size_t size = SIZE;
	using type = TYPE;
	using data_type = std::conditional_t<DATA_IS_CONST, const TYPE, TYPE>;
};

template <bool DATA_IS_CONST, class T>
struct any_arr_helper : any_arr_info<false, 0, void, DATA_IS_CONST> {};

template <bool DATA_IS_CONST, class T, size_t N>
struct any_arr_helper<DATA_IS_CONST, std::array<T, N>> : any_arr_info<true, N, T, DATA_IS_CONST> {};

template <bool DATA_IS_CONST, class T, size_t N>
struct any_arr_helper<DATA_IS_CONST, T[N]> : any_arr_info<true, N, T, DATA_IS_CONST> {};

template <class T>
struct any_arr : any_arr_helper<std::is_const_v<std::remove_reference_t<T>>,
				std::remove_cv_t<std::remove_reference_t<T>>> {};

/**
 * Type of member.
 */
template <class T>
struct member_helper { using type = T; };

template <class T, class U>
struct member_helper<T U::*> { using type = T; };

template <class T>
using member_t = typename member_helper<std::remove_cv_t<T>>::type;

/**
 * Transform rvalue reference to const value.
 * Don't transform otherwise.
 */
template <class T>
using save_or_ref = std::conditional_t<std::is_rvalue_reference_v<T&&>,
				       const std::remove_reference_t<T>, T>;

/**
 * Early time check of SIZE argument, that could be one of the following:
 * 1. integral value.
 * 2. enum value.
 * 3. std::integral_constant.
 * 4. pointer to integral or enum member of some class.
 */
template <class T>
constexpr void check_size(const T&)
{
	static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
}

template <class T, T V>
constexpr void check_size(const std::integral_constant<T, V>&)
{
}

template <class T, class U>
constexpr void check_size(T U::*const&)
{
	static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
}

}; // namespace mpp {
