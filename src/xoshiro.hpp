#ifndef XOSHIRO_HPP__
#define XOSHIRO_HPP__
#include <cstdint>
#include <random>
#include <algorithm>
#include <functional>
#include <limits>
using std::uint64_t;
inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}
inline uint64_t _splitmix64_fwd(uint64_t& x) {
	uint64_t z = (x += (0x9E3779B97F4A7C15));
	z = (z ^ (z >> 30)) * (0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * (0x94D049BB133111EB);
	return z ^ (z >> 31);
}
struct alignas(32) xoshiro_256{
	using result_type = uint64_t;
	//xoshiro_256(const xoshiro_256&) = default;
public:
#ifdef ENABLE_AVX2
	union{
#endif
	uint64_t s[4];
#ifdef ENABLE_AVX2
	__m256i vs;
	} us;
	uint64_t* s = us.s;
#endif
	
	uint64_t next() {
		const uint64_t result = rotl(s[1] * 5, 7) * 9;
		const uint64_t t = s[1] << 17;
		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];
		s[2] ^= t;
		s[3] = rotl(s[3], 45);
		return result;
	}


	/* This is the jump function for the generator. It is equivalent
	   to 2^128 calls to next(); it can be used to generate 2^128
	   non-overlapping subsequences for parallel computations. */
	public:
	inline void jump() {
		static const uint64_t JUMP[] = { 0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c };

		uint64_t s0 = 0;
		uint64_t s1 = 0;
		uint64_t s2 = 0;
		uint64_t s3 = 0;
		for(unsigned i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
			for(unsigned b = 0; b < 64; b++) {
				if (JUMP[i] & UINT64_C(1) << b) {
					s0 ^= s[0];
					s1 ^= s[1];
					s2 ^= s[2];
					s3 ^= s[3];
				}
				next();
			}

		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
	}



	/* This is the long-jump function for the generator. It is equivalent to
	   2^192 calls to next(); it can be used to generate 2^64 starting points,
	   from each of which jump() will generate 2^64 non-overlapping
	   subsequences for parallel distributed computations. */

	inline void long_jump() {
		static const uint64_t LONG_JUMP[] = { 0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };

		uint64_t s0 = 0;
		uint64_t s1 = 0;
		uint64_t s2 = 0;
		uint64_t s3 = 0;
		for(unsigned i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
			for(unsigned b = 0; b < 64; b++) {
				if (LONG_JUMP[i] & UINT64_C(1) << b) {
					s0 ^= s[0];
					s1 ^= s[1];
					s2 ^= s[2];
					s3 ^= s[3];
				}
				next();
			}

		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
	}
public:
	inline uint64_t operator()(){
		return next();
	}
	inline xoshiro_256(){
		std::random_device dev;
		using ft = std::random_device::result_type;
		ft* begin = (ft*)std::begin(s);
		ft* end = (ft*)std::end(s);
		std::generate(begin, end, std::ref(dev));
	}
	inline xoshiro_256(xoshiro_256& xo) {
		s[0] = xo.s[0];
		s[1] = xo.s[1];
		s[2] = xo.s[2];
		s[3] = xo.s[3];
	}
	inline xoshiro_256(const xoshiro_256& xo) {
		s[0] = xo.s[0];
		s[1] = xo.s[1];
		s[2] = xo.s[2];
		s[3] = xo.s[3];
	}
	inline xoshiro_256(uint64_t s0){
		s[0] = _splitmix64_fwd(s0);
		s[1] = _splitmix64_fwd(s0);
		s[2] = _splitmix64_fwd(s0);
		s[3] = _splitmix64_fwd(s0);
	}
	inline xoshiro_256(uint64_t s0, uint64_t s1, uint64_t s2, uint64_t s3){
		s[0] = s0;
		s[1] = s1;
		s[2] = s2;
		s[3] = s3;
	}
	template<typename Sseq>
	inline xoshiro_256(Sseq& seed_seq){
		seed_seq.generate(s, s + 4);
	}
	inline void seed(uint64_t s0){
		s[0] = _splitmix64_fwd(s0);
		s[1] = _splitmix64_fwd(s0);
		s[2] = _splitmix64_fwd(s0);
		s[3] = _splitmix64_fwd(s0);
	}
	template<typename Sseq>
	inline void seed(Sseq& seed_seq){
		seed_seq.generate(s, s + 4);
	}
	constexpr static result_type min(){
		return std::numeric_limits<result_type>::min();
	}
	constexpr static result_type max(){
		return std::numeric_limits<result_type>::max();
	}
};


struct alignas(32) xoshiro_128{
	using result_type = uint64_t;
private:
	uint64_t s[2];

	inline uint64_t next() {
		const uint64_t s0 = s[0];
		uint64_t s1 = s[1];
		const uint64_t result = rotl(s0 * 5, 7) * 9;
		s1 ^= s0;
		s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
		s[1] = rotl(s1, 37); // c
		return result;
	}



	 /* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */
	void jump(void) {
	 	static const uint64_t JUMP[] = { 0xdf900294d8f554a5, 0x170865df4b3201fc };

	 	uint64_t s0 = 0;
	 	uint64_t s1 = 0;
	 	for(unsigned i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
	 		for(unsigned b = 0; b < 64; b++) {
	 			if (JUMP[i] & UINT64_C(1) << b) {
	 				s0 ^= s[0];
	 				s1 ^= s[1];
	 			}
	 			next();
	 		}

	 	s[0] = s0;
	 	s[1] = s1;
	 }




	 /* This is the long-jump function for the generator. It is equivalent to
    2^96 calls to next(); it can be used to generate 2^32 starting points,
    from each of which jump() will generate 2^32 non-overlapping
    subsequences for parallel distributed computations. */

 void long_jump(void) {
 	static const uint64_t LONG_JUMP[] = { 0xd2a98b26625eee7b, 0xdddf9b1090aa7ac1 };

 	uint64_t s0 = 0;
 	uint64_t s1 = 0;
 	for(unsigned i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
 		for(int b = 0; b < 64; b++) {
 			if (LONG_JUMP[i] & UINT64_C(1) << b) {
 				s0 ^= s[0];
 				s1 ^= s[1];
 			}
 			next();
 		}

 	s[0] = s0;
 	s[1] = s1;
 }

public:
	inline uint64_t operator()(){
		return next();
	}
	inline xoshiro_128(){
		std::random_device dev;
		using ft = std::random_device::result_type;
		ft* begin = (ft*)std::begin(s);
		ft* end = (ft*)std::end(s);
		std::generate(begin, end, std::ref(dev));
	}
	inline xoshiro_128(uint64_t s0){
		s[0] = _splitmix64_fwd(s0);
		s[1] = _splitmix64_fwd(s0);
	}
	inline xoshiro_128(uint64_t s0, uint64_t s1){
		s[0] = s0;
		s[1] = s1;
	}
	template<typename Sseq>
	inline xoshiro_128(Sseq& seed_seq){
		seed_seq.generate(s, s + 2);
	}
	inline void seed(uint64_t s0){
		s[0] = _splitmix64_fwd(s0);
		s[1] = _splitmix64_fwd(s0);
	}
	template<typename Sseq>
	inline void seed(Sseq& seed_seq){
		seed_seq.generate(s, s + 2);
	}
	result_type min(){
		return std::numeric_limits<result_type>::min();
	}
	result_type max(){
		return std::numeric_limits<result_type>::max();
	}
};

#endif // !XOSHIRO_HPP__
