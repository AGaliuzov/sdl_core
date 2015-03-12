/*
 * Copyright (c) 2015, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <gtest/gtest.h>
#include <fstream>

#include "security_manager/crypto_manager_impl.h"

#ifdef __QNXNTO__
#include <openssl/ssl3.h>
#else
#include <openssl/tls1.h>
#endif

#ifdef __QNXNTO__
#define FORD_CIPHER   SSL3_TXT_RSA_DES_192_CBC3_SHA
#else
// Used cipher from ford protocol requirement
#define FORD_CIPHER   TLS1_TXT_RSA_WITH_AES_256_GCM_SHA384
#endif

#define ALL_CIPHERS   "ALL"

namespace test {
namespace components {
namespace crypto_manager_test {

class CryptoManagerTest : public testing::Test {
 protected:
  void SetUp() OVERRIDE {
    crypto_manager = new security_manager::CryptoManagerImpl();
  }
  void TearDown() OVERRIDE {
    delete crypto_manager;
  }
  void InitSecurityManger() {
    const bool crypto_manager_initialization = crypto_manager->Init(
        security_manager::CLIENT, security_manager::TLSv1_2, "", "",
          ALL_CIPHERS, false, "/tmp/ca_cert.crt");
    ASSERT_TRUE(crypto_manager_initialization);
  }
  std::string GenerateCertificateString() {
    std::ifstream ca_certificate_file("root.crt");
    EXPECT_TRUE(ca_certificate_file.good());

    const std::string ca_cetrificate(
          (std::istreambuf_iterator<char>(ca_certificate_file)),
          std::istreambuf_iterator<char>());
    EXPECT_FALSE(ca_cetrificate.empty());
    return ca_cetrificate;
  }

  security_manager::CryptoManager* crypto_manager;
};

TEST_F(CryptoManagerTest, UsingBeforeInit) {
  EXPECT_TRUE(crypto_manager->CreateSSLContext() == NULL);
  EXPECT_EQ(std::string ("Initialization is not completed"),
            crypto_manager->LastError());
}

TEST_F(CryptoManagerTest, WrongInit) {
  security_manager::CryptoManager *crypto_manager = new security_manager::CryptoManagerImpl();

  //We have to cast (-1) to security_manager::Protocol Enum to be accepted by crypto_manager->Init(...)
  security_manager::Protocol UNKNOWN = static_cast<security_manager::Protocol>(-1);

  // Unknown protocol version
  EXPECT_FALSE(crypto_manager->Init(security_manager::SERVER, UNKNOWN,
                                    "mycert.pem", "mykey.pem", FORD_CIPHER,
                                    false, ""));

  EXPECT_FALSE(crypto_manager->LastError().empty());
  // Unexistent cert file
  EXPECT_FALSE(crypto_manager->Init(security_manager::SERVER, security_manager::TLSv1_2,
          "unexists_file.pem", "mykey.pem", FORD_CIPHER, false, ""));
  EXPECT_FALSE(crypto_manager->LastError().empty());
  // Unexistent key file
  EXPECT_FALSE(crypto_manager->Init(security_manager::SERVER, security_manager::TLSv1_2,
          "mycert.pem", "unexists_file.pem", FORD_CIPHER, false, ""));
  EXPECT_FALSE(crypto_manager->LastError().empty());
  // Unexistent cipher value
  EXPECT_FALSE(crypto_manager->Init(security_manager::SERVER, security_manager::TLSv1_2,
          "mycert.pem", "mykey.pem", "INVALID_UNKNOWN_CIPHER", false, ""));
  EXPECT_FALSE(crypto_manager->LastError().empty());

}

//#ifndef __QNXNTO__
TEST_F(CryptoManagerTest, CorrectInit) {
  // Empty cert and key values for SERVER
  EXPECT_TRUE(crypto_manager->Init(security_manager::SERVER, security_manager::TLSv1_2,
          "", "", FORD_CIPHER, false, ""));
  EXPECT_TRUE(crypto_manager->LastError().empty());
  // Recall init
  EXPECT_TRUE(crypto_manager->Init(security_manager::CLIENT, security_manager::TLSv1_2,
          "", "", FORD_CIPHER, false, ""));
  EXPECT_TRUE(crypto_manager->LastError().empty());
  // Recall init with other protocols
  EXPECT_TRUE(crypto_manager->Init(security_manager::CLIENT, security_manager::TLSv1_1,
          "", "", FORD_CIPHER, false, ""));
  EXPECT_TRUE(crypto_manager->LastError().empty());
  EXPECT_TRUE(crypto_manager->Init(security_manager::CLIENT, security_manager::TLSv1,
          "", "", FORD_CIPHER, false, ""));
  EXPECT_TRUE(crypto_manager->LastError().empty());

  // Cipher value
  EXPECT_TRUE(crypto_manager->Init(security_manager::SERVER, security_manager::TLSv1_2,
          "mycert.pem", "mykey.pem", ALL_CIPHERS, false, ""));
  EXPECT_TRUE(crypto_manager->LastError().empty());
}
//#endif  // __QNX__

TEST_F(CryptoManagerTest, ReleaseSSLContext_Null) {
  EXPECT_NO_THROW(crypto_manager->ReleaseSSLContext(NULL));
}

TEST_F(CryptoManagerTest, CreateReleaseSSLContext) {
  EXPECT_TRUE(crypto_manager->Init(
                security_manager::CLIENT, security_manager::TLSv1_2,
                "", "", ALL_CIPHERS, false, ""));
  security_manager::SSLContext *context = crypto_manager->CreateSSLContext();
  EXPECT_TRUE(context);
  EXPECT_NO_THROW(crypto_manager->ReleaseSSLContext(context));
}

TEST_F(CryptoManagerTest, OnCertificateUpdated) {
  InitSecurityManger();

  const std::string ca_cetrificate = GenerateCertificateString();
  ASSERT_FALSE(ca_cetrificate.empty());

  EXPECT_TRUE(crypto_manager->OnCertificateUpdated(ca_cetrificate));
}

TEST_F(CryptoManagerTest, OnCertificateUpdated_NotInitialized) {
  const std::string ca_cetrificate = GenerateCertificateString();
  ASSERT_FALSE(ca_cetrificate.empty());

  EXPECT_FALSE(crypto_manager->OnCertificateUpdated(ca_cetrificate));
}

TEST_F(CryptoManagerTest, OnCertificateUpdated_NullString) {
  InitSecurityManger();
  EXPECT_FALSE(crypto_manager->OnCertificateUpdated(std::string()));
}

TEST_F(CryptoManagerTest, OnCertificateUpdated_MalformedSign) {
  InitSecurityManger();

  std::string ca_cetrificate = GenerateCertificateString();
  ASSERT_FALSE(ca_cetrificate.empty());
  // corrupt the middle symbol
  ca_cetrificate[ca_cetrificate.size() / 2] =  '?';

  EXPECT_FALSE(crypto_manager->OnCertificateUpdated(ca_cetrificate));
}

TEST_F(CryptoManagerTest, OnCertificateUpdated_WrongInitFolder) {
  const bool crypto_manager_initialization = crypto_manager->Init(
        ::security_manager::CLIENT, security_manager::TLSv1_2, "", "",
        ALL_CIPHERS, true, "/wrong_folder");
  ASSERT_TRUE(crypto_manager_initialization);

  std::string ca_cetrificate = GenerateCertificateString();
  ASSERT_FALSE(ca_cetrificate.empty());

  EXPECT_FALSE(crypto_manager->OnCertificateUpdated(ca_cetrificate));
}

}  // namespace crypto_manager_test
}  // namespace components
}  // namespace test
