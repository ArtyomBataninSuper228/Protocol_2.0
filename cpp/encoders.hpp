//
//  encoders.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 26.06.2026.
//
#include <sodium.h>


class ChaCha20_Poly1305_Encoder{
public:
    size_t keylength =32;
    size_t noncelength = 24;
    
    std::vector<uint8_t> encrypt_packet(const std::vector<uint8_t>& data,
                                        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES>& key,
                                        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES>& nonce) {
        
        // Размер выходного буфера: размер данных + 16 байт на тег Poly1305
        std::vector<uint8_t> ciphertext(data.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
        unsigned long long ciphertext_len;

        // Вызов из libsodium (используем XChaCha20 для длинного нонса в 24 байта)
        crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext.data(), &ciphertext_len,
            data.data(), data.size(),
            nullptr, 0, // Дополнительные нешифруемые данные (если нужны, например заголовок пакета)
            nullptr,
            nonce.data(),
            key.data()
        );

        return ciphertext;
    }
    
    std::vector<uint8_t> decrypt_packet(const std::vector<uint8_t>& ciphertext,
                                        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES>& key,
                                        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES>& nonce) {
        
        if (ciphertext.size() < crypto_aead_xchacha20poly1305_ietf_ABYTES) {
            throw std::runtime_error("Пакет слишком мал, тег Poly1305 отсутствует!");
        }

        std::vector<uint8_t> decrypted(ciphertext.size() - crypto_aead_xchacha20poly1305_ietf_ABYTES);
        unsigned long long decrypted_len;

        int result = crypto_aead_xchacha20poly1305_ietf_decrypt(
            decrypted.data(), &decrypted_len,
            nullptr,
            ciphertext.data(), ciphertext.size(),
            nullptr, 0,
            nonce.data(),
            key.data()
        );

        if (result != 0) {
            throw std::runtime_error("Критическая ошибка: Тег Poly1305 не валиден!");
        }
        return decrypted;
    }
};


class zero_encoder {
public:
    size_t keylength = 0;
    size_t noncelength = 0;

    std::vector<uint8_t> encrypt_packet(const std::vector<uint8_t>& data,
        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES>& key,
        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES>& nonce) {

        return data;
    }

    std::vector<uint8_t> decrypt_packet(const std::vector<uint8_t>& ciphertext,
        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_KEYBYTES>& key,
        const std::array<uint8_t, crypto_aead_xchacha20poly1305_ietf_NPUBBYTES>& nonce) {


        return ciphertext;
    }

};