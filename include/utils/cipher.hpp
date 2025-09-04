/**
 * @file cipher.hpp
 * @brief Cryptographic utilities for Minecraft protocol authentication and security
 * 
 * This file provides essential cryptographic functions for implementing the Minecraft
 * Java Edition server protocol, including MD5 hashing, UUID generation for offline
 * mode players, and various authentication utilities required for the login process.
 * 
 * @author [@logpacket](https://github.com/logpacket)
 * @version 1.0.0
 * @date 2025/07/08
 * 
 * @copyright Copyright (c) 2025 ParellelStone Project
 * 
 * @details
 * Key Features:
 * - MD5 hash computation for offline UUID generation
 * - Minecraft offline mode UUID generation following official specifications
 * - Cryptographically secure random token generation for authentication
 * - Player name validation according to Minecraft naming rules
 * - Cross-platform compatibility (Windows x86_64, macOS arm64, Linux x86_64/arm64)
 */

#pragma once

#include <array>
#include <random>
#include <string>
#include <vector>

namespace minecraft::utils {

/**
 * @brief Computes MD5 hash of the input string
 * @param input The input string to hash
 * @return std::array<uint8_t, 16> The 16-byte MD5 hash result
 * 
 * This function implements the MD5 cryptographic hash algorithm.
 * It processes the input string and returns a 128-bit (16-byte) hash value.
 */
inline std::array<uint8_t, 16> computeMD5(const std::string &input) {
  // MD5 constants
  constexpr uint32_t S[64] = {
      7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
      5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

  constexpr uint32_t K[64] = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
      0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
      0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
      0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
      0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
      0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
      0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
      0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
      0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

  // Initialize MD5 hash values
  uint32_t h[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

  // Prepare message
  std::vector<uint8_t> message(input.begin(), input.end());
  uint64_t originalLength = message.size();

  // Append '1' bit
  message.push_back(0x80);

  // Append '0' bits until message length ≡ 448 (mod 512)
  while ((message.size() % 64) != 56) {
    message.push_back(0x00);
  }

  // Append original length as 64-bit little-endian
  for (int i = 0; i < 8; ++i) {
    message.push_back(static_cast<uint8_t>((originalLength * 8) >> (i * 8)));
  }

  // Process message in 512-bit chunks
  for (size_t chunk = 0; chunk < message.size(); chunk += 64) {
    uint32_t w[16];

    // Break chunk into sixteen 32-bit little-endian words
    for (int i = 0; i < 16; ++i) {
      w[i] = static_cast<uint32_t>(message[chunk + i * 4]) |
             (static_cast<uint32_t>(message[chunk + i * 4 + 1]) << 8) |
             (static_cast<uint32_t>(message[chunk + i * 4 + 2]) << 16) |
             (static_cast<uint32_t>(message[chunk + i * 4 + 3]) << 24);
    }

    uint32_t a = h[0], b = h[1], c = h[2], d = h[3];

    // Main loop
    for (int i = 0; i < 64; ++i) {
      uint32_t f, g;

      if (i < 16) {
        f = (b & c) | (~b & d);
        g = i;
      } else if (i < 32) {
        f = (d & b) | (~d & c);
        g = (5 * i + 1) % 16;
      } else if (i < 48) {
        f = b ^ c ^ d;
        g = (3 * i + 5) % 16;
      } else {
        f = c ^ (b | ~d);
        g = (7 * i) % 16;
      }

      f = f + a + K[i] + w[g];
      a = d;
      d = c;
      c = b;
      b = b + ((f << S[i]) | (f >> (32 - S[i])));
    }

    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
  }

  // Convert result to byte array (little-endian)
  std::array<uint8_t, 16> result;
  for (int i = 0; i < 4; ++i) {
    result[i * 4] = static_cast<uint8_t>(h[i] & 0xFF);
    result[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 8) & 0xFF);
    result[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 16) & 0xFF);
    result[i * 4 + 3] = static_cast<uint8_t>((h[i] >> 24) & 0xFF);
  }

  return result;
}

/**
 * @brief Generates an offline mode UUID for a player
 * @param playerName The player's name
 * @return UUID The generated UUID for offline mode
 * 
 * This function generates a UUID for offline mode by creating an MD5 hash
 * of "OfflinePlayer:" concatenated with the player name. The result is
 * formatted as a version 3 UUID according to Minecraft's offline UUID standard.
 */
inline UUID generateOfflineUuid(const std::string &playerName) {
  // Minecraft offline UUID generation: MD5 hash of "OfflinePlayer:" +
  // playerName
  std::string input = "OfflinePlayer:" + playerName;

  // Calculate MD5 hash (16 bytes)
  std::array<uint8_t, 16> md5Hash = computeMD5(input);

  // Format as UUID version 3 (name-based using MD5)
  // Set version bits (4 bits) to 3 (0011)
  md5Hash[6] = (md5Hash[6] & 0x0F) | 0x30;

  // Set variant bits (2 bits) to 10
  md5Hash[8] = (md5Hash[8] & 0x3F) | 0x80;

  // Convert to UUID format (most significant 8 bytes, least significant 8
  // bytes)
  uint64_t msb = 0;
  uint64_t lsb = 0;

  for (int i = 0; i < 8; ++i) {
    msb = (msb << 8) | md5Hash[i];
    lsb = (lsb << 8) | md5Hash[i + 8];
  }

  return UUID(msb, lsb);
}

/**
 * @brief Generates a random verify token for authentication
 * @return std::vector<uint8_t> A 4-byte random verify token
 * 
 * This function generates a 4-byte random token used in the Minecraft
 * authentication process. Each byte is randomly generated using a
 * cryptographically secure random number generator.
 */
inline std::vector<uint8_t> generateVerifyToken() {
  std::vector<uint8_t> token(4);
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 255);
  for (auto &byte : token) {
    byte = static_cast<uint8_t>(dist(rd));
  }
  return token;
}

/**
 * @brief Generates a random AES shared secret
 * @return std::vector<uint8_t> A 16-byte random shared secret for AES-128
 * 
 * This function generates a 16-byte (128-bit) random shared secret used
 * for AES encryption in the Minecraft protocol. The secret is generated
 * using a cryptographically secure random number generator.
 */
inline std::vector<uint8_t> generateSharedSecret() {
  std::vector<uint8_t> secret(16);  // AES-128
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, 255);
  for (auto &byte : secret) {
    byte = static_cast<uint8_t>(dist(rd));
  }
  return secret;
}
}  // namespace minecraft::utils
