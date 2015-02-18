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

namespace test {
namespace components {
namespace ssl_handshake_test {

namespace {
typedef std::vector<std::string> strings;
const std::string root_certificate_   = "root.crt";
const std::string ford_certificate_   = "ford.crt";
const std::string ford2_certificate_   = "ford2.crt";
const std::string server_storage_folder_ = "server";
const std::string client_storage_folder_ = "client";
const std::string client_certificate_ = "client_sign.crt";
const std::string server_certificate_ = "server_sign.crt";
const std::string client_key_ = "client.key";
const std::string server_key_ = "server.key";
static const std::string certificates[]  = {root_certificate_, ford2_certificate_};
static const std::string certificates2[] = {root_certificate_, ford_certificate_};
// Server chain needs for client credential verification
const strings server_certificate_chain =
    strings(certificates, certificates + sizeof(certificates) / sizeof(certificates[0]));
// Client chain needs for server credential verification
const strings client_certificate_chain =
    strings(certificates2, certificates2 + sizeof(certificates2) / sizeof(certificates2[0]));
}  // namespace

class SSLHandshakeTest : public testing::Test {
 protected:
  void SetUp() OVERRIDE {
    server_manager = new security_manager::CryptoManagerImpl();
    ASSERT_TRUE(server_manager);
    client_manager = new security_manager::CryptoManagerImpl();
    ASSERT_TRUE(client_manager);
    server_ctx = NULL;
    client_ctx = NULL;
  }

  void TearDown() OVERRIDE {
    server_manager->ReleaseSSLContext(server_ctx);
    delete server_manager;
    client_manager->ReleaseSSLContext(client_ctx);
    delete client_manager;
  }

  bool InitServerManagers(security_manager::Protocol protocol,
                          const std::string &cert_filename,
                          const std::string &key_filename,
                          const std::string &ciphers_list,
                          const bool verify_peer,
                          const strings& certificate = strings()) {
    const bool initialized = server_manager->Init(
          security_manager::SERVER, protocol, cert_filename,
          key_filename, ciphers_list, verify_peer, server_storage_folder_);
    if(!initialized) {
      return false;
    }
    if(!certificate.empty()) {
      const std::string cert = LoadCertificate(certificate);
      if(!server_manager->OnCertificateUpdated(cert)) {
        return false;
      }
    }
    server_ctx = server_manager->CreateSSLContext();
    if(!server_ctx) {
      return false;
    }
    return true;
  }
  bool InitClientManagers(security_manager::Protocol protocol,
                          const std::string &cert_filename,
                          const std::string &key_filename,
                          const std::string &ciphers_list,
                          const bool verify_peer,
                          const strings& certificate = strings()) {
    const bool initialized = client_manager->Init(
          security_manager::CLIENT, protocol, cert_filename,
          key_filename, ciphers_list, verify_peer, client_storage_folder_);
    if(!initialized) {
      return false;
    }
    if(!certificate.empty()) {
      const std::string cert = LoadCertificate(certificate);
      if(!client_manager->OnCertificateUpdated(cert)) {
        return false;
      }
    }
    client_ctx = client_manager->CreateSSLContext();
    if(!client_ctx) {
      return false;
    }
    return true;
  }

  std::string LoadCertificate(const strings& ca_files) {
    std::stringstream stream;

    for (size_t i = 0; i < ca_files.size(); ++i) {
      std::ifstream file(ca_files[i]);
      EXPECT_TRUE(file.good());

      std::copy(std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>(),
                std::ostreambuf_iterator<char>(stream));
    }
    const std::string ca_cetrificate = stream.str();
    EXPECT_FALSE(ca_cetrificate.empty());
    return ca_cetrificate;
  }

  security_manager::CryptoManager* server_manager;
  security_manager::CryptoManager* client_manager;
  security_manager::SSLContext *server_ctx;
  security_manager::SSLContext *client_ctx;

