/*
 * Copyright (c) 2014, Ford Motor Company
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

#include <assert.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory.h>
#include <map>
#include <algorithm>

#include "utils/macro.h"

namespace security_manager {

CREATE_LOGGERPTR_GLOBAL(logger_, "CryptoManagerImpl")

CryptoManagerImpl::SSLContextImpl::SSLContextImpl(SSL *conn, Mode mode)
  : connection_(conn),
    bioIn_(BIO_new(BIO_s_mem())),
    bioOut_(BIO_new(BIO_s_mem())),
    bioFilter_(NULL),
    // TODO(EZamakhov): get MTU by parameter (from transport)
    // default buffer size is TCP MTU
    buffer_size_(1500),
    buffer_(new uint8_t[buffer_size_]),
    is_handshake_pending_(false),
    mode_(mode) {
  SSL_set_bio(connection_, bioIn_, bioOut_);
}

std::string CryptoManagerImpl::SSLContextImpl::LastError() const {
  if (last_error_.empty()) {
    const char *reason = ERR_reason_error_string(ERR_get_error());
    if (reason) {
      last_error_ = std::string(reason ? reason : "");
    } else {
      if (!IsInitCompleted()) {
        last_error_ = "Initialization is not completed";
      }
    }
  }
  return last_error_;
}

bool CryptoManagerImpl::SSLContextImpl::IsInitCompleted() const {
  sync_primitives::AutoLock locker(bio_locker);
  return SSL_is_init_finished(connection_);
}

SSLContext::HandshakeResult CryptoManagerImpl::SSLContextImpl::
StartHandshake(const uint8_t** const out_data, size_t *out_data_size) {
  is_handshake_pending_ = true;
  return DoHandshakeStep(NULL, 0, out_data, out_data_size);
}

namespace {
  size_t aes128_gcm_sha256_max_block_size(size_t mtu) {
    if (mtu < 29)
      return 0;
    return mtu - 29;
  }
  size_t rc4_md5_max_block_size(size_t mtu) {
    if (mtu < 21)
      return 0;
    return mtu - 21;
  }
  size_t rc4_sha_max_block_size(size_t mtu) {
    if (mtu < 25)
      return 0;
    return mtu - 25;
  }
  size_t seed_sha_max_block_size(size_t mtu) {
    if (mtu < 53)
      return 0;
    return ((mtu - 37) & 0xfffffff0) - 5;
  }
  size_t aes128_sha256_max_block_size(size_t mtu) {
    if (mtu < 69)
      return 0;
    return ((mtu - 53) & 0xfffffff0) - 1;
  }
  size_t des_cbc3_sha_max_block_size(size_t mtu) {
    if (mtu < 37)
      return 0;
    return ((mtu - 29) & 0xfffffff8) - 5;
  }
}  // namespace

std::map<std::string, CryptoManagerImpl::SSLContextImpl::BlockSizeGetter>
CryptoManagerImpl::SSLContextImpl::create_max_block_sizes() {
  std::map<std::string, CryptoManagerImpl::SSLContextImpl::BlockSizeGetter> rc;
  rc.insert(std::make_pair("AES128-GCM-SHA256", aes128_gcm_sha256_max_block_size));
  rc.insert(std::make_pair("AES128-SHA256",     aes128_sha256_max_block_size));
  rc.insert(std::make_pair("AES128-SHA",        seed_sha_max_block_size));
  rc.insert(std::make_pair("AES256-GCM-SHA384", aes128_gcm_sha256_max_block_size));
  rc.insert(std::make_pair("AES256-SHA256",     aes128_sha256_max_block_size));
  rc.insert(std::make_pair("AES256-SHA",        seed_sha_max_block_size));
  rc.insert(std::make_pair("CAMELLIA128-SHA",   seed_sha_max_block_size));
  rc.insert(std::make_pair("CAMELLIA256-SHA",   seed_sha_max_block_size));
  rc.insert(std::make_pair("DES-CBC3-SHA",      des_cbc3_sha_max_block_size));
  rc.insert(std::make_pair("DES-CBC-SHA",       des_cbc3_sha_max_block_size));
  rc.insert(std::make_pair("RC4-MD5",           rc4_md5_max_block_size));
  rc.insert(std::make_pair("RC4-SHA",           rc4_sha_max_block_size));
  rc.insert(std::make_pair("SEED-SHA",          seed_sha_max_block_size));
  return rc;
}

std::map<std::string, CryptoManagerImpl::SSLContextImpl::BlockSizeGetter>
CryptoManagerImpl::SSLContextImpl::max_block_sizes =
    CryptoManagerImpl::SSLContextImpl::create_max_block_sizes();

SSLContext::HandshakeResult CryptoManagerImpl::SSLContextImpl::
DoHandshakeStep(const uint8_t*  const in_data,  size_t in_data_size,
                const uint8_t** const out_data, size_t* out_data_size) {
  LOG4CXX_AUTO_TRACE(logger_);
  DCHECK(out_data);
  DCHECK(out_data_size);
  *out_data = NULL;
  *out_data_size = 0;
  // TODO(Ezamakhov): add test - hanshake fail -> restart StartHandshake
  sync_primitives::AutoLock locker(bio_locker);
  if (SSL_is_init_finished(connection_)) {
    LOG4CXX_DEBUG(logger_, "SSL initilization is finished");
    is_handshake_pending_ = false;
    return SSLContext::Handshake_Result_Success;
  }

  if (in_data && in_data_size) {
    LOG4CXX_DEBUG(logger_, "Handling " << in_data_size << " bytes");
    const int ret = BIO_write(bioIn_, in_data, in_data_size);
    if (ret <= 0) {
      LOG4CXX_WARN(logger_, "BIO write fail");
      is_handshake_pending_ = false;
      ResetConnection();
      return SSLContext::Handshake_Result_AbnormalFail;
    }
  }

  STACK_OF(X509) *peer_certs = SSL_get_peer_cert_chain(connection_);
  while (sk_X509_num(peer_certs) > 0) {
    X509* cert = sk_X509_pop(peer_certs);
    char *subj = X509_NAME_oneline(X509_get_subject_name(cert), NULL, 0);
    if (subj) {
      std::replace(subj, subj + strlen(subj), '/', ' ');
      LOG4CXX_DEBUG(logger_, "Mobile cert subject:" << subj);
      OPENSSL_free(subj);
    }
    char *issuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);
    if (issuer) {
      std::replace(issuer, issuer + strlen(issuer), '/', ' ');
      LOG4CXX_DEBUG(logger_, "Mobile cert issuer:" << issuer);
      OPENSSL_free(issuer);
    }
  }

  const int handshake_result = SSL_do_handshake(connection_);
  LOG4CXX_TRACE(logger_, "Do handshake result is " << handshake_result);
  if (handshake_result == 1) {
    LOG4CXX_DEBUG(logger_, "SSL handshake successfully finished");
    // Handshake is successful
    bioFilter_ = BIO_new(BIO_f_ssl());
    BIO_set_ssl(bioFilter_, connection_, BIO_NOCLOSE);

    const SSL_CIPHER *cipher = SSL_get_current_cipher(connection_);
    max_block_size_ = max_block_sizes[SSL_CIPHER_get_name(cipher)];
    is_handshake_pending_ = false;
  } else if (handshake_result == 0) {
    const int error = SSL_get_error(connection_, handshake_result);
    SetHandshakeError(error);
    LOG4CXX_WARN(logger_, "Handshake failed on the mobile side with error " << error
                 << " \"" << LastError() << '"');
    ResetConnection();
    is_handshake_pending_ = false;
    return SSLContext::Handshake_Result_Fail;
  } else {
    const int error = SSL_get_error(connection_, handshake_result);
    if (error != SSL_ERROR_WANT_READ) {
      const long error = SSL_get_verify_result(connection_);
      SetHandshakeError(error);
      LOG4CXX_WARN(logger_, "Handshake failed with error " << error
                   << " \"" << LastError() << '"');
      ResetConnection();
      is_handshake_pending_ = false;
      return SSLContext::Handshake_Result_AbnormalFail;
    }
  }

  const size_t pend = BIO_ctrl_pending(bioOut_);
  LOG4CXX_DEBUG(logger_, "Available " << pend << " bytes for handshake");

  if (pend > 0) {
    LOG4CXX_DEBUG(logger_, "Reading handshake data");
    EnsureBufferSizeEnough(pend);

    const int read_count = BIO_read(bioOut_, buffer_, pend);
    if (read_count  == static_cast<int>(pend)) {
      *out_data_size = read_count;
      *out_data =  buffer_;
    } else {
      LOG4CXX_WARN(logger_, "BIO read fail");
      is_handshake_pending_ = false;
      ResetConnection();
      return SSLContext::Handshake_Result_AbnormalFail;
    }
  }

  return SSLContext::Handshake_Result_Success;
}

bool CryptoManagerImpl::SSLContextImpl::Encrypt(
    const uint8_t *  const in_data,  size_t in_data_size,
    const uint8_t ** const out_data, size_t *out_data_size) {

  sync_primitives::AutoLock locker(bio_locker);
  if (!SSL_is_init_finished(connection_) ||
      !in_data ||
      !in_data_size) {
    return false;
  }

  BIO_write(bioFilter_, in_data, in_data_size);
  const size_t len = BIO_ctrl_pending(bioOut_);

  EnsureBufferSizeEnough(len);
  const int read_size = BIO_read(bioOut_, buffer_, len);
  DCHECK(len == static_cast<size_t>(read_size));
  if (read_size <= 0) {
    // Reset filter and connection deinitilization instead
    BIO_ctrl(bioFilter_, BIO_CTRL_RESET, 0, NULL);
    return false;
  }
  *out_data_size = read_size;
  *out_data = buffer_;

  return true;
}

bool CryptoManagerImpl::SSLContextImpl::Decrypt(
    const uint8_t *  const in_data,  size_t in_data_size,
    const uint8_t ** const out_data, size_t *out_data_size) {

  sync_primitives::AutoLock locker(bio_locker);
  if (!SSL_is_init_finished(connection_)) {
    return false;
  }

  if (!in_data || !in_data_size) {
    return false;
  }
  BIO_write(bioIn_, in_data, in_data_size);
  int len = BIO_ctrl_pending(bioFilter_);
  ptrdiff_t offset = 0;

  *out_data_size = 0;
  while (len) {
    EnsureBufferSizeEnough(len + offset);
    len = BIO_read(bioFilter_, buffer_ + offset, len);
    // TODO(EZamakhov): investigate BIO_read return 0, -1 and -2 meanings
    if (len <= 0) {
      // Reset filter and connection deinitilization instead
      BIO_ctrl(bioFilter_, BIO_CTRL_RESET, 0, NULL);
      return false;
    }
    *out_data_size += len;
    offset += len;
    len = BIO_ctrl_pending(bioFilter_);
  }
  *out_data = buffer_;
  return true;
}

size_t CryptoManagerImpl::SSLContextImpl::get_max_block_size(size_t mtu) const {
  if (!max_block_size_) {
    // FIXME(EZamakhov): add correct logics for TLS1/1.2/SSL3
    // For SSL3.0 set temporary value 90, old TLS1.2 value is 29
    assert(mtu > 90);
    return mtu - 90;
  }
  return max_block_size_(mtu);
}

bool CryptoManagerImpl::SSLContextImpl::IsHandshakePending() const {
  return is_handshake_pending_;
}

CryptoManagerImpl::SSLContextImpl::~SSLContextImpl() {
  SSL_shutdown(connection_);
  SSL_free(connection_);
  delete[] buffer_;
}

void CryptoManagerImpl::SSLContextImpl::SetHandshakeError(const int error) {
  const char* error_str = X509_verify_cert_error_string(error);
  if (error_str) {
    last_error_ = error_str;
  } else {
    // Error will be updated with the next LastError call
    last_error_.clear();
  }
}

void CryptoManagerImpl::SSLContextImpl::ResetConnection() {
  const int clear_result = SSL_clear(connection_);
  if (!clear_result) {
    const char *reason = ERR_reason_error_string(ERR_get_error());
    UNUSED(reason);
    LOG4CXX_WARN(logger_, "Connection reset failed with \""
                 << (reason ? reason : "") << '"');
  }
}

void CryptoManagerImpl::SSLContextImpl::EnsureBufferSizeEnough(size_t size) {
  if (buffer_size_ < size) {
    delete[] buffer_;
    buffer_ = new(std::nothrow) uint8_t[size];
    if (buffer_) {
      buffer_size_ = size;
    }
  }
}

}  // namespace security_manager
