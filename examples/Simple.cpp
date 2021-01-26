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

#include "../src/Client/Connector.hpp"
#include "../src/Buffer/Buffer.hpp"

const char *address = "127.0.0.1";
int port = 3301;
int WAIT_TIMEOUT = 1000; //milliseconds

using Buf_t = tnt::Buffer<16 * 1024>;
using Net_t = DefaultNetProvider<Buf_t >;

template<class BUFFER>
void
printReponse(Response<BUFFER> &response)
{
	if (response.body.error_stack != std::nullopt) {
		Error err = (*response.body.error_stack).error;
		std::cout << "RESPONSE: msg=" << err.msg << " line=" << err.file <<
			  " file=" << err.file << " errno=" << err.saved_errno <<
			  " type=" << err.type_name << " code=" << err.errcode <<
			  std::endl;
	}
	if (response.body.data != std::nullopt) {
		Tuple<BUFFER> t = response.body.data->tuple;
		std::cout << "RESPONSE: tuple with field count=" <<
			  t.field_count << std::endl;
	}
}

int
main()
{
	/*
	 * Create default connector - it'll handle many connections
	 * asynchronously.
	 */
	Connector<Buf_t, Net_t> client;
	/*
	 * Create single connection. Constructor takes only client reference.
	 */
	Connection<Buf_t, Net_t> conn(client);
	/*
	 * Try to connect to given address:port. Current implementation is
	 * exception free, so we rely only on return codes.
	 */
	int rc = client.connect(conn, address, port);
	if (rc != 0) {
		assert(conn.status.is_failed);
		std::cerr << conn.getError() << std::endl;
		return -1;
	}
	/*
	 * Now let's execute several requests: ping, replace and select.
	 * Note that any of :request() methods can't fail; they always
	 * return request id - the future (number) which is used to get
	 * response once it is received. Also note that at this step,
	 * requests are encoded (into msgpack format) and saved into
	 * output connection's buffer - they are ready to be sent.
	 * But network communication itself will be done later.
	 */
	/* PING */
	rid_t ping = conn.ping();
	/* REPLACE - equals to space:replace(pk_value, "111", 1)*/
	uint32_t space_id = 512;
	int pk_value = 666;
	std::tuple data = std::make_tuple(pk_value /* field 1*/, "111" /* field 2*/, 1 /* field 3*/);
	rid_t replace = conn.replace(space_id, data);
	/* SELECT - equals to space.index[0]:select({pk_value}, {limit = 1})*/
	uint32_t space_id = 512;
	uint32_t index_id = 0;
	uint32_t limit = 1;
	uint32_t offset = 0;
	IteratorType iter = IteratorType::EQ;
	std::tuple key = std::make_tuple(pk_value);
	rid_t select = conn.select(space_id, index_id, limit, offset, iter, key);
	/*
	 * Now let's send our requests to the server. There are two options
	 * for single connection: we can either wait for one specific
	 * future or for all at once. Let's try both variants.
	 */
	while (conn.futureIsReady(ping)) {
		/*
		 * wait() is the main function responsible for sending/receiving
		 * requests and implements event-loop under the hood. It may
		 * fail due to several reasons:
		 *  - connection is timed out;
		 *  - connection is broken (e.g. closed);
		 *  - epoll is failed.
		 */
		if (client.wait(conn, ping, WAIT_TIMEOUT) != 0) {
			assert(conn.status.is_failed);
			std::cerr << conn.getError() << std::endl;
			conn.reset();
		}
	}
	/* Now let's get response using our future.*/
	std::optional<Response<Buf_t>> response = conn.getResponse(ping);
	/*
	 * Since conn.futureIsReady(ping) returned <true>, then response
	 * must be ready.
	 */
	assert(response != std::nullopt);
	/*
	 * If request is successfully executed on server side, response
	 * will contain data (i.e. tuple being replaced in case of :replace()
	 * request or tuples satisfying search conditions in case of :select();
	 * responses for pings contain nothing - empty map).
	 * To tell responses containing data from error responses, one can
	 * rely on response code storing in the header or check
	 * Response->body.data and Response->body.error_stack members.
	 */
	printReponse(response);
	/* Let's wait for both futures at once. */
	rid_t futures[2];
	futures[0] = replace;
	futures[1] = select;
	/* No specified timeout means that we poll futures until they are ready.*/
	if (client.waitAll(conn, (rid_t *) &features, 2) != 0) {
		assert(conn.status.is_failed);
		std::cerr << conn.getError() << std::endl;
		client.close(conn);
		return -1;
	}
	for (int i = 0; i < 2; ++i) {
		assert(conn.futureIsReady(features[i]));
		response = conn.getResponse(features[i]);
		assert(response != std::nullopt);
		printReponse(response);
	}
	/* Now create another one connection. */
	Connection<Buf_t, Net_t> another(client);
	if (client.connect(another, address, port) != 0) {
		assert(conn.status.is_failed);
		std::cerr << conn.getError() << std::endl;
		return -1;
	}
	/* Simultaneously execute two requests from different connections. */
	rid_t f1 = conn.ping();
	rid_t f2 = another.ping();
	/*
	 * waitAny() returns the first connection received response.
	 * All connections registered via :connect() call are participating.
	 */
	Connection<Buf_t, Net_t> *first = client.waitAny(WAIT_TIMEOUT);
	if (first == &conn) {
		assert(conn.futureIsReady(f1));
	} else {
		assert(another.futureIsReady(f2));
	}
	/* Finally, user is responsible for closing connections. */
	client.close(conn);
	client.close(another);
	return 0;
}
