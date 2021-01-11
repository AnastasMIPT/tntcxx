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
#include "../src/Buffer/Buffer.hpp"
#include "../src/mpp/mpp.hpp"
#include "../src/mpp/Dec.hpp"
#include "../src/Client/ResponseDecoder.hpp"

/** Corresponds to data stored in _space[512]. */
struct UserTuple {
	uint64_t field1;
	std::string field2;
	double field3;
};

std::ostream&
operator<<(std::ostream& strm, const UserTuple &t)
{
	return strm << "Tuple: field1=" << t.field1 << " field2=" << t.field2 <<
		    " field3=" << t.field3;
}

using Buf_t = tnt::Buffer<16 * 1024>;
using BufIter_t = typename Buf_t::iterator;

struct UserTupleValueReader : mpp::DefaultErrorHandler {
	explicit UserTupleValueReader(UserTuple& t) : tuple(t) {}
	static constexpr mpp::Type VALID_TYPES = mpp::MP_UINT | mpp::MP_STR | mpp::MP_DBL;
	template <class T>
	void Value(const BufIter_t&, mpp::compact::Type, T v)
	{
		using A = UserTuple;
		static constexpr std::tuple map(&A::field1, &A::field3);
		auto ptr = std::get<std::decay_t<T> A::*>(map);
		tuple.*ptr = v;
	}
	void Value(const BufIter_t& itr, mpp::compact::Type, mpp::StrValue v)
	{
		BufIter_t tmp = itr;
		tmp += v.offset;
		std::string &dst = tuple.field2;
		while (v.size) {
			dst.push_back(*tmp);
			++tmp;
			--v.size;
		}
	}
	void WrongType(mpp::Type expected, mpp::Type got)
	{
		std::cout << "expected type is " << expected <<
			  " but got " << got << std::endl;
	}

	BufIter_t* StoreEndIterator() { return nullptr; }
	UserTuple& tuple;
};

template <class BUFFER>
struct UserTupleReader : mpp::SimpleReaderBase<BUFFER, mpp::MP_ARR> {

	UserTupleReader(mpp::Dec<BUFFER>& d, UserTuple& t) : dec(d), tuple(t) {}

	void Value(const iterator_t<BUFFER>&, mpp::compact::Type, mpp::ArrValue u)
	{
		assert(u.size == 3);
		dec.SetReader(false, UserTupleValueReader{tuple});
	}
	mpp::Dec<BUFFER>& dec;
	UserTuple& tuple;
};

template <class BUFFER>
std::vector<UserTuple>
decodeUserTuple(BUFFER &buf, Data<BUFFER> &data)
{
	std::vector<UserTuple> results;
	for(auto const& t: data.tuples) {
		assert(t.begin != std::nullopt);
		assert(t.end != std::nullopt);
		UserTuple tuple;
		mpp::Dec dec(buf);
		dec.SetPosition(*t.begin);
		dec.SetReader(false, UserTupleReader<BUFFER>{dec, tuple});
		mpp::ReadResult_t res = dec.Read();
		assert(res == mpp::READ_SUCCESS);
		results.push_back(tuple);
	}
	return results;
}
