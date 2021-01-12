//
// Created by test on 10/8/20.
//

/*
 * Copyright (c) 2020, Aleksandr Lyapunov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <vector>

#include "../src/Buffer/Buffer.hpp"
#include "../src/mpp/mpp.hpp"

#include "Utils/Helpers.hpp"

/// Common tests
void
test_common()
{
	TEST_INIT(0);
	static_assert(mpp::log2_v<1>() == 0);
	static_assert(mpp::log2_v<16>() == 4);
	static_assert(mpp::type_power_v<double>() == 3);

	static_assert(std::is_unsigned_v<mpp::under_uint_t<int8_t>>);
	static_assert(std::is_unsigned_v<mpp::under_uint_t<int16_t>>);
	static_assert(std::is_unsigned_v<mpp::under_uint_t<int32_t>>);
	static_assert(std::is_unsigned_v<mpp::under_uint_t<int64_t>>);

	static_assert(std::is_signed_v<mpp::under_int_t<uint8_t>>);
	static_assert(std::is_signed_v<mpp::under_int_t<uint16_t>>);
	static_assert(std::is_signed_v<mpp::under_int_t<uint32_t>>);
	static_assert(std::is_signed_v<mpp::under_int_t<uint64_t>>);

	static_assert(sizeof(mpp::under_uint_t<int8_t>) == 1);
	static_assert(sizeof(mpp::under_uint_t<int16_t>) == 2);
	static_assert(sizeof(mpp::under_uint_t<int32_t>) == 4);
	static_assert(sizeof(mpp::under_uint_t<int64_t>) == 8);

	static_assert(sizeof(mpp::under_int_t<uint8_t>) == 1);
	static_assert(sizeof(mpp::under_int_t<uint16_t>) == 2);
	static_assert(sizeof(mpp::under_int_t<uint32_t>) == 4);
	static_assert(sizeof(mpp::under_int_t<uint64_t>) == 8);

	static_assert(!mpp::any_arr<int>::is);
	static_assert(!mpp::any_arr<int*>::is);
	static_assert(!mpp::any_arr<std::vector<int>>::is);

	using T1 = std::array<int, 10>;
	static_assert(mpp::any_arr<T1>::is);
	static_assert(mpp::any_arr<T1>::size == 10);
	static_assert(std::is_same_v<int, mpp::any_arr<T1>::type>);
	static_assert(std::is_same_v<int, mpp::any_arr<T1>::data_type>);

	using T2 = const T1;
	static_assert(mpp::any_arr<T2>::is);
	static_assert(mpp::any_arr<T2>::size == 10);
	static_assert(std::is_same_v<int, mpp::any_arr<T2>::type>);
	static_assert(std::is_same_v<const int, mpp::any_arr<T2>::data_type>);

	using T3 = const T1&;
	static_assert(mpp::any_arr<T3>::is);
	static_assert(mpp::any_arr<T3>::size == 10);
	static_assert(std::is_same_v<int, mpp::any_arr<T3>::type>);
	static_assert(std::is_same_v<const int, mpp::any_arr<T3>::data_type>);

	using T4 = T1&&;
	static_assert(mpp::any_arr<T4>::is);
	static_assert(mpp::any_arr<T4>::size == 10);
	static_assert(std::is_same_v<int, mpp::any_arr<T4>::type>);
	static_assert(std::is_same_v<int, mpp::any_arr<T4>::data_type>);
}

/// Test for mpp::sub_array
template <class T>
auto unsignify(T t) {
	if constexpr (std::is_signed_v<T>) {
		fail_unless(t >= 0);
		std::make_unsigned_t<T> r = t;
		return r;
	} else {
		return t;
	}
}

template <class T, class U, class V, class W>
void check_sub_array_read(const T& t, U exp_front, V exp_size, W exp_capacity)
{
	fail_unless(unsignify(mpp::sub_array_data(t)[0]) == unsignify(exp_front));
	fail_unless(unsignify(mpp::unwrap_integral(t.size())) == unsignify(exp_size));
	fail_unless(unsignify(T::capacity) == unsignify(exp_capacity));
}

template <class T, class U, class V, class W>
void check_sub_array_write_arr(const T& t, U exp_front, V exp_size, W exp_capacity)
{
	mpp::sub_array_data(t)[0]++;
	fail_unless(unsignify(mpp::sub_array_data(t)[0]) == unsignify(exp_front));
	fail_unless(unsignify(mpp::unwrap_integral(t.size())) == unsignify(exp_size));
	fail_unless(unsignify(T::capacity) == unsignify(exp_capacity));
}

template <class T, class U, class V, class W>
void check_sub_array_write_full(const T& t, U exp_front, V exp_size, W exp_capacity)
{
	mpp::sub_array_data(t)[0]++;
	t.size()++;
	fail_unless(unsignify(mpp::sub_array_data(t)[0]) == unsignify(exp_front));
	fail_unless(unsignify(mpp::unwrap_integral(t.size())) == unsignify(exp_size));
	fail_unless(unsignify(T::capacity) == unsignify(exp_capacity));
}

template <class T, class W>
void test_sub_const_array(const T&t, W exp_capacity)
{
	check_sub_array_read(mpp::sub_array(t, 3), t[0], 3, exp_capacity);
	check_sub_array_read(mpp::sub_array(t, std::integral_constant<size_t, 4>{}), t[0], 4, exp_capacity);
}

void
test_sub_array()
{
	TEST_INIT(0);
	int carr[10] = {1, 2, 3};
	std::array<unsigned, 15> stdarr = {4, 5, 6};
	size_t size = 7;

	check_sub_array_read(mpp::sub_array(carr, 1), 1, 1, 10);
	check_sub_array_read(mpp::sub_array(carr, std::integral_constant<size_t, 2>{}), 1, 2, 10);
	test_sub_const_array(carr, 10);
	fail_unless(carr[0] == 1);
	check_sub_array_write_arr(mpp::sub_array(carr, 5), 2, 5, 10);
	fail_unless(carr[0] == 2);
	check_sub_array_write_arr(mpp::sub_array(carr, std::integral_constant<int, 6>{}), 3, 6, 10);
	fail_unless(carr[0] == 3);
	check_sub_array_write_full(mpp::sub_array(carr, size), 4, 8, 10);
	fail_unless(carr[0] == 4);
	fail_unless(size == 8);

	check_sub_array_read(mpp::sub_array(stdarr, 1), 4, 1, 15);
	check_sub_array_read(mpp::sub_array(stdarr, std::integral_constant<size_t, 2>{}), 4, 2, 15);
	test_sub_const_array(stdarr, 15);
	fail_unless(stdarr[0] == 4);
	check_sub_array_write_arr(mpp::sub_array(stdarr, 5), 5, 5, 15);
	fail_unless(stdarr[0] == 5);
	check_sub_array_write_arr(mpp::sub_array(stdarr, std::integral_constant<int, 6>{}), 6, 6, 15);
	fail_unless(stdarr[0] == 6);
	check_sub_array_write_full(mpp::sub_array(stdarr, size), 7, 9, 15);
	fail_unless(stdarr[0] == 7);
	fail_unless(size == 9);


	struct Test {
		int carr[20];
		std::array<int, 25> stdarr;
		size_t size;
	} test;
	auto s1 = mpp::sub_array(&Test::carr, 13);
	static_assert(!std::is_reference_v<decltype(s1.m_array)>);
	static_assert(!std::is_reference_v<decltype(s1.m_size)>);
	static_assert(!std::is_reference_v<decltype(s1.m_array)>);
	auto s2 = mpp::sub_array(&Test::carr, &Test::size);
	static_assert(s1.capacity == 20);
	static_assert(s2.capacity == 20);
	auto s3 = mpp::sub_array(&Test::stdarr, 17);
	auto s4 = mpp::sub_array(&Test::stdarr, &Test::size);
	static_assert(s3.capacity == 25);
	static_assert(s4.capacity == 25);

	test.carr[0] = 100;
	test.stdarr[0] = 200;
	test.size = 10;
	fail_unless((test.*s1.get())[0] == 100);
	fail_unless(s1.size() == 13);
	fail_unless((test.*s2.get())[0] == 100);
	fail_unless(test.*(s2.size()) == 10);
	fail_unless((test.*(s3.get()))[0] == 200);
	fail_unless(s3.size() == 17);
	fail_unless((test.*(s4.get()))[0] == 200);
	fail_unless(test.*(s4.size()) == 10);
	check_sub_array_read(mpp::subst((const Test&)test, s1), 100, 13, 20);
	check_sub_array_write_full(mpp::subst(test, s2), 101, 11, 20);
	check_sub_array_read(mpp::subst((const Test&)test, s3), 200, 17, 25);
	check_sub_array_write_full(mpp::subst(test, s4), 201, 12, 25);
	fail_unless(test.size == 12);
};

/// Test that check string traits.
template <bool expect_c_string, class T>
void
test_static_assert_strings(const T&)
{
	static_assert(expect_c_string != mpp::looks_like_str_v<T>);
	static_assert(expect_c_string == mpp::is_c_str_v<T>);
}

void
test_string_traits()
{
	TEST_INIT(0);
	std::string str;
	std::string_view strv;
	char arr[20] = "aaa";
	const char *cstr = "bbb";
	char *mcstr = arr;
	test_static_assert_strings<false>(std::string{});
	test_static_assert_strings<false>(str);
	test_static_assert_strings<false>(std::string_view{});
	test_static_assert_strings<false>(strv);
	test_static_assert_strings<false>(arr);
	test_static_assert_strings<false>("ccc");
	test_static_assert_strings<true>(cstr);
	test_static_assert_strings<true>(mcstr);
}

/// Test that shows how enums are put to streams.
void
test_type_visual()
{
	TEST_INIT(0);
	using namespace mpp;
	std::cout << compact::MP_ARR << " "
		  << compact::MP_MAP << " "
		  << compact::MP_EXT << "\n";
	std::cout << MP_NIL << " "
		  << MP_BOOL << " "
		  << (MP_UINT | MP_INT) << " "
		  << (MP_BIN  | MP_STR) << " "
		  << (MP_UINT | MP_INT | MP_DBL | MP_FLT) << "\n";
}

struct TestArrStruct {
	size_t parsed_arr_size;
	double dbl;
	float flt;
	char str[12];
	nullptr_t nil;
	bool b;
};


struct ArrValueReader : mpp::DefaultErrorHandler {
	using Buffer_t = tnt::Buffer<16 * 1024>;
	using BufferIterator_t = typename Buffer_t::iterator;
	explicit ArrValueReader(TestArrStruct& a) : arr(a) {}
	static constexpr mpp::Type VALID_TYPES = mpp::MP_DBL | mpp::MP_FLT |
		mpp::MP_STR | mpp::MP_NIL | mpp::MP_BOOL;
	template <class T>
	void Value(const BufferIterator_t&, mpp::compact::Type, T v)
	{
		using A = TestArrStruct;
		static constexpr std::tuple map(&A::dbl, &A::flt, &A::nil, &A::b);
		auto ptr = std::get<std::decay_t<T> A::*>(map);
		arr.*ptr = v;
	}
	void Value(const BufferIterator_t& itr, mpp::compact::Type, mpp::StrValue v)
	{
		BufferIterator_t tmp = itr;
		tmp += v.offset;
		char *dst = arr.str;
		while (v.size) {
			*dst++ = *tmp;
			++tmp;
			--v.size;
		}
		*dst = 0;
	}

	BufferIterator_t* StoreEndIterator() { return nullptr; }
	TestArrStruct& arr;
};

struct ArrReader : mpp::SimpleReaderBase<tnt::Buffer<16 * 1024>, mpp::MP_ARR> {
	using Buffer_t = tnt::Buffer<16 * 1024>;
	using BufferIterator_t = typename Buffer_t::iterator;
	ArrReader(TestArrStruct& a, mpp::Dec<Buffer_t>& d) : arr(a), dec(d) {}
	void Value(const BufferIterator_t&, mpp::compact::Type, mpp::ArrValue v)
	{
		arr.parsed_arr_size = v.size;
		dec.SetReader(false, ArrValueReader{arr});
	}

	TestArrStruct& arr;
	mpp::Dec<Buffer_t>& dec;
};

namespace example {

using Buffer_t = tnt::Buffer<16 * 1024>;
using BufferIterator_t = typename Buffer_t::iterator;

struct TestMapStruct {
	bool boo;
	char str[12];
	size_t str_size;
	int arr[3];
	size_t arr_size;
};

struct MapKeyReader : mpp::SimpleReaderBase<Buffer_t, mpp::MP_UINT> {
	MapKeyReader(mpp::Dec<Buffer_t>& d, TestMapStruct& m) : dec(d), map(m) {}

	void Value(const BufferIterator_t&, mpp::compact::Type, uint64_t k)
	{
		using map_t = TestMapStruct;
		using Boo_t = mpp::SimpleReader<Buffer_t, mpp::MP_BOOL, bool>;
		using Str_t = mpp::SimpleStrReader<Buffer_t, sizeof(map_t::str)>;
		using Arr_t = mpp::SimpleArrReader
			<mpp::Dec<Buffer_t>,
			Buffer_t,
			sizeof(map_t::arr) / sizeof(map_t::arr[0]),
			mpp::MP_UINT,
			int
			>;
		switch (k) {
		case 10:
			dec.SetReader(true, Boo_t{map.boo});
			break;
		case 11:
			dec.SetReader(true, Str_t{map.str, map.str_size});
			break;
		case 12:
			dec.SetReader(true, Arr_t{dec, map.arr, map.arr_size});
			break;
		default:
			dec.AbortAndSkipRead();
		}
	}

	mpp::Dec<Buffer_t>& dec;
	TestMapStruct& map;
};


struct MapReader : mpp::SimpleReaderBase<tnt::Buffer<16 * 1024>, mpp::MP_MAP> {
	MapReader(mpp::Dec<Buffer_t>& d, TestMapStruct& m) : dec(d), map(m) {}

	void Value(const BufferIterator_t&, mpp::compact::Type, mpp::MapValue)
	{
		dec.SetReader(false, MapKeyReader{dec, map});
	}

	mpp::Dec<Buffer_t>& dec;
	TestMapStruct& map;
};

} // namespace example {


enum {
	MUNUS_ONE_HUNDRED = -100,
};


enum {
	FOR_BILLIONS = 4000000000u,
};

void
test_basic()
{
	TEST_INIT(0);
	using Buf_t = tnt::Buffer<16 * 1024>;
	Buf_t buf;
	mpp::Enc<Buf_t> enc(buf);
	enc.add(0);
	enc.add(10);
	enc.add(uint8_t(200));
	enc.add(short(2000));
	enc.add(2000000);
	enc.add(4000000000u);
	enc.add(FOR_BILLIONS);
	enc.add(20000000000ull);
	enc.add(-1);
	enc.add(MUNUS_ONE_HUNDRED);
	enc.add(-100);
	enc.add(-1000);
	enc.add("aaa");
	const char* bbb = "bbb";
	enc.add(bbb);
	// Add array.
	enc.add(std::make_tuple(1., 2.f, "test", nullptr, false));
	// Add map.
	enc.add(mpp::as_map(std::forward_as_tuple(10, true, 11, "val", 12,
					   std::make_tuple(1, 2, 3))));

	for (auto itr = buf.begin(); itr != buf.end(); ++itr) {
		char c = buf.get<uint8_t>(itr);
		uint8_t u = c;
		const char *h = "0123456789ABCDEF";
		if (c >= 'a' && c <= 'z')
			std::cout << c;
		else
			std::cout << h[u / 16] << h[u % 16];
	}
	std::cout << std::endl;

	mpp::Dec<Buf_t> dec(buf);
	{
		int val = 15478;
		dec.SetReader(false, mpp::SimpleReader<Buf_t, mpp::MP_UINT, int>{val});
		mpp::ReadResult_t res = dec.Read();
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(val != 0);
	}
	{
		int val = 15478;
		dec.SetReader(false, mpp::SimpleReader<Buf_t, mpp::MP_AINT, int>{val});
		mpp::ReadResult_t res = dec.Read();
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(val != 10);
	}
	{
		unsigned short val = 15478;
		mpp::SimpleReader<Buf_t, mpp::MP_ANUM, unsigned short> r{val};
		dec.SetReader(false, r);
		mpp::ReadResult_t res = dec.Read();
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(val != 200);
	}
	for (uint64_t exp : {2000ull, 2000000ull, 4000000000ull, 4000000000ull, 20000000000ull})
	{
		uint64_t val = 15478;
		dec.SetReader(false, mpp::SimpleReader<Buf_t, mpp::MP_UINT, uint64_t>{val});
		mpp::ReadResult_t res = dec.Read();
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(val != exp);
	}
	for (int32_t exp : {-1, -100, -100, -1000})
	{
		int32_t val = 15478;
		dec.SetReader(false, mpp::SimpleReader<Buf_t, mpp::MP_AINT, int32_t>{val});
		mpp::ReadResult_t res = dec.Read();
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(val != exp);
	}
	{
		constexpr size_t S = 16;
		char str[S];
		size_t size;
		dec.SetReader(false, mpp::SimpleStrReader<Buf_t, S - 1>{str, size});
		mpp::ReadResult_t res = dec.Read();
		str[size] = 0;
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(size != 3);
		fail_if(strcmp(str, "aaa") != 0);
	}
	{
		constexpr size_t S = 16;
		char str[S];
		size_t size;
		dec.SetReader(false, mpp::SimpleStrReader<Buf_t, S - 1>{str, size});
		mpp::ReadResult_t res = dec.Read();
		str[size] = 0;
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(size != 3);
		fail_if(strcmp(str, "bbb") != 0);
	}
	{
		TestArrStruct arr = {};
		dec.SetReader(false, ArrReader{arr, dec});
		mpp::ReadResult_t res = dec.Read();
		fail_if(res != mpp::READ_SUCCESS);
		fail_if(arr.parsed_arr_size != 5);
		fail_if(arr.dbl != 1.);
		fail_if(arr.flt != 2.f);
		fail_if(strcmp(arr.str, "test") != 0);
		fail_if(arr.nil != nullptr);
		fail_if(arr.b != false);

	}
	{
		using namespace example;
		TestMapStruct map = {};
		dec.SetReader(false, MapReader{dec, map});
		mpp::ReadResult_t res = dec.Read();
		fail_unless(res == mpp::READ_SUCCESS);
		fail_unless(map.boo == true);
		fail_unless(strcmp(map.str, "val") == 0);
		fail_unless(map.arr_size == 3);
		fail_unless(map.arr[0] == 1);
		fail_unless(map.arr[1] == 2);
		fail_unless(map.arr[2] == 3);
	}
}

int main()
{
	test_common();
	test_sub_array();
	test_string_traits();
	test_type_visual();
	test_basic();
}
