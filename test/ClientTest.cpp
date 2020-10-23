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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <sys/prctl.h>

#include <iostream>

#include "Helpers.hpp"

#include "../src/Client/Connector.hpp"
#include "../src/Buffer/Buffer.hpp"

const char *localhost = "127.0.0.1";
//FIXME: in case of pre-installed tarantool path is not required.
const char *tarantool_path = "/home/nikita/tarantool/src/tarantool";

int WAIT_TIMEOUT = 1000; //milliseconds

int
launchTarantool()
{
	pid_t ppid_before_fork = getpid();
	pid_t pid = fork();
	if (pid == -1) {
		fprintf(stderr, "Can't launch Tarantool: fork failed! %s",
			strerror(errno));
		return -1;
	}
	if (pid == 0) {
		//int status;
		//waitpid(pid, &status, 0);
		return 0;
	}
	/* Kill child (i.e. Tarantool process) when the test is finished. */
	if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
		fprintf(stderr, "Can't launch Tarantool: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (getppid() != ppid_before_fork) {
		fprintf(stderr, "Can't launch Tarantool: parent process exited "\
				"just before prctl call");
		exit(EXIT_FAILURE);
	}
	const char * argv[] = {"cfg.lua"};
	if (execv(tarantool_path, (char * const *)argv) == -1) {
		fprintf(stderr, "Can't launch Tarantool: exec failed! %s",
			strerror(errno));
	}
	exit(EXIT_FAILURE);
}

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

using Buf_t = tnt::Buffer<16 * 1024>;
using Net_t = DefaultNetProvider<Buf_t >;

template <class BUFFER>
void
trivial(Connector<BUFFER> &client)
{
	TEST_INIT(0);
	Connection<Buf_t, Net_t> conn(client);
	/* Get nonexistent future. */
	std::optional<Response<Buf_t>> response = conn.getResponse(666);
	fail_unless(response == std::nullopt);
	/* Execute request without connecting to the host. */
	rid_t f = conn.ping();
	client.wait(conn, f, WAIT_TIMEOUT);
	fail_unless(conn.status.is_failed);
	//std::cout << conn.getError() << std::endl;
}

/** Single connection, separate/sequence pings, no errors */
template <class BUFFER>
void
single_conn_ping(Connector<BUFFER> &client)
{
	TEST_INIT(0);
	Connection<Buf_t, Net_t> conn(client);
	int rc = client.connect(conn, localhost, 3301);
	fail_unless(rc == 0);
	rid_t f = conn.ping();
	fail_unless(!conn.futureIsReady(f));
	client.wait(conn, f, WAIT_TIMEOUT);
	fail_unless(conn.futureIsReady(f));
	std::optional<Response<Buf_t>> response = conn.getResponse(f);
	fail_unless(response != std::nullopt);
	fail_unless(response->header.code == 0);
	f = conn.ping();
	client.wait(conn, f, WAIT_TIMEOUT);
	fail_unless(conn.futureIsReady(f));
	/* Second wait() should terminate immediately. */
	client.wait(conn, f, WAIT_TIMEOUT);
	fail_unless(conn.futureIsReady(f));
	response = conn.getResponse(f);
	fail_unless(response != std::nullopt);
	fail_unless(response->header.code == 0);
	/* Many requests at once. */
	rid_t features[3];
	features[0] = conn.ping();
	features[1] = conn.ping();
	features[2] = conn.ping();
	client.waitAll(conn, (rid_t *) &features, 3, WAIT_TIMEOUT);
	for (int i = 0; i < 3; ++i) {
		fail_unless(conn.futureIsReady(features[i]));
		response = conn.getResponse(features[i]);
		fail_unless(response != std::nullopt);
		fail_unless(response->header.code == 0);
		fail_unless(response->body.error_stack == std::nullopt);
	}
	client.close(conn);
}

