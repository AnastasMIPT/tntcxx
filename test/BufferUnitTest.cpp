#include <sys/uio.h> /* struct iovec */
#include <iostream>

#include "../src/Buffer/Buffer.hpp"

template<size_t N>
struct Announcer
{
	Announcer(const char *testName) : m_testName(testName) {
		std::cout << "*** TEST " << m_testName << "<" << N << ">" <<
		" started... ***" << std::endl;
	}
	~Announcer() {
		std::cout << "*** TEST " << m_testName << "<" << N << ">" <<
		": done" << std::endl;
	}
	const char *m_testName;
};

#define TEST_INIT() Announcer<N> _Ann(__func__)

#define fail(expr, result) do {						      \
	std::cerr << "Test failed: " << expr << " is " << result << " at " << \
	__FILE__ << ":" << __LINE__ << " in test " << __func__ << std::endl;  \
	exit(-1);							      \
} while (0)

#define fail_if(expr) if (expr) fail(#expr, "true")
#define fail_unless(expr) if (!(expr)) fail(#expr, "false")

constexpr static size_t SMALL_BLOCK_SZ = 32;
constexpr static size_t LARGE_BLOCK_SZ = 104;

static char char_samples[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

constexpr static int SAMPLES_CNT = sizeof(char_samples);

static int int_sample = 666;

static double double_sample = 66.6;

struct struct_sample {
	int i;
	char c;
	double d;
} struct_sample = {1, '1', 1.1};

static char end_marker = '#';

template<size_t N>
static void
fillBuffer(tnt::Buffer<N> &buffer, size_t size)
{
	for (size_t i = 0; i < size; ++i)
		buffer.template addBack<char>(std::move(char_samples[i % SAMPLES_CNT]));
}

template<size_t N>
static void
eraseBuffer(tnt::Buffer<N> &buffer)
{
	int IOVEC_MAX = 1024;
	struct iovec vec[IOVEC_MAX];
	do {
		size_t vec_size = buffer.getIOV(buffer.begin(), vec, IOVEC_MAX);
		buffer.dropFront(vec_size);
	} while (!buffer.empty());
}

/**
 * Dump buffer to @output string with human readable format.
 * Not the fastest, but quite elementary implementation.
 */
template<size_t N>
static void
dumpBuffer(tnt::Buffer<N> &buffer, std::string &output)
{
	size_t vec_len = 0;
	int IOVEC_MAX = 1024;
	size_t block_cnt = 0;
	struct iovec vec[IOVEC_MAX];
	for (auto itr = buffer.begin(); itr != buffer.end(); itr += vec_len) {
		size_t vec_cnt = buffer.getIOV(buffer.begin(), vec, IOVEC_MAX);
		for (size_t i = 0; i < vec_cnt; ++i) {
			output.append("|sz=" + std::to_string(vec[i].iov_len) + "|");
			output.append((const char *) vec[i].iov_base,
				      vec[i].iov_len);
			output.append("|");
			vec_len += vec[i].iov_len;
		}
		block_cnt += vec_cnt;
	}
	output.insert(0, "bcnt=" + std::to_string(block_cnt));
}

template<size_t N>
static void
printBuffer(tnt::Buffer<N> &buffer)
{
	std::string str;
	dumpBuffer(buffer, str);
	std::cout << "Buffer:" << str << std::endl;
}

/**
 * AddBack() + dropBack()/dropFront() combinations.
 */
template<size_t N>
void
buffer_basic()
{
	TEST_INIT();
	tnt::Buffer<N> buf;
	fail_unless(buf.empty());
	size_t sz = buf.template addBack<int>(std::move(int_sample));
	fail_unless(! buf.empty());
	fail_unless(sz == sizeof(int));
	auto itr = buf.begin();
	int int_res = -1;
	buf.template get<int>(itr, int_res);
	fail_unless(int_res == int_sample);
	itr.~iterator();
	buf.dropBack(sz);
	fail_unless(buf.empty());
	/* Test non-template ::addBack() method. */
	buf.addBack((const char *)&char_samples, SAMPLES_CNT);
	fail_unless(! buf.empty());
	char char_res[SAMPLES_CNT];
	itr = buf.begin();
	buf.get(itr, (char *)&char_res, SAMPLES_CNT);
	for (int i = 0; i < SAMPLES_CNT; ++i)
		fail_unless(char_samples[i] == char_res[i]);
	itr.~iterator();
	buf.dropFront(SAMPLES_CNT);
	fail_unless(buf.empty());
	/* Add double value in buffer. */
	itr = buf.appendBack(sizeof(double));
	buf.set(itr, std::move(double_sample));
	double double_res = 0;
	buf.get(itr, double_res);
	fail_unless(double_res == double_sample);
	itr.~iterator();
	buf.dropFront(sizeof(double));
	fail_unless(buf.empty());
	/* Add struct value in buffer. */
	itr = buf.appendBack(sizeof(struct_sample));
	buf.set(itr, std::move(struct_sample));
	struct struct_sample struct_res = { };
	buf.get(itr, struct_res);
	fail_unless(struct_res.c == struct_sample.c);
	fail_unless(struct_res.i == struct_sample.i);
	fail_unless(struct_res.d == struct_sample.d);
	itr.~iterator();
	buf.dropFront(sizeof(struct_sample));
	fail_unless(buf.empty());
}

template<size_t N>
void
buffer_iterator()
{
	TEST_INIT();
	tnt::Buffer<N> buf;
	fillBuffer(buf, SAMPLES_CNT);
	buf.addBack(std::move(end_marker));
	auto itr = buf.begin();
	char res = 'x';
	/* Iterator to the start of buffer should not change. */
	for (int i = 0; i < SAMPLES_CNT; ++i) {
		buf.template get<char>(itr, res);
		fail_unless(res == char_samples[i]);
		++itr;
	}
	buf.template get<char>(itr, res);
	fail_unless(res == end_marker);
	auto begin = buf.begin();
	while (begin != itr)
		begin += 1;
	res = 'x';
	buf.template get<char>(begin, res);
	fail_unless(res == end_marker);
	buf.dropFront(SAMPLES_CNT);
	auto end = buf.end();
	fail_unless(end != itr);
	fail_unless(end != begin);
	++itr;
	fail_unless(end == itr);
	itr.~iterator();
	begin.~iterator();
	end.~iterator();
	buf.dropBack(1);
	fail_unless(buf.empty());
}

template <size_t N>
void
buffer_insert()
{
	TEST_INIT();
	tnt::Buffer<N> buf;
	fillBuffer(buf, SAMPLES_CNT);
	buf.addBack(std::move(end_marker));
	auto begin = buf.begin();
	auto mid_itr = buf.end();
	auto mid_itr2 = buf.end();
	fillBuffer(buf, SAMPLES_CNT);
	buf.addBack(std::move(end_marker));
	auto end_itr = buf.end();
	/* For SMALL_BLOCK_SZ = 32
	 * Buffer:bcnt=3|sz=8|01234567||sz=8|89#01234||sz=6|56789#|
	 *                                      ^
	 *                                   mid_itr
	 * */
	buf.insert(mid_itr, SMALL_BLOCK_SZ / 2);
	char res = 'x';
	mid_itr += SMALL_BLOCK_SZ / 2;
	for (int i = 0; i < SAMPLES_CNT / 2; ++i) {
		buf.template get<char>(mid_itr, res);
		fail_unless(res == char_samples[i]);
		++mid_itr;
	}
	mid_itr2 += SMALL_BLOCK_SZ / 2;
	for (int i = 0; i < SAMPLES_CNT / 2; ++i) {
		buf.template get<char>(mid_itr2, res);
		fail_unless(res == char_samples[i]);
		++mid_itr2;
	}
	begin.~iterator();
	mid_itr.~iterator();
	mid_itr2.~iterator();
	end_itr.~iterator();
	eraseBuffer(buf);
	/* Try the same but with more elements in buffer (i.e. more blocks). */
	fillBuffer(buf, SAMPLES_CNT * 2);
	//buf.addBack(std::move(end_marker));
	mid_itr = buf.end();
	fillBuffer(buf, SAMPLES_CNT * 4);
	mid_itr2 = buf.end();
	buf.addBack(std::move(end_marker));
	fillBuffer(buf, SAMPLES_CNT * 4);
	end_itr = buf.end();
	buf.addBack(std::move(end_marker));
	fillBuffer(buf, SAMPLES_CNT * 2);
	buf.addBack(std::move(end_marker));
	buf.insert(mid_itr, SAMPLES_CNT * 3);
	buf.template get<char>(end_itr, res);
	fail_unless(res == end_marker);
	buf.template get<char>(mid_itr2, res);
	fail_unless(res == end_marker);
	/*
	 * Buffer content prior to the iterator used to process insertion
	 * should remain unchanged.
	 */
	int i = 0;
	for (auto tmp = buf.begin(); tmp < mid_itr; ++tmp) {
		buf.template get<char>(tmp, res);
		fail_unless(res == char_samples[i++ % SAMPLES_CNT]);
	}
}

template <size_t N>
void
buffer_release()
{
	TEST_INIT();
	tnt::Buffer<N> buf;
	fillBuffer(buf, SAMPLES_CNT);
	buf.addBack(std::move(end_marker));
	auto begin = buf.begin();
	auto mid_itr = buf.end();
	auto mid_itr2 = buf.end();
	fillBuffer(buf, SAMPLES_CNT);
	buf.addBack(std::move(end_marker));
	auto end_itr = buf.end();
	/* For SMALL_BLOCK_SZ = 32
	 * Buffer:|sz=8|01234567||sz=8|89#01234||sz=6|56789#|
	 *                                ^                 ^
	 *                              mid_itr          end_itr
	 */
	buf.release(mid_itr, SAMPLES_CNT / 2);
	/* Buffer:|sz=8|01234567||sz=8|89#56789||sz=1|#|
	 *                                ^            ^
	 *                              mid_itr     end_itr
	 */
	char res = 'x';
	for (int i = 0; i < SAMPLES_CNT / 2; ++i) {
		buf.template get<char>(mid_itr, res);
		fail_unless(res == char_samples[i + SAMPLES_CNT / 2]);
		++mid_itr;
	}
	for (int i = 0; i < SAMPLES_CNT / 2; ++i) {
		buf.template get<char>(mid_itr2, res);
		fail_unless(res == char_samples[i + SAMPLES_CNT / 2]);
		++mid_itr2;
	}
	fail_unless(++mid_itr == end_itr);
	mid_itr.~iterator();
	mid_itr2.~iterator();
	end_itr.~iterator();
	begin.~iterator();
	eraseBuffer(buf);
	/* Try the same but with more elements in buffer (i.e. more blocks). */
	fillBuffer(buf, SAMPLES_CNT * 2);
	mid_itr = buf.end();
	fillBuffer(buf, SAMPLES_CNT * 4);
	mid_itr2 = buf.end();
	buf.addBack(std::move(end_marker));
	fillBuffer(buf, SAMPLES_CNT * 4);
	end_itr = buf.end();
	buf.addBack(std::move(end_marker));
	fillBuffer(buf, SAMPLES_CNT * 2);
	buf.addBack(std::move(end_marker));
	buf.release(mid_itr, SAMPLES_CNT * 3);
	buf.template get<char>(end_itr, res);
	fail_unless(res == end_marker);
	buf.template get<char>(mid_itr2, res);
	fail_unless(res == end_marker);
	/*
	 * Buffer content prior to the iterator used to process insertion
	 * should remain unchanged.
	 */
	int i = 0;
	for (auto tmp = buf.begin(); tmp < mid_itr; ++tmp) {
		buf.template get<char>(tmp, res);
		fail_unless(res == char_samples[i++ % SAMPLES_CNT]);
	}
}

/**
 * Complex test emulating IPROTO interaction.
 */
template<size_t N>
void
buffer_out()
{
	TEST_INIT();
	tnt::Buffer<N> buf;
	buf.template addBack<char>(0xce); // uin32 tag
	auto save = buf.appendBack(4); // uint32, will be set later
	size_t total = buf.template addBack<char>(0x82); // map(2) - header
	total += buf.template addBack<char>(0x00); // IPROTO_REQUEST_TYPE
	total += buf.template addBack<char>(0x01); // IPROTO_SELECT
	total += buf.template addBack<char>(0x01); // IPROTO_SYNC
	total += buf.template addBack<char>(0x00); // sync = 0
	total += buf.template addBack<char>(0x82); // map(2) - body
	total += buf.template addBack<char>(0x10); // IPROTO_SPACE_ID
	total += buf.template addBack<char>(0xcd); // uint16 tag
	total += buf.template addBack(__builtin_bswap16(512)); // space_id = 512
	total += buf.template addBack<char>(0x20); // IPROTO_KEY
	total += buf.template addBack<char>(0x90); // empty array key
	buf.set(save, __builtin_bswap32(total)); // set calculated size
	save.~iterator();
	do {
		int IOVEC_MAX = 1024;
		struct iovec vec[IOVEC_MAX];
		size_t vec_size = buf.getIOV(buf.begin(), vec, IOVEC_MAX);
		buf.dropFront(vec_size);
	} while (!buf.empty());
}

int main()
{
	buffer_basic<SMALL_BLOCK_SZ>();
	buffer_basic<LARGE_BLOCK_SZ>();
	buffer_iterator<SMALL_BLOCK_SZ>();
	buffer_iterator<LARGE_BLOCK_SZ>();
	buffer_insert<SMALL_BLOCK_SZ>();
	buffer_insert<LARGE_BLOCK_SZ>();
	buffer_release<SMALL_BLOCK_SZ>();
	buffer_release<LARGE_BLOCK_SZ>();
	buffer_out<SMALL_BLOCK_SZ>();
	buffer_out<LARGE_BLOCK_SZ>();
}
