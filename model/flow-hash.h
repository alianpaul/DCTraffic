#ifndef FLOW_HASH_H
#define FLOW_HASH_H

namespace ns3
{

#define ROT32(x, y) ((x << y) | (x >> (32 - y))) // avoid effort

/* The this function is copied from LossRadar simple_switch.cpp.
 * LossRadar is the same author with FlowRadar.https://github.com/USC-NSL/
 * The orginal code is from wikipedia: https://en.wikipedia.org/wiki/MurmurHash
 */
inline uint32_t murmur3_32(const char *key, uint32_t len, uint32_t seed) {
	static const uint32_t c1 = 0xcc9e2d51;
	static const uint32_t c2 = 0x1b873593;
	static const uint32_t r1 = 15;
	static const uint32_t r2 = 13;
	static const uint32_t m = 5;
	static const uint32_t n = 0xe6546b64;

	uint32_t hash = seed;

	const int nblocks = len / 4;
	const uint32_t *blocks = (const uint32_t *) key;
	int i;
	uint32_t k;
	for (i = 0; i < nblocks; i++) {
		k = blocks[i];
		k *= c1;
		k = ROT32(k, r1);
		k *= c2;

		hash ^= k;
		hash = ROT32(hash, r2) * m + n;
	}

	const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
	uint32_t k1 = 0;

	switch (len & 3) {
		case 3:
			k1 ^= tail[2] << 16;
		case 2:
			k1 ^= tail[1] << 8;
		case 1:
			k1 ^= tail[0];

			k1 *= c1;
			k1 = ROT32(k1, r1);
			k1 *= c2;
			hash ^= k1;
	}

	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);

	return hash;
}
/*
struct my_hash1 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 1);
  }
};

struct my_hash2 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 2);
  }
};

struct my_hash3 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 3);
  }
};

struct my_hash4 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 4);
  }
};

struct my_hash5 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 5);
  }
};

struct my_hash6 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 6);
  }
};

struct my_hash7 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 7);
  }
};

struct my_hash8 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 8);
  }
};

struct my_hash9 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 9);
  }
};

struct my_hash10 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 10);
  }
};

struct my_hash11 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 11);
  }
};

struct my_hash12 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 12);
  }
};

struct my_hash13 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 13);
  }
};

struct my_hash14 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 14);
  }
};

struct my_hash15 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 15);
  }
};

struct my_hash16 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 16);
  }
};

struct my_hash17 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 17);
  }
};

struct my_hash18 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 18);
  }
};

struct my_hash19 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 19);
  }
};

struct my_hash20 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 20);
  }
};

struct my_hash21 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 21);
  }
};

struct my_hash22 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 22);
  }
};

struct my_hash23 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 23);
  }
};

struct my_hash24 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 24);
  }
};

struct my_hash25 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 25);
  }
};

struct my_hash26 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 26);
  }
};

struct my_hash27 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 27);
  }
};

struct my_hash28 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 28);
  }
};

struct my_hash29 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 29);
  }
};

struct my_hash30 {
  uint32_t operator()(const char *buf, size_t s) const {
    return murmur3_32(buf, s, 30);
  }
};
*/  
}

#endif