/** Several connection, separate/sequence pings, no errors */
template <class BUFFER>
void
many_conn_ping(Connector<BUFFER> &client)
{
	TEST_INIT(0);
	Connection<Buf_t, Net_t> conn1(client);
	Connection<Buf_t, Net_t> conn2(client);
	Connection<Buf_t, Net_t> conn3(client);
	int rc = client.connect(conn1, localhost, 3301);
	fail_unless(rc == 0);
	/* Try to connect to the same port */
	rc = client.connect(conn2, localhost, 3301);
	fail_unless(rc == 0);
	/*
	 * Try to re-connect to another address whithout closing
	 * current connection.
	 */
	rc = client.connect(conn2, localhost, 3303);
	fail_unless(rc != 0);
	rc = client.connect(conn3, localhost, 3301);
	fail_unless(rc == 0);
	rid_t f1 = conn1.ping();
	rid_t f2 = conn2.ping();
	rid_t f3 = conn3.ping();
	Connection<Buf_t, Net_t> *conn = client.waitAny(WAIT_TIMEOUT);
	(void) conn;
	fail_unless(conn1.futureIsReady(f1) || conn2.futureIsReady(f2) ||
		    conn3.futureIsReady(f3));
	client.close(conn1);
	client.close(conn2);
	client.close(conn3);
}

/** Single connection, separate replaces */
template <class BUFFER>
void
single_conn_replace(Connector<BUFFER> &client)
{
	TEST_INIT(0);
	Connection<Buf_t, Net_t> conn(client);
	int rc = client.connect(conn, localhost, 3301);
	fail_unless(rc == 0);
	uint32_t space_id = 512;
	std::tuple data = std::make_tuple(666, "111", 1);
	rid_t f1 = conn.replace(space_id, data);
	data = std::make_tuple(777, "asd", 2);
	rid_t f2 = conn.replace(space_id, data);
	client.wait(conn, f1, WAIT_TIMEOUT);
	fail_unless(conn.futureIsReady(f1));
	client.wait(conn, f2, WAIT_TIMEOUT);
	fail_unless(conn.futureIsReady(f2));
	std::optional<Response<Buf_t>> response = conn.getResponse(f1);
	printReponse<BUFFER>(*response);
	fail_unless(response != std::nullopt);
	fail_unless(response->body.data != std::nullopt);
	fail_unless(response->body.error_stack == std::nullopt);
	response = conn.getResponse(f2);
	fail_unless(response != std::nullopt);
	fail_unless(response->body.data != std::nullopt);
	fail_unless(response->body.error_stack == std::nullopt);
	client.close(conn);
}

/** Single connection, select single tuple */
template <class BUFFER>
void
single_conn_select(Connector<BUFFER> &client)
{
	TEST_INIT(0);
	Connection<Buf_t, Net_t> conn(client);
	int rc = client.connect(conn, localhost, 3301);
	fail_unless(rc == 0);
	uint32_t space_id = 512;
	uint32_t index_id = 0;
	uint32_t limit = 1;
	uint32_t offset = 0;
	IteratorType iter = IteratorType::EQ;
	std::tuple key1 = std::make_tuple(666);
	std::tuple key2 = std::make_tuple(777);
	rid_t f1 = conn.select(space_id, index_id, limit, offset, iter, key1);
	rid_t f2 = conn.select(space_id, index_id, limit, offset, iter, key2);
	client.wait(conn, f1, WAIT_TIMEOUT);
	fail_unless(conn.futureIsReady(f1));
	(void) f2;
	//client.wait(conn, f2, WAIT_TIMEOUT);
	//fail_unless(conn.futureIsReady(f2));
	std::optional<Response<Buf_t>> response = conn.getResponse(f1);
	printReponse<BUFFER>(*response);
	fail_unless(response != std::nullopt);
	fail_unless(response->body.data != std::nullopt);
	fail_unless(response->body.error_stack == std::nullopt);
	//response = conn.getResponse(f2);
	//fail_unless(response != std::nullopt);
	//fail_unless(response->body.data != std::nullopt);
	//fail_unless(response->body.error_stack == std::nullopt);
	client.close(conn);
}

int main()
{
//	if (launchTarantool() != 0)
//		return 1;
	Connector<Buf_t> client;
	trivial(client);
	single_conn_ping<Buf_t>(client);
	many_conn_ping<Buf_t>(client);
	single_conn_replace<Buf_t>(client);
	single_conn_select(client);
	return 0;
}