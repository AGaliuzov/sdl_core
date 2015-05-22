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

#include "security_manager/crypto_manager_impl.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fstream>
#include "security_manager/security_manager.h"
#include "utils/logger.h"
#include "utils/atomic.h"
#include "utils/file_system.h"
#include "utils/macro.h"
#include "utils/scope_guard.h"

#define TLS1_1_MINIMAL_VERSION            0x1000103fL
#define CONST_SSL_METHOD_MINIMAL_VERSION  0x00909000L

namespace security_manager {

CREATE_LOGGERPTR_GLOBAL(logger_, "CryptoManagerImpl")

uint32_t CryptoManagerImpl::instance_count_ = 0;
sync_primitives::Lock CryptoManagerImpl::instance_lock_;

// Handshake verification callback
// Used for debug outpute only
int debug_callback(int preverify_ok, X509_STORE_CTX *ctx);

namespace {
  void free_ctx(SSL_CTX** ctx) {
    if (ctx) {
      SSL_CTX_free(*ctx);
      *ctx = NULL;
    }
  }
}
CryptoManagerImpl::CryptoManagerImpl()
    : context_(NULL),
      mode_(CLIENT),
      verify_peer_(false) {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock lock(instance_lock_);
  instance_count_++;
  if (instance_count_ == 1) {
    LOG4CXX_DEBUG(logger_, "Openssl engine initialization");
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    SSL_library_init();
  }
}

CryptoManagerImpl::~CryptoManagerImpl() {
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock lock(instance_lock_);
  LOG4CXX_DEBUG(logger_, "Deinitilization");
  if (!context_) {
    LOG4CXX_WARN(logger_, "Manager is not initialized");
  } else {
    SSL_CTX_free(context_);
  }
  instance_count_--;
  if (instance_count_ == 0) {
    LOG4CXX_DEBUG(logger_, "Openssl engine deinitialization");
    EVP_cleanup();
    ERR_free_strings();
  }
}

bool CryptoManagerImpl::Init(Mode mode, Protocol protocol,
                             const std::string &cert_filename,
                             const std::string &key_filename,
                             const std::string &ciphers_list,
                             const bool verify_peer,
                             const std::string &ca_certificate_file) {
  LOG4CXX_AUTO_TRACE(logger_);
  mode_ = mode;
  verify_peer_ = verify_peer;
  ca_certificate_file_ = ca_certificate_file;
  LOG4CXX_DEBUG(logger_, (mode_ == SERVER ? "Server" : "Client") << " mode");
  LOG4CXX_DEBUG(logger_, "Peer verification " << (verify_peer_? "enabled" : "disabled"));
  LOG4CXX_DEBUG(logger_, "CA certificate file is \"" << ca_certificate_file_ << '"');

  const bool is_server = (mode == SERVER);
#if OPENSSL_VERSION_NUMBER < CONST_SSL_METHOD_MINIMAL_VERSION
  SSL_METHOD *method;
#else
  const SSL_METHOD *method;
#endif
  switch (protocol) {
    case SSLv3:
      method = is_server ? SSLv3_server_method() : SSLv3_client_method();
      break;
    case TLSv1:
      method = is_server ? TLSv1_server_method() : TLSv1_client_method();
      break;
    case TLSv1_1:
#if OPENSSL_VERSION_NUMBER < TLS1_1_MINIMAL_VERSION
      LOG4CXX_WARN(logger_,
          "OpenSSL has no TLSv1.1 with version lower 1.0.1, set TLSv1.0");
      method = is_server ?
      TLSv1_server_method() :
      TLSv1_client_method();
#else
      method = is_server ? TLSv1_1_server_method() : TLSv1_1_client_method();
#endif
      break;
    case TLSv1_2:
#if OPENSSL_VERSION_NUMBER < TLS1_1_MINIMAL_VERSION
      LOG4CXX_WARN(logger_,
          "OpenSSL has no TLSv1.2 with version lower 1.0.1, set TLSv1.0");
      method = is_server ?
      TLSv1_server_method() :
      TLSv1_client_method();
#else
      method = is_server ? TLSv1_2_server_method() : TLSv1_2_client_method();
#endif
      break;
    default:
      LOG4CXX_ERROR(logger_, "Unknown protocol: " << protocol);
      return false;
  }
  if (context_) {
    free_ctx(&context_);
  }

  context_ = SSL_CTX_new(method);
  if (!context_) {
    const char *error = ERR_reason_error_string(ERR_get_error());
    UNUSED(error);
    LOG4CXX_ERROR(logger_,
                  "Could not create OpenSSLContext " << (error ? error : ""));
  }
  utils::ScopeGuard guard = utils::MakeGuard(free_ctx, &context_);

  // Disable SSL2 as deprecated
  SSL_CTX_set_options(context_, SSL_OP_NO_SSLv2);

  if (cert_filename.empty()) {
    LOG4CXX_WARN(logger_, "Empty certificate path");
  } else {
    LOG4CXX_DEBUG(logger_, "Certificate path: " << cert_filename);
    if (!SSL_CTX_use_certificate_file(context_, cert_filename.c_str(),
    SSL_FILETYPE_PEM)) {
      LOG4CXX_ERROR(logger_, "Could not use certificate " << cert_filename);
    }
  }

  if (key_filename.empty()) {
    LOG4CXX_WARN(logger_, "Empty key path");
  } else {
    LOG4CXX_DEBUG(logger_, "Key path: " << key_filename);
    if (!SSL_CTX_use_PrivateKey_file(context_, key_filename.c_str(),
    SSL_FILETYPE_PEM)) {
      LOG4CXX_ERROR(logger_, "Could not use key " << key_filename);
    }
    if (!SSL_CTX_check_private_key(context_)) {
      LOG4CXX_ERROR(logger_, "Could not use certificate " << cert_filename);
    }
  }

  if (ciphers_list.empty()) {
    LOG4CXX_WARN(logger_, "Empty ciphers list");
  } else {
    LOG4CXX_DEBUG(logger_, "Cipher list: " << ciphers_list);
    if (!SSL_CTX_set_cipher_list(context_, ciphers_list.c_str())) {
      LOG4CXX_ERROR(logger_, "Could not set cipher list: " << ciphers_list);
    }
  }

  guard.Dismiss();
  SetVerification();
  return true;
}

bool CryptoManagerImpl::OnCertificateUpdated(const std::string &data) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!context_) {
    LOG4CXX_WARN(logger_, "Not initialized");
    return false;
  }
  if (data.empty()) {
    LOG4CXX_WARN(logger_, "Empty certificate data");
    return false;
  }
  LOG4CXX_DEBUG(logger_,
                "New certificate data : \"" << std::endl << data << '"');

  BIO* bio = BIO_new(BIO_s_mem());
  BIO_write(bio, data.c_str(), data.size());
  X509 *certificate = PEM_read_bio_X509(bio, NULL, NULL, NULL);
  BIO_free(bio);
  if (!certificate) {
    LOG4CXX_WARN(logger_, "New data is not a PEM X509 certificate");
    return false;
  }

  std::ofstream ca_certificate_file(ca_certificate_file_.c_str());
  if (!ca_certificate_file.is_open()) {
    LOG4CXX_ERROR(
        logger_,
        "Couldn't open certificate file \"" << ca_certificate_file << '"');
    return false;
  }

  ca_certificate_file << data << std::endl;
  if (!ca_certificate_file) {
    // Writing failed
    LOG4CXX_ERROR(
        logger_,
        "Couldn't write data to certificate file \"" << ca_certificate_file << '"');
    return false;
  }

  ca_certificate_file.close();
  LOG4CXX_DEBUG(logger_,
                "CA certificate saved as '" << ca_certificate_file_ << "' ");
  SetVerification();
  return true;
}

