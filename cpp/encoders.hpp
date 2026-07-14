//
//  encoders.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 26.06.2026.
//
#include <sodium.h>
#pragma once

struct ChaCha20Poly1305Coder {
	// Стандартный ключ ChaCha20 - 32 байта
    static constexpr std::size_t KEY_BYTES = crypto_aead_chacha20poly1305_ietf_KEYBYTES;
    // Poly1305 MAC tag всегда 16 байт
    static constexpr std::size_t TAG_BYTES = 16;
    // Стандартный IETF ChaCha20 nonce - 12 байт
    static constexpr std::size_t NONCE_BYTES = 12;

    // encrypt ожидает, что размер payload достаточен для записи данных + 16 байт тега в конец.
    // plaintext_len — это чистый размер твоих данных до шифрования.
    static void encrypt(std::span<uint8_t> payload,
        size_t plaintext_len,
        std::span<const uint8_t> key,
        std::span<const uint8_t> nonce) {

        unsigned long long ciphertext_len;

        // Используем встроенную в libsodium функцию in-place шифрования
        // Она зашифрует данные прямо в payload.data() и запишет 16 байт тега сразу за ними.
        crypto_aead_chacha20poly1305_ietf_encrypt(
            payload.data(), &ciphertext_len,             // куда писать результат
            payload.data(), plaintext_len,               // что шифровать (берем из того же буфера)
            nullptr, 0,                                  // связанные данные (AAD) — не используем
            nullptr,                                     // nsec (не используется)
            nonce.data(),                                // одноразовый код
            key.data()                                   // симметричный ключ
        );
    }

    static bool decrypt(std::span<uint8_t> payload,
        std::span<const uint8_t> key,
        std::span<const uint8_t> nonce) {

        unsigned long long decrypted_len;

        // Дешифруем прямо по месту. Если пакет подделан, функция вернет код ошибки != 0.
        int result = crypto_aead_chacha20poly1305_ietf_decrypt(
            payload.data(), &decrypted_len,              // куда писать расшифрованный текст
            nullptr,                                     // nsec (не используется)
            payload.data(), payload.size(),              // зашифрованные данные вместе с тегом
            nullptr, 0,                                  // связанные данные
            nonce.data(),
            key.data()
        );

        return (result == 0); // true если целостность подтверждена и данные расшифрованы
    }
};

struct XChaCha20Poly1305Coder {
    static constexpr std::size_t KEY_BYTES = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
    static constexpr std::size_t TAG_BYTES = 16;
    static constexpr std::size_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    // encrypt ожидает, что размер payload достаточен для записи данных + 16 байт тега в конец.
    // plaintext_len — это чистый размер твоих данных до шифрования.
    static void encrypt(std::span<uint8_t> payload,
        size_t plaintext_len,
        std::span<const uint8_t> key,
        std::span<const uint8_t> nonce) {

        unsigned long long ciphertext_len;

        // Используем встроенную в libsodium функцию in-place шифрования
        // Она зашифрует данные прямо в payload.data() и запишет 16 байт тега сразу за ними.
        crypto_aead_xchacha20poly1305_ietf_encrypt(
            payload.data(), &ciphertext_len,             // куда писать результат
            payload.data(), plaintext_len,               // что шифровать (берем из того же буфера)
            nullptr, 0,                                  // связанные данные (AAD) — не используем
            nullptr,                                     // nsec (не используется)
            nonce.data(),                                // одноразовый код
            key.data()                                   // симметричный ключ
        );
    }

    static bool decrypt(std::span<uint8_t> payload,
        std::span<const uint8_t> key,
        std::span<const uint8_t> nonce) {

        unsigned long long decrypted_len;

        // Дешифруем прямо по месту. Если пакет подделан, функция вернет код ошибки != 0.
        int result = crypto_aead_xchacha20poly1305_ietf_decrypt(
            payload.data(), &decrypted_len,              // куда писать расшифрованный текст
            nullptr,                                     // nsec (не используется)
            payload.data(), payload.size(),              // зашифрованные данные вместе с тегом
            nullptr, 0,                                  // связанные данные
            nonce.data(),
            key.data()
        );

        return (result == 0); // true если целостность подтверждена и данные расшифрованы
    }
};


class zero_encoder {
public:
    const size_t keylength = 0;
    const size_t noncelength = 0;
private:
    std::array<uint8_t, 0> key;

    bool set_key(const std::array<uint8_t, 0>& new_key) {
		return true; // Ничего не делаем, так как ключа нет
    }

    std::array<uint8_t, 0> get_key() const {
		return key; // Возвращаем пустой массив, так как ключа нет
    }

    bool generate_key() {
		return true; // Ничего не делаем, так как ключа нет
    }

    std::vector<uint8_t> encrypt_packet(const std::vector<uint8_t>& data,
        const std::array<uint8_t, 0>& key,
        const std::array<uint8_t, 0>& nonce) {

        return data;
    }

    std::vector<uint8_t> decrypt_packet(const std::vector<uint8_t>& ciphertext,
        const std::array<uint8_t, 0>& key,
        const std::array<uint8_t, 0>& nonce) {


        return ciphertext;
    }




    struct ZeroCoder {
        static constexpr std::size_t KEY_BYTES = 0;
        static constexpr std::size_t TAG_BYTES = 0;
        static constexpr std::size_t NONCE_BYTES = 0;

        // In-place "шифрование" — ничего не делаем
        static inline void encrypt(std::span<uint8_t> /*payload*/,
            std::span<const uint8_t> /*key*/,
            std::span<const uint8_t> /*nonce*/) noexcept {
            // Компилятор при -O3 полностью сотрет вызов
        }

        static inline bool decrypt(std::span<uint8_t> /*payload*/,
            std::span<const uint8_t> /*key*/,
            std::span<const uint8_t> /*nonce*/) noexcept {
            return true; // Всегда успешно
        }
    };

};