  const uint8_t *server_buf;
  const uint8_t *client_buf;
  size_t server_buf_len;
  size_t client_buf_len;
};


TEST_F(SSLHandshakeTest, Handshake_NoVerification) {
  using security_manager::SSLContext;
  ASSERT_TRUE(InitServerManagers(security_manager::TLSv1_2, server_certificate_,
                                 server_key_, "ALL", false))
      << server_manager->LastError();
  ASSERT_TRUE(InitClientManagers(security_manager::TLSv1_2, client_certificate_,
                                 client_key_, "ALL", false))
      << client_manager->LastError();

  ASSERT_EQ(client_ctx->StartHandshake(&client_buf, &client_buf_len),
            SSLContext::Handshake_Result_Success);
  ASSERT_FALSE(client_buf == NULL);
  ASSERT_GT(client_buf_len, 0u);

  while (true) {
    ASSERT_EQ(server_ctx->DoHandshakeStep(client_buf,
                                          client_buf_len,
                                          &server_buf,
                                          &server_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    ASSERT_FALSE(server_buf == NULL);
    ASSERT_GT(server_buf_len, 0u);

    ASSERT_EQ(client_ctx->DoHandshakeStep(server_buf,
                                          server_buf_len,
                                          &client_buf,
                                          &client_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    if (server_ctx->IsInitCompleted()) {
      break;
    }

    ASSERT_FALSE(client_buf == NULL);
    ASSERT_GT(client_buf_len, 0u);
  }
}

TEST_F(SSLHandshakeTest, CAVerification_ClientSide) {
  using security_manager::SSLContext;
  ASSERT_TRUE(InitServerManagers(security_manager::TLSv1_2, server_certificate_,
                                 server_key_, "ALL", false))
      << server_manager->LastError();
  ASSERT_TRUE(InitClientManagers(security_manager::TLSv1_2, client_certificate_,
                                 client_key_, "ALL", true, client_certificate_chain))
      << client_manager->LastError();

  const uint8_t *server_buf;
  const uint8_t *client_buf;
  size_t server_buf_len;
  size_t client_buf_len;

  ASSERT_EQ(client_ctx->StartHandshake(&client_buf, &client_buf_len),
            SSLContext::Handshake_Result_Success);
  ASSERT_FALSE(client_buf == NULL);
  ASSERT_GT(client_buf_len, 0u);

  while (true) {
    ASSERT_EQ(server_ctx->DoHandshakeStep(client_buf,
                                          client_buf_len,
                                          &server_buf,
                                          &server_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    ASSERT_FALSE(server_buf == NULL);
    ASSERT_GT(server_buf_len, 0u);

    ASSERT_EQ(client_ctx->DoHandshakeStep(server_buf,
                                          server_buf_len,
                                          &client_buf,
                                          &client_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    if (server_ctx->IsInitCompleted()) {
      break;
    }

    ASSERT_FALSE(client_buf == NULL);
    ASSERT_GT(client_buf_len, 0u);
  }
}

TEST_F(SSLHandshakeTest, CAVerification_ServerSide) {
  using security_manager::SSLContext;
  ASSERT_TRUE(InitServerManagers(security_manager::TLSv1_2, server_certificate_,
                                 server_key_, "ALL", true, server_certificate_chain))
      << server_manager->LastError();
  ASSERT_TRUE(InitClientManagers(security_manager::TLSv1_2, client_certificate_,
                                 client_key_, "ALL", false))
      << client_manager->LastError();

  const uint8_t *server_buf;
  const uint8_t *client_buf;
  size_t server_buf_len;
  size_t client_buf_len;

  ASSERT_EQ(client_ctx->StartHandshake(&client_buf, &client_buf_len),
            SSLContext::Handshake_Result_Success);
  ASSERT_FALSE(client_buf == NULL);
  ASSERT_GT(client_buf_len, 0u);

  while (true) {
    ASSERT_EQ(server_ctx->DoHandshakeStep(client_buf,
                                          client_buf_len,
                                          &server_buf,
                                          &server_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    ASSERT_FALSE(server_buf == NULL);
    ASSERT_GT(server_buf_len, 0u);

    ASSERT_EQ(client_ctx->DoHandshakeStep(server_buf,
                                          server_buf_len,
                                          &client_buf,
                                          &client_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    if (server_ctx->IsInitCompleted()) {
      break;
    }

    ASSERT_FALSE(client_buf == NULL);
    ASSERT_GT(client_buf_len, 0u);
  }
}

TEST_F(SSLHandshakeTest, CAVerification_BothSides) {
  using security_manager::SSLContext;
  ASSERT_TRUE(InitServerManagers(security_manager::TLSv1_2, server_certificate_,
                                 server_key_, "ALL", true, server_certificate_chain))
      << server_manager->LastError();
  ASSERT_TRUE(InitClientManagers(security_manager::TLSv1_2, client_certificate_,
                                 client_key_, "ALL", true, client_certificate_chain))
      << client_manager->LastError();

  const uint8_t *server_buf;
  const uint8_t *client_buf;
  size_t server_buf_len;
  size_t client_buf_len;

  ASSERT_EQ(client_ctx->StartHandshake(&client_buf, &client_buf_len),
            SSLContext::Handshake_Result_Success);
  ASSERT_FALSE(client_buf == NULL);
  ASSERT_GT(client_buf_len, 0u);

  while (true) {
    ASSERT_EQ(server_ctx->DoHandshakeStep(client_buf,
                                          client_buf_len,
                                          &server_buf,
                                          &server_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    ASSERT_FALSE(server_buf == NULL);
    ASSERT_GT(server_buf_len, 0u);

    ASSERT_EQ(client_ctx->DoHandshakeStep(server_buf,
                                          server_buf_len,
                                          &client_buf,
                                          &client_buf_len),
              SSLContext::Handshake_Result_Success)
        << ERR_reason_error_string(ERR_get_error());
    if (server_ctx->IsInitCompleted()) {
      break;
    }

    ASSERT_FALSE(client_buf == NULL);
    ASSERT_GT(client_buf_len, 0u);
  }
}

}  // namespace ssl_handshake_test
}  // namespace components
}  // namespace test