SSLContext* CryptoManagerImpl::CreateSSLContext() {
  if (context_ == NULL) {
    LOG4CXX_ERROR(logger_, "Not initialized");
    return NULL;
  }

  SSL *conn = SSL_new(context_);
  if (conn == NULL) {
    LOG4CXX_ERROR(logger_, "SSL context was not created");
    return NULL;
  }

  if (mode_ == SERVER) {
    SSL_set_accept_state(conn);
  } else {
    SSL_set_connect_state(conn);
  }
  return new SSLContextImpl(conn, mode_);
}

void CryptoManagerImpl::ReleaseSSLContext(SSLContext *context) {
  delete context;
}

std::string CryptoManagerImpl::LastError() const {
  if (!context_) {
    return std::string("Initialization is not completed");
  }
  const char *reason = ERR_reason_error_string(ERR_get_error());
  return std::string(reason ? reason : "");
}

void CryptoManagerImpl::SetVerification() {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!verify_peer_) {
    LOG4CXX_WARN(logger_,
                 "Peer verification disabling according to init options");
    SSL_CTX_set_verify(context_, SSL_VERIFY_NONE, &debug_callback);
    return;
  }
  LOG4CXX_DEBUG(logger_, "Setting up peer verification");
  const int verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  SSL_CTX_set_verify(context_, verify_mode, &debug_callback);

  if (ca_certificate_file_.empty()) {
    LOG4CXX_WARN(logger_, "Setting up empty CA certificate location");
    return;
  }
  LOG4CXX_DEBUG(logger_, "Setting up CA certificate location");
  const int result = SSL_CTX_load_verify_locations(context_,
                                                   NULL,
                                                   ca_certificate_file_.c_str());
  if (!result) {
    const unsigned long error = ERR_get_error();
    UNUSED(error);
    LOG4CXX_WARN(
        logger_,
        "Wrong certificate file '" << ca_certificate_file_
        << "', err 0x" << std::hex << error
        << " \"" << ERR_reason_error_string(error) << '"');
    return;
  }
}

int debug_callback(int preverify_ok, X509_STORE_CTX *ctx) {
  if (!preverify_ok) {
    const int error = X509_STORE_CTX_get_error(ctx);
    UNUSED(error);
    LOG4CXX_WARN(
        logger_,
        "Certificate verification failed with error " << error
        << " \"" << X509_verify_cert_error_string(error) << '"');
  }
  return preverify_ok;
}
}  // namespace security_manager
