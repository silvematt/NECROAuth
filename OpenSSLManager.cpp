#include "OpenSSLManager.h"
SSL_CTX* OpenSSLManager::ctx = nullptr;
const std::array<unsigned char, 4> OpenSSLManager::cache_id = { 0x01, 0x02, 0x03, 0x04 };
