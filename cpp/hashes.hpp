
#ifndef hashes_
#define hashes_
#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <sodium.h>

#pragma GCC visibility push(default)
// Трейт для BLAKE2b (libsodium)
struct Trait_BLAKE2b {
    static constexpr std::size_t hash_size = crypto_generichash_BYTES;
    static constexpr const char* name = "BLAKE2b";
};
// Трейт для SHA-256 (libsodium)
struct Trait_SHA256 {
    static constexpr std::size_t hash_size = crypto_hash_sha256_BYTES;
    static constexpr const char* name = "SHA-256";
};
/*
struct Trait_CRC32 {
    static constexpr std::size_t hash_size = 4; // 32 бита = 4 байта
    static constexpr const char* name = "CRC32";
};
 */



template <typename T>
class UniversalHasher {
public:
    static constexpr std::size_t HASH_SIZE = T::hash_size;
    using HashBuf = std::array<uint8_t, HASH_SIZE>;

    // 1. Одиночное хэширование (когда данные уже собраны в один буфер)
    static HashBuf hash_single(const std::vector<uint8_t>& data)  {
        HashBuf out_hash;

        if constexpr (std::is_same_v<T, Trait_BLAKE2b>) {
            crypto_generichash(out_hash.data(), out_hash.size(),
                data.data(), data.size(),
                nullptr, 0); // Без ключа
        }
        else if constexpr (std::is_same_v<T, Trait_SHA256>) {
            crypto_hash_sha256(out_hash.data(), data.data(), data.size());
        }

        return out_hash;
    }
    static HashBuf hash_single(const uint8_t* data, size_t size) {
        HashBuf out_hash;

        if constexpr (std::is_same_v<T, Trait_BLAKE2b>) {
            crypto_generichash(out_hash.data(), out_hash.size(),
                data, size,
                nullptr, 0); // Без ключа
        }
        else if constexpr (std::is_same_v<T, Trait_SHA256>) {
            crypto_hash_sha256(out_hash.data(), data, size);
        }

        return out_hash;
    }

    // 2. Потоковое хэширование (Multi-part для логов хэндшейка)
    class Stream {
    private:
        crypto_generichash_state blake_state;
        // Здесь можно добавить union или std::variant для состояний других хэшей

    public:
        Stream() {
            if constexpr (std::is_same_v<T, Trait_BLAKE2b>) {
                crypto_generichash_init(&blake_state, nullptr, 0, HASH_SIZE);
            }
            // Инициализация для SHA-256...
        }

        void update(const uint8_t* data, std::size_t len) {
            if constexpr (std::is_same_v<T, Trait_BLAKE2b>) {
                crypto_generichash_update(&blake_state, data, len);
            }
        }

        HashBuf finalize() {
            HashBuf out_hash;
            if constexpr (std::is_same_v<T, Trait_BLAKE2b>) {
                crypto_generichash_final(&blake_state, out_hash.data(), out_hash.size());
            }
            return out_hash;
        }
    };
};

#pragma GCC visibility pop
#endif

