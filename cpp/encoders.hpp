//
//  encoders.hpp
//  MyProtocolCore
//
//  Created by Артем Батанин on 26.06.2026.
//
#include <sodium.h>
#pragma once

class ChaCha20_Poly1305_Encoder{
public:
    static constexpr size_t keylength = crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
    static constexpr size_t noncelength = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
private:
	std::array<uint8_t, keylength> key;
	bool is_key_set = false;

	bool set_key(const std::array<uint8_t, keylength>& new_key) {
		if (new_key.size() != keylength) {
			return false; // Неверный размер ключа
		}
        if (is_key_set) {
			return false; // Ключ уже установлен, нельзя изменить
        }
		std::copy(new_key.begin(), new_key.end(), key.begin());
		is_key_set = true;
		return true;
	}

	std::array<uint8_t, keylength> get_key() const {
        if (is_key_set) {
            return key;
        }
        else {
            throw std::runtime_error("Ключ не установлен!");
        }
	}

    bool generate_key() {
        if (is_key_set == 0) {
            randombytes_buf(key.data(), keylength);
            is_key_set = true;
            return true;
        }
        else {
            return 0;
        }
    }

    std::vector<uint8_t> encrypt_packet(const std::vector<uint8_t>& data,
                                        const std::array<uint8_t, noncelength>& nonce) {
        
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
        const std::array < uint8_t, noncelength > & nonce) {
        
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

};