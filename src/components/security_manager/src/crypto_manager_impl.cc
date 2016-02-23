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
#include <openssl/pkcs12.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include "security_manager/security_manager.h"
#include "config_profile/profile.h"

#include "utils/logger.h"
#include "utils/atomic.h"
#include "utils/file_system.h"
#include "utils/macro.h"
#include "utils/scope_guard.h"

#define TLS1_1_MINIMAL_VERSION            0x1000103fL
#define CONST_SSL_METHOD_MINIMAL_VERSION  0x00909000L

namespace security_manager {

CREATE_LOGGERPTR_GLOBAL(logger_, "SecurityManager")

uint32_t CryptoManagerImpl::instance_count_ = 0;
sync_primitives::Lock CryptoManagerImpl::instance_lock_;

// Handshake verification callback
// Used for debug outpute only
int debug_callback(int preverify_ok, X509_STORE_CTX* ctx);

namespace {
void free_ctx(SSL_CTX** ctx) {
  if (ctx) {
    SSL_CTX_free(*ctx);
    *ctx = NULL;
  }
}
}  // namespace
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
                             const std::string& cert_data,
                             const std::string& ciphers_list,
                             const bool verify_peer,
                             const std::string& ca_certificate_path,
                             const size_t hours_before_update) {
  LOG4CXX_AUTO_TRACE(logger_);
  mode_ = mode;
  verify_peer_ = verify_peer;
  certificate_data_ = cert_data;
  seconds_before_update_ = hours_before_update * 60 * 60;
  LOG4CXX_DEBUG(logger_, (mode_ == SERVER ? "Server" : "Client") << " mode");
  LOG4CXX_DEBUG(logger_, "Peer verification " << (verify_peer_ ? "enabled" : "disabled"));
  LOG4CXX_DEBUG(logger_, "Certificate data is \"" << certificate_data_ << '"');
  LOG4CXX_DEBUG(logger_, "CA certificate path is \"" << ca_certificate_path << '"');

  const bool is_server = (mode == SERVER);
#if OPENSSL_VERSION_NUMBER < CONST_SSL_METHOD_MINIMAL_VERSION
  SSL_METHOD* method;
#else
  const SSL_METHOD* method;
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
    LOG4CXX_ERROR(logger_,
                  "Could not create OpenSSLContext - " << LastError());
    return false;
  }
  utils::ScopeGuard guard = utils::MakeGuard(free_ctx, &context_);

  // Disable SSL2 as deprecated
  SSL_CTX_set_options(context_, SSL_OP_NO_SSLv2);

  set_certificate(certificate_data_);

  if (ciphers_list.empty()) {
    LOG4CXX_WARN(logger_, "Empty ciphers list");
  } else {
    LOG4CXX_DEBUG(logger_, "Cipher list: " << ciphers_list);
    if (!SSL_CTX_set_cipher_list(context_, ciphers_list.c_str())) {
      LOG4CXX_ERROR(logger_, "Could not set cipher list: " << ciphers_list);
      return false;
    }
  }

  if (ca_certificate_path.empty()) {
    LOG4CXX_WARN(logger_, "Setting up empty CA certificate location");
  }
  const std::string absolute_ca_path = file_system::GetAbsolutePath(ca_certificate_path);
  LOG4CXX_DEBUG(logger_, "Setting up CA certificate location: "
                << absolute_ca_path);
  const int result = SSL_CTX_load_verify_locations(context_,
                                                   NULL,
                                                   absolute_ca_path.c_str());
  if (!result) {
    LOG4CXX_WARN(
      logger_,
      "Wrong certificate path '" << ca_certificate_path << "' - " << LastError());
  }

  guard.Dismiss();

  const int verify_mode = verify_peer_ ? SSL_VERIFY_PEER |
                          SSL_VERIFY_FAIL_IF_NO_PEER_CERT
                          : SSL_VERIFY_NONE;
  LOG4CXX_DEBUG(logger_, "Setting up peer verification in mode: " << verify_mode);
  SSL_CTX_set_verify(context_, verify_mode, &debug_callback);

  return true;
}

bool CryptoManagerImpl::is_initialized() const {
  return context_;
}

bool CryptoManagerImpl::OnCertificateUpdated(const std::string& data) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (!is_initialized()) {
    LOG4CXX_WARN(logger_, "Not initialized");
    return false;
  }
  return set_certificate(data);
}

SSLContext* CryptoManagerImpl::CreateSSLContext() {
  if (context_ == NULL) {
    LOG4CXX_ERROR(logger_, "Not initialized");
    return NULL;
  }

  SSL* conn = SSL_new(context_);
  if (conn == NULL) {
    LOG4CXX_ERROR(logger_, "SSL context was not created - " << LastError());
    return NULL;
  }

  if (mode_ == SERVER) {
    SSL_set_accept_state(conn);
  } else {
    SSL_set_connect_state(conn);
  }
  return new SSLContextImpl(conn, mode_);
}

void CryptoManagerImpl::ReleaseSSLContext(SSLContext* context) {
  delete context;
}

