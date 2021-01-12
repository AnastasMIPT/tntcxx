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
#include <type_traits>

#include "../Utils/CStr.hpp"
#include "Common.hpp"

namespace mpp {

using std::integral_constant;
using tnt::CStr;

/**
 * Specificators a wrappers around some objects, usually holds a constant
 * reference on that object and describes how that object must be packed to
 * msgpack stream.
 * For example std::tuple is packed as array by default, if you want it to
 * be packed as map, wrap it with mpp::as_map(<that tuple>).
 * Because of holding reference to an object a user must think about original
 * object's lifetime. The best practice is to use temporary specificator
 * objects, liks encoder.add(mpp::as_map(<that tuple>)).
 *
 * List of type specificators in this header (only one is allowed):
 * as_str - treat a value as string.
 * as_bin - treat a value as binary data.
 * as_arr - treat a value as msgpack array.
 * as_map - treat a value as msgpack map.
 * as_ext - treat a value as msgpack ext of given type.
 * as_raw - write already packed msgpack object as raw data.
 * reserve - write nothing of given size.
 *
 * List of optional specificators in this header:
 * track - additionally save beginning and end positions of the written object.
 * as_fixed - fix underlying type of msgpack object.
 *
 * List of terminal types:
 * sub_array - a part of an array (limited by given size).
 * MPP_AS_CONST - compile-time constant value (except strings).
 * MPP_AS_CONSTR - compile-time constant string value.
 */

/**
 * Dummy that declares that nothing should be encoded/decoded.
 */
struct ignore {};

/**
 * SubArray is C array or std::array with limited size.
 */
template <class ARRAY, class SIZE>
struct SubArray {
	ARRAY m_array;
	SIZE m_size;
	constexpr ARRAY get() const noexcept { return m_array; }
	constexpr SIZE size() const noexcept { return m_size; }
	static constexpr size_t capacity = any_arr<member_t<ARRAY>>::size;
};

template <class ARRAY, class SIZE>
constexpr auto sub_array_data(const SubArray<ARRAY, SIZE>& t) noexcept
{
	static_assert(any_arr<ARRAY>::is, "must be C or std array");
	return static_cast<typename any_arr<ARRAY>::data_type*>(std::data(t.get()));
}

template <class ARRAY, class SIZE>
constexpr auto
sub_array(ARRAY&& arr, SIZE&& size) noexcept
{
	static_assert(any_arr<member_t<ARRAY>>::is, "must be C or std array");
	check_size(size);
	using result_t = SubArray<save_or_ref<ARRAY>, save_or_ref<SIZE>>;
	return result_t{std::forward<ARRAY>(arr), std::forward<SIZE>(size)};
}

/**
 * A group of specificators - as_str(..), as_bin(..), as_arr(..), as_map(..),
 * as_raw(..), reserve(...).
 * They create SimpleWrapper with appropriate properties.
 * A wrapper takes a container or a range and specify explicitly how it must
 * be packed/unpacked as msgpack object.
 * A bit outstanding is 'as_raw' - it means that the data passed is expected
 * to be a valid msgpack object and must be just copied to the stream.
 * Another outstanding wrapper is 'reserve' - it means some number of bytes
 * must be skipped * (not written) in msgpack stream..
 * Specificators also accept the same arguments as range(..), in that case
 * it's a synonym of as_xxx(range(...)).
 */
template <class T, class EXT_T, compact::Type TYPE, bool IS_RAW, bool IS_RESERVE>
struct SimpleWrapper {
	static constexpr compact::Type mp_type = TYPE;
	static constexpr bool is_raw = IS_RAW;
	static constexpr bool is_reserve = IS_RESERVE;
	using type = T;
	using ext_type_t = EXT_T;
	T value;
	EXT_T ext_type;
};

#define DEFINE_SIMPLE_WRAPPER(name, mp_type, is_raw, is_reserve)		\
template <class T>								\
constexpr auto name(T&& t) noexcept						\
{										\
	using result_t = SimpleWrapper<save_or_ref<T>, ignore,			\
					mp_type, is_raw, is_reserve>;		\
	return result_t{std::forward<T>(t)};					\
}										\
										\
template <class ARRAY, class SIZE>						\
constexpr auto name(ARRAY&& arr, SIZE&& size) noexcept				\
{										\
	return name(sub_array(std::forward<ARRAY>(arr),				\
				   std::forward<SIZE>(size)));			\
}										\
										\
struct forgot_to_add_semicolon

DEFINE_SIMPLE_WRAPPER(as_str, compact::MP_STR, false, false);
DEFINE_SIMPLE_WRAPPER(as_bin, compact::MP_BIN, false, false);
DEFINE_SIMPLE_WRAPPER(as_arr, compact::MP_ARR, false, false);
DEFINE_SIMPLE_WRAPPER(as_map, compact::MP_MAP, false, false);
DEFINE_SIMPLE_WRAPPER(as_raw, compact::MP_END, true, false);

#undef DEFINE_SIMPLE_WRAPPER

/**
 * Specificator - as_ext(..). Creates a wrapper ext_holder that holds ext type
 * and a container (or range) and specifies that the data must packed/unpacked
 * as MP_EXIT msgpack object.
 * Specificator also accepts the same arguments as range(..), in that case
 * it's a synonym of as_ext(type, range(...)).
 */
template <class EXT_T, class T>
constexpr auto as_ext(EXT_T&& type, T&& t) noexcept
{
	using result_t = SimpleWrapper<save_or_ref<T>, save_or_ref<EXT_T>,
				       compact::MP_EXT, false, false>;
	return result_t{std::forward<T>(t), std::forward<EXT_T>(type)};
}

template <class EXT_T, class ARRAY, class SIZE>
constexpr auto as_ext(EXT_T&& type, ARRAY&& arr, SIZE&& size) noexcept
{
	return as_ext(type, sub_array(std::forward<ARRAY>(arr),
				      std::forward<SIZE>(size)));
}

/**
 * Specificator - track(..). Creates a wrapper track_holder that holds a value
 * and a range - a pair of iterators. The first iterator will be set to the
 * beginning of written msgpack object, the second - at the end of it.
*/
template <class T, class ITR_RANGE>
struct TrackWrapper {
	using type = T;
	T value;
	ITR_RANGE range;
};

template <class T, class ITR_RANGE>
constexpr auto track(T&& t, ITR_RANGE&& range)
{
	using result_t = TrackWrapper<save_or_ref<T>, save_or_ref<ITR_RANGE>>;
	return result_t{std::forward<T>(t), std::forward<ITR_RANGE>(range)};
}

/**
 * Reserve is an object that specifies that some number of bytes must be skipped
 * (not written) in msgpack stream.
 * There is also reserve(N, range) specificator that is synonyms for
 * track(reserve(N), range).
 */
template <class T>
constexpr auto reserve(T&& t) noexcept
{
	using result_t = SimpleWrapper<save_or_ref<T>, ignore,
				       compact::MP_END, false, true>;
	return result_t{std::forward<T>(t)};
}

template <class T, class ITR_RANGE>
constexpr auto reserve(T&& t, ITR_RANGE&& range) noexcept
{
	return reserve(track(std::forward<T>(t),
			     std::forward<ITR_RANGE>(range)));
}

/**
 * Specificator - is_fixed(..). Creates a wrapper fixed_holder that holds
 * a value and a definite underlying type by which the value must be written
 * to msgpack stream.
 * For example: as_fixed<uint8_t>(1) will be packed as "0xcc0x01",
 * as_fixed<uint64_t>(1) will be packed as "0xcf0x00x00x00x00x00x00x000x01",
 * as_fixed<void>(1) will be packed as "0x01".
 * By default the type is determined by the type of given value, but it also
 * may be specified explicitly.
 * The 'void' type means that a value must be one-byte packed into msgpack tag.
 */
template <class T, class U>
struct FixedWrapper {
	using as_type = T;
	using type = U;
	U value;
};

template <class U>
constexpr auto as_fixed(U&& u) noexcept
{
	using T = std::remove_cv_t<std::remove_reference_t<U>>;
	return FixedWrapper<T, save_or_ref<U>>{std::forward<U>(u)};
}

template <class T, class U>
constexpr auto as_fixed(U&& u) noexcept
{
	return FixedWrapper<T, save_or_ref<U>>{std::forward<U>(u)};
}

/**
 * Substitution: SubArray.
 */
template <class T, class ARRAY, class SIZE>
constexpr auto subst(T&& obj, const SubArray<ARRAY, SIZE>& t) noexcept
{
	if constexpr (std::is_member_object_pointer_v<ARRAY>) {
		if constexpr (std::is_member_object_pointer_v<SIZE>) {
			return sub_array(obj.*(t.get()), obj.*(t.size()));
		} else {
			return sub_array(obj.*(t.get()), t.size());
		}
	} else {
		if constexpr (std::is_member_object_pointer_v<SIZE>) {
			return sub_array(t.get(), obj.*(t.size()));
		} else {
			return t;
		}
	}
}

/**
 * Convinient wrappers for MP_ARR and MP_MAP construction.
 */
template<class... Types>
constexpr auto arr(Types&&... args) noexcept
{
	return as_arr(std::tuple<save_or_ref<Types>...>{std::forward<Types>(args)...});
}

template<class... Types>
constexpr auto tuple(Types&&... args) noexcept
{
	return as_arr(std::tuple<save_or_ref<Types>...>{std::forward<Types>(args)...});
}

template<class... Types>
constexpr auto map(Types&&... args) noexcept
{
	return as_map(std::tuple<save_or_ref<Types>...>{std::forward<Types>(args)...});
}

/**
 * Constants are types that have a constant value enclosed in type itself,
 * as some constexpr static member of the class.
 * For the most of constants std::integral_constant works perfectly.
 * MPP_AS_CONST is just a short form of creating an integral_constant.
 * There' some complexity in creating string constants. There's a special class
 * for them - CStr, that could be instantiated in a pair of ways.
 * MPP_AS_CONSTR is just a macro that instantiates on one of those forms.
 * There are also 'as_const' and 'as_constr' macros that are disabled by
 * default.
 */
#ifndef MPP_DISABLE_AS_CONST_MACRO
#define MPP_AS_CONST(x) std::integral_constant<decltype(x), x>{}
#endif
#ifndef TNT_DISABLE_STR_MACRO
#define MPP_AS_CONSTR(x) TNT_CON_STR(x)
#else
#ifndef TNT_DISABLE_STR_LITERAL
#define MPP_AS_CONSTR(x) x##_cs
#endif
#endif

#ifdef MPP_USE_SHORT_CONST_MACROS
#define as_const(x) std::integral_constant<decltype(x), x>{}
#define as_constr(x) MPP_AS_CONSTR(x)
#endif // #ifdef MPP_USE_SHORT_CONST_MACROS

}; // namespace mpp {
