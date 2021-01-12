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

#include <cstdint>
#include <tuple>

#include "Constants.hpp"

namespace mpp {

template <bool CAN_DO_SHORT, bool CAN_DO_LONG, bool MUST_BE_POSITIVE, bool HAS_POSITIVE_RULE = false>
struct RuleBase {
	static constexpr bool can_do_short = CAN_DO_SHORT;
	static constexpr bool can_do_long = CAN_DO_LONG;
	static constexpr bool must_be_positive = MUST_BE_POSITIVE;
	static constexpr bool has_positive_rule = HAS_POSITIVE_RULE;
};

struct NilRule : RuleBase<true, false, false> {
	static constexpr uint8_t do_short(nullptr_t) { return 0xc0; }
};

struct BoolRule : RuleBase<true, false, false> {
	static constexpr uint8_t do_short(bool t) { return 0xc2 + t; }
};

struct UintRule : RuleBase<true, true, true> {
	template <class T>
	static constexpr bool check_short(T t) { return t < 128; }
	template <class T>
	static constexpr uint8_t do_short(T t) { return uint8_t(t); }

	using types = std::tuple<uint8_t, uint16_t, uint32_t, uint64_t>;
	static constexpr uint8_t type_tag = 0xcc;
};

struct IntRule : RuleBase<true, true, false, true> {
	template <class T>
	static constexpr bool check_short(T t) { return t >= -32; }
	template <class T>
	static constexpr uint8_t do_short(T t) { return uint8_t(t); }

	std::tuple<int8_t, int16_t, int32_t, int64_t> type;
	static constexpr uint8_t type_tag = 0xd0;
	using positive_rule = UintRule;
};

struct FltRule : RuleBase<false, true, false> {
	std::tuple<float> type;
	static constexpr uint8_t type_tag = 0xca;
};

struct DblRule : RuleBase<false, true, false> {
	std::tuple<double> type;
	static constexpr uint8_t type_tag = 0xcb;
};

struct StrRule : RuleBase<true, true, true> {
	template <class T>
	static constexpr bool check_short(T t) { return t < 32; }
	template <class T>
	static constexpr uint8_t do_short(T t) { return 0xa0 + uint8_t(t); }

	using types = std::tuple<uint8_t, uint16_t, uint32_t>;
	static constexpr uint8_t type_tag = 0xd9;
};

struct BinRule : RuleBase<false, true, true> {
	using types = std::tuple<uint8_t, uint16_t, uint32_t>;
	static constexpr uint8_t type_tag = 0xc4;
};

struct ArrRule : RuleBase<true, true, true> {
	template <class T>
	static constexpr bool check_short(T t) { return t < 16; }
	template <class T>
	static constexpr uint8_t do_short(T t) { return 0x90 + uint8_t(t); }

	using types = std::tuple<uint16_t, uint32_t>;
	static constexpr uint8_t type_tag = 0xdc;
};

struct MapRule : RuleBase<true, true, true> {
	template <class T>
	static constexpr bool check_short(T t) { return t < 16; }
	template <class T>
	static constexpr uint8_t do_short(T t) { return 0x80 + uint8_t(t); }

	using types = std::tuple<uint16_t, uint32_t>;
	static constexpr uint8_t type_tag = 0xde;
};

struct ExtRule : RuleBase<true, true, true> {
	template <class T>
	static constexpr bool check_short(T t)
	{
		return (t > 0) && (t <= 16) && ((t & (t - 1)) == 0);
	}
	template <class T>
	static constexpr uint8_t do_short(T t)
	{
		return t == 1 ? 0xd4 : t == 2 ? 0xd5 : t == 4 ? 0xd6 : t == 8 ? 0xd7 : 0xd8;
	}

	using types = std::tuple<uint8_t, uint16_t, uint32_t>;
	static constexpr uint8_t type_tag = 0xc7;
};

template <compact::Type TYPE>
struct RuleSelector;

template <> struct RuleSelector<compact::MP_NIL > { using type = NilRule;  };
template <> struct RuleSelector<compact::MP_BOOL> { using type = BoolRule; };
template <> struct RuleSelector<compact::MP_UINT> { using type = UintRule; };
template <> struct RuleSelector<compact::MP_INT > { using type = IntRule;  };
template <> struct RuleSelector<compact::MP_FLT > { using type = FltRule;  };
template <> struct RuleSelector<compact::MP_DBL > { using type = DblRule;  };
template <> struct RuleSelector<compact::MP_STR > { using type = StrRule;  };
template <> struct RuleSelector<compact::MP_BIN > { using type = BinRule;  };
template <> struct RuleSelector<compact::MP_ARR > { using type = ArrRule;  };
template <> struct RuleSelector<compact::MP_MAP > { using type = MapRule;  };
template <> struct RuleSelector<compact::MP_EXT > { using type = ExtRule;  };

template <compact::Type TYPE>
using Rule_t = typename RuleSelector<TYPE>::type;

}; // namespace mpp {