std::string CryptoManagerImpl::LastError() const {
  const unsigned long openssl_error_id = ERR_get_error();
  std::stringstream string_stream;
  if (openssl_error_id == 0) {
    string_stream << "no openssl error occurs";
  } else {
    const char* error_string = ERR_reason_error_string(openssl_error_id);
    string_stream << "error: 0x" << std::hex << openssl_error_id
                  << ", \"" << std::string(error_string ? error_string : "") << '"';
  }
  if (!is_initialized()) {
    string_stream << ", initialization is not completed";
  }
  return string_stream.str();
}

bool CryptoManagerImpl::IsCertificateUpdateRequired() const {
  LOG4CXX_AUTO_TRACE(logger_);

  const time_t now = time(NULL);
  const time_t cert_date = mktime(&expiration_time_);

  const double seconds = difftime(cert_date, now);

  LOG4CXX_DEBUG(logger_, "Certificate time: " << asctime(&expiration_time_));
  LOG4CXX_DEBUG(logger_, "Host time: " << asctime(localtime(&now)));
  LOG4CXX_DEBUG(logger_, "Seconds before expiration: " << seconds);
  LOG4CXX_DEBUG(logger_, "Trigger update if less then: " << seconds_before_update_ <<
                " seconds left");

  return seconds <= seconds_before_update_;
}

int debug_callback(int preverify_ok, X509_STORE_CTX* ctx) {
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

bool CryptoManagerImpl::set_certificate(const std::string& cert_data) {
  LOG4CXX_AUTO_TRACE(logger_);

  if (cert_data.empty()) {
    LOG4CXX_WARN(logger_, "Empty certificate data");
    return false;
  }
  LOG4CXX_DEBUG(logger_, "Updating certificate and key from base64 data: \" "
                << cert_data);

  BIO* bio_cert = BIO_new_mem_buf(const_cast<char*>(cert_data.c_str()), cert_data.length());

  utils::ScopeGuard bio_guard = utils::MakeGuard(BIO_free, bio_cert);
  UNUSED(bio_guard)

  X509* cert = NULL;
  PEM_read_bio_X509(bio_cert, &cert,0, 0);

  EVP_PKEY* pkey = NULL;
  if (1 == BIO_reset(bio_cert)) {
    PEM_read_bio_PrivateKey(bio_cert, &pkey, 0,0);
  } else {
    LOG4CXX_WARN(logger_, "Unabled to reset BIO in order to read private key, " << LastError());
  }

  if (NULL == cert || NULL == pkey) {
    LOG4CXX_WARN(logger_, "Either certificate or key not valid, " << LastError());
    return false;
  }

  asn1_time_to_tm(X509_get_notAfter(cert));

  if (!SSL_CTX_use_certificate(context_, cert)) {
    LOG4CXX_WARN(logger_, "Could not use certificate, " << LastError());
    return false;
  }

  if (!SSL_CTX_use_PrivateKey(context_, pkey)) {
    LOG4CXX_ERROR(logger_, "Could not use key, " << LastError());
    return false;
  }
  if (!SSL_CTX_check_private_key(context_)) {
    LOG4CXX_ERROR(logger_, "Could not check private key, " << LastError());
    return false;
  }
  LOG4CXX_INFO(logger_, "Certificate and key data successfully updated");
  return true;
}

int CryptoManagerImpl::pull_number_from_buf(char* buf, int* idx) {
  if (!idx) {
    return 0;
  }
  const int val = ((buf[*idx] - '0') * 10) + buf[(*idx) + 1] - '0';
  *idx = *idx + 2;
  return val;
}

void CryptoManagerImpl::asn1_time_to_tm(ASN1_TIME* time) {
  char* buf = (char*)time->data;
  int index = 0;
  const int year = pull_number_from_buf(buf, &index);
  if (V_ASN1_GENERALIZEDTIME == time->type) {
    expiration_time_.tm_year = (year * 100 - 1900) + pull_number_from_buf(buf, &index);
  } else {
    expiration_time_.tm_year = year < 50 ? year + 100 : year;
  }

  const int mon = pull_number_from_buf(buf, &index);
  const int day = pull_number_from_buf(buf, &index);
  const int hour = pull_number_from_buf(buf, &index);
  const int mn = pull_number_from_buf(buf, &index);

  expiration_time_.tm_mon = mon - 1;
  expiration_time_.tm_mday = day;
  expiration_time_.tm_hour = hour;
  expiration_time_.tm_min = mn;

  if (buf[index] == 'Z') {
    expiration_time_.tm_sec = 0;
  }
  if ((buf[index] == '+') || (buf[index] == '-')) {
    const int mn = pull_number_from_buf(buf, &index);
    const int mn1 = pull_number_from_buf(buf, &index);
    expiration_time_.tm_sec = (mn * 3600) + (mn1 * 60);
  } else {
    const int sec =  pull_number_from_buf(buf, &index);
    expiration_time_.tm_sec = sec;
  }
}

}  // namespace security_manager
