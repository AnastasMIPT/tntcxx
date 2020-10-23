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
#include <any>
#include <cstdint>
#include <map>
#include <tuple>

#include "IprotoConstants.hpp"
#include "../mpp/mpp.hpp"
#include "../Utils/Logger.hpp"

enum IteratorType {
	EQ = 0,
	REQ = 1,
	ALL = 2,
	LT = 3,
	LE = 4,
	GE = 5,
	GT = 6,
	BITS_ALL_SET = 7,
	BITS_ANY_SET = 8,
	BITS_ALL_NOT_SET = 9,
	OVERLAPS = 10,
	NEIGHBOR = 11,
};

template<class BUFFER>
using iterator_t = typename BUFFER::iterator;

template<class BUFFER>
class RequestEncoder {
public:
	RequestEncoder(BUFFER &buf) : m_Buf(buf), m_Enc(buf) {};
	~RequestEncoder() { };
	RequestEncoder() = delete;
	RequestEncoder(const RequestEncoder& encoder) = delete;
	RequestEncoder& operator = (const RequestEncoder& encoder) = delete;

	size_t encodePing();
	template <typename... Args>
	size_t encodeReplace(uint32_t space_id, const std::tuple<Args...> &tuple);
	template <typename... Args>
	size_t encodeSelect(uint32_t space_id, uint32_t index_id,
			  uint32_t limit, uint32_t offset,
			  IteratorType iterator, const std::tuple<Args...> &key);

	/** Sync value is used as request id. */
	static size_t getSync() { return sync; }
private:
	void encodeHeader(int request);
	BUFFER &m_Buf;
	mpp::Enc<BUFFER> m_Enc;
	inline static ssize_t sync = -1;
};

template<class BUFFER>
void
RequestEncoder<BUFFER>::encodeHeader(int request)
{
	//TODO: add schema version.
	auto header = std::make_tuple(
		Iproto::SYNC, ++RequestEncoder::sync,
		Iproto::REQUEST_TYPE, request);
	m_Enc.add(mpp::as_map(header));
}

template<class BUFFER>
size_t
RequestEncoder<BUFFER>::encodePing()
{
	iterator_t<BUFFER> request_start = m_Buf.end();
	m_Buf.addBack('\xce');
	iterator_t<BUFFER> request_sz_itr = m_Buf.end();
	m_Buf.addBack(uint32_t{0});
	iterator_t<BUFFER> request_header = m_Buf.end();
	encodeHeader(Iproto::PING);
	auto body = std::make_tuple();
	m_Enc.add(mpp::as_map(body));
	uint32_t request_size = m_Buf.end() - request_header;
	LOG_DEBUG("request size %d", request_size);
	m_Buf.set(request_sz_itr, __builtin_bswap32(request_size));
	LOG_DEBUG("package size %d", m_Buf.end() - request_start);
	return m_Buf.end() - request_start;
}

template<class BUFFER>
template <typename... Args>
size_t
RequestEncoder<BUFFER>::encodeReplace(uint32_t space_id,
				      const std::tuple<Args...> &tuple)
{
	iterator_t<BUFFER> request_start = m_Buf.end();
	m_Buf.addBack('\xce');
	iterator_t<BUFFER> request_sz_itr = m_Buf.end();
	m_Buf.addBack(uint32_t{0});
	iterator_t<BUFFER> request_header = m_Buf.end();
	encodeHeader(Iproto::REPLACE);
	auto body = std::make_tuple(
		Iproto::SPACE_ID, space_id,
		Iproto::TUPLE, tuple);
	m_Enc.add(mpp::as_map(body));
	uint32_t request_size = m_Buf.end() - request_header;
	LOG_DEBUG("request size %d", request_size);
	m_Buf.set(request_sz_itr, __builtin_bswap32(request_size));
	LOG_DEBUG("package size %d", m_Buf.end() - request_start);
	return m_Buf.end() - request_start;
}

template<class BUFFER>
template <typename... Args>
size_t
RequestEncoder<BUFFER>::encodeSelect(uint32_t space_id, uint32_t index_id,
				     uint32_t limit, uint32_t offset,
				     IteratorType iterator,
				     const std::tuple<Args...> &key)
{
	m_Buf.addBack('\xce');
	iterator_t<BUFFER> request_sz_itr = m_Buf.end();
	m_Buf.addBack(uint32_t{0});
	iterator_t<BUFFER> request_start = m_Buf.end();
	encodeHeader(Iproto::SELECT);
	auto body = std::make_tuple(
		Iproto::SPACE_ID, space_id,
		Iproto::INDEX_ID, index_id,
		Iproto::LIMIT, limit,
		Iproto::OFFSET, offset,
		Iproto::ITERATOR, iterator,
		Iproto::KEY, key);
	m_Enc.add(mpp::as_map(body));
	uint32_t request_size = m_Buf.end() - request_start;
	m_Buf.set(request_sz_itr, __builtin_bswap32(request_size));
	return request_size;
}
