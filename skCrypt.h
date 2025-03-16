#pragma once
#include <string>
#include <iostream>

#define BASE_SKCRYPT(str) []() -> skCryptStr { return skCryptStr(str); }()

class skCryptStr {
protected:
    char* m_data;
    size_t m_size;
    bool m_encrypted;
    mutable char m_key;

public:
    skCryptStr(const char* str) : m_size(strlen(str)), m_encrypted(true) {
        // Generate key based on string length
        m_key = static_cast<char>((m_size % 255) + 1);

        // Allocate and encrypt
        m_data = new char[m_size + 1];
        for (size_t i = 0; i < m_size; i++) {
            m_data[i] = str[i] ^ m_key;
        }
        m_data[m_size] = '\0';
    }

    // Rule of three (destructor, copy constructor, assignment operator)
    ~skCryptStr() {
        clear();
    }

    skCryptStr(const skCryptStr& other) : m_size(other.m_size), m_encrypted(other.m_encrypted), m_key(other.m_key) {
        m_data = new char[m_size + 1];
        memcpy(m_data, other.m_data, m_size + 1);
    }

    skCryptStr& operator=(const skCryptStr& other) {
        if (this != &other) {
            clear();
            m_size = other.m_size;
            m_encrypted = other.m_encrypted;
            m_key = other.m_key;
            m_data = new char[m_size + 1];
            memcpy(m_data, other.m_data, m_size + 1);
        }
        return *this;
    }

    // Decrypt method
    std::string decrypt() const {
        if (!m_data) return "";

        // Allocate temporary buffer
        char* decrypted = new char[m_size + 1];

        // Decrypt
        for (size_t i = 0; i < m_size; i++) {
            decrypted[i] = m_data[i] ^ m_key;
        }
        decrypted[m_size] = '\0';

        // Create string and clean up
        std::string result(decrypted);
        delete[] decrypted;

        return result;
    }

    // Get C-style string (decrypts if needed)
    const char* c_str() const {
        if (m_encrypted && m_data) {
            for (size_t i = 0; i < m_size; i++) {
                m_data[i] ^= m_key;
            }
            m_encrypted = false;
        }
        return m_data;
    }

    // Free memory
    void clear() {
        if (m_data) {
            // Secure clearing
            for (size_t i = 0; i < m_size; i++) {
                m_data[i] = 0;
            }
            delete[] m_data;
            m_data = nullptr;
        }
    }
};

// The actual skCrypt macro that will be used
#define skCrypt(str) BASE_SKCRYPT(str)