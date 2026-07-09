//
//  asymetric_encoders.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 26.06.2026.
//
#include <array>
#include <cstdint>
#include <algorithm>
#include <oqs/oqs.h>
#include <sodium.h>
#pragma once




struct OQS_Level1 {
    static constexpr const char* alg_name = OQS_KEM_alg_ml_kem_512;
    static constexpr std::size_t pk_size = 800;
    static constexpr std::size_t ct_size = 768;
    static constexpr std::size_t secret_size = 32;
};

struct OQS_Level3 {
    static constexpr const char* alg_name = OQS_KEM_alg_ml_kem_768;
    static constexpr std::size_t pk_size = 1184;
    static constexpr std::size_t ct_size = 1088;
    static constexpr std::size_t secret_size = 32;
};

struct OQS_Level5 {
    static constexpr const char* alg_name = OQS_KEM_alg_ml_kem_1024;
    static constexpr std::size_t pk_size = 1568;
    static constexpr std::size_t ct_size = 1568;
    static constexpr std::size_t secret_size = 32;
};


struct Trait_X25519_SHA512 {
    static constexpr std::size_t pk_size = 32;
    static constexpr std::size_t sk_size = 32;
    static constexpr std::size_t ct_size = 32;
    static constexpr std::size_t secret_size = 64; // Полный хэш
};

template <typename T>
class OqsEncoder { 
public:
    static constexpr std::size_t PUBLIC_KEY_SIZE = T::pk_size;
    static constexpr std::size_t PRIVATE_KEY_SIZE = T::sk_size;
    static constexpr std::size_t CIPHERTEXT_SIZE = T::ct_size;
    static constexpr std::size_t SECRET_SIZE = T::secret_size;

    using PublicKeyBuf = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using PrivateKeyBuf = std::array<uint8_t, PRIVATE_KEY_SIZE>;
    using CiphertextBuf = std::array<uint8_t, CIPHERTEXT_SIZE>;
    using SecretBuf = std::array<uint8_t, SECRET_SIZE>;

    /**
     * ГЕНЕРАЦИЯ КЛЮЧЕЙ (Вызывается на стороне СЕРВЕРА)
     * Заполняет out_pub_key и out_priv_key случайными постквантовыми ключами.
     */
    bool generate_keypair(PublicKeyBuf& out_pub_key, PrivateKeyBuf& out_priv_key) {
        // Инициализируем выбранный в шаблоне алгоритм (например, ML-KEM-1024)
        OQS_KEM* kem = OQS_KEM_new(T::alg_name);
        if (!kem) return false;

        // Генерируем пару ключей. Функция сама заполнит массивы нужным числом байт
        int rc = OQS_KEM_keypair(kem, out_pub_key.data(), out_priv_key.data());

        // Обязательно освобождаем память структуры
        OQS_KEM_free(kem);

        return rc == OQS_SUCCESS;
    }


    bool encapsulate(const PublicKeyBuf& pub_key, CiphertextBuf& out_ct, SecretBuf& out_secret) {
        OQS_KEM* kem = OQS_KEM_new(T::alg_name);
        if (!kem) return false;
        int rc = OQS_KEM_encaps(kem, out_ct.data(), out_secret.data(), pub_key.data());
        OQS_KEM_free(kem);
        return rc == OQS_SUCCESS;
    }

    bool decapsulate(const PrivateKeyBuf& priv_key, const CiphertextBuf& ciphertext, SecretBuf& out_secret) {
        OQS_KEM* kem = OQS_KEM_new(T::alg_name);
        if (!kem) return false;
        int rc = OQS_KEM_decaps(kem, out_secret.data(), ciphertext.data(), priv_key.data());
        OQS_KEM_free(kem);
        return rc == OQS_SUCCESS;
    }
};

template <typename T>
class ClassicalEncoder { // sodium_memzero(server_sk.data(), server_sk.size()); !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
public:
    static constexpr std::size_t PUBLIC_KEY_SIZE = T::pk_size;
    static constexpr std::size_t PRIVATE_KEY_SIZE = T::sk_size;
    static constexpr std::size_t CIPHERTEXT_SIZE = T::ct_size;
    static constexpr std::size_t SECRET_SIZE = T::secret_size;

    using PublicKeyBuf = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using PrivateKeyBuf = std::array<uint8_t, PRIVATE_KEY_SIZE>;
    using CiphertextBuf = std::array<uint8_t, CIPHERTEXT_SIZE>;
    using SecretBuf = std::array<uint8_t, SECRET_SIZE>;

    bool encapsulate(const PublicKeyBuf& server_pub_key, CiphertextBuf& out_ct, SecretBuf& out_secret) {
        std::array<uint8_t, 32> ephem_priv, ephem_pub;
        if (crypto_kx_keypair(ephem_pub.data(), ephem_priv.data()) != 0) return false;
        std::copy(ephem_pub.begin(), ephem_pub.end(), out_ct.begin());

        std::array<uint8_t, 32> shared_point;
        if (crypto_scalarmult(shared_point.data(), ephem_priv.data(), server_pub_key.data()) != 0) return false;
        crypto_hash_sha512(out_secret.data(), shared_point.data(), shared_point.size());

        sodium_memzero(ephem_priv.data(), ephem_priv.size());
        return true;
    }

    bool decapsulate(const PrivateKeyBuf& server_priv_key, const CiphertextBuf& ciphertext, SecretBuf& out_secret) {
        std::array<uint8_t, 32> shared_point;
        if (crypto_scalarmult(shared_point.data(), server_priv_key.data(), ciphertext.data()) != 0) return false;
        crypto_hash_sha512(out_secret.data(), shared_point.data(), shared_point.size());
        return true;
    }
};

class ZeroCoder {
public:
    static constexpr std::size_t PUBLIC_KEY_SIZE = 0;
    static constexpr std::size_t PRIVATE_KEY_SIZE = 0;
    static constexpr std::size_t CIPHERTEXT_SIZE = 0;
    static constexpr std::size_t SECRET_SIZE = 32; 

    using PublicKeyBuf = std::array<uint8_t, PUBLIC_KEY_SIZE>;
    using PrivateKeyBuf = std::array<uint8_t, PRIVATE_KEY_SIZE>;
    using CiphertextBuf = std::array<uint8_t, CIPHERTEXT_SIZE>;
    using SecretBuf = std::array<uint8_t, SECRET_SIZE>;

    bool encapsulate(const PublicKeyBuf&, CiphertextBuf&, SecretBuf& out_secret) {
        out_secret.fill(0); 
        return true;
    }

    bool decapsulate(const PrivateKeyBuf&, const CiphertextBuf&, SecretBuf& out_secret) {
        out_secret.fill(0);
        return true;
    }
};
