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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_PROTOCOL_CONFIG_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_PROTOCOL_CONFIG_H_

#include <map>
#include <string>

namespace transport_manager {
namespace transport_adapter {

/**
 * @brief Class for reading iAP/iAP2 protocol names from system config files
 */
class ProtocolConfig {
 public:
  /**
   * @typedef Container for protocol names
   */
  typedef std::map<int, std::string> ProtocolNameContainer;

  /**
   * @brief Legacy protocol names for iAP
   * (read once and stored statically)
   */
  static const ProtocolNameContainer& IAPLegacyProtocolNames();
  /**
   * @brief Legacy protocol names for iAP2
   * (read once and stored statically)
   */
  static const ProtocolNameContainer& IAP2LegacyProtocolNames();
  /**
   * @brief Hub protocol names for iAP
   * (read once and stored statically)
   */
  static const ProtocolNameContainer& IAPHubProtocolNames();
  /**
   * @brief Hub protocol names for iAP2
   * (read once and stored statically)
   */
  static const ProtocolNameContainer& IAP2HubProtocolNames();
  /**
   * @brief Pool (i.e. non-legacy and non-hub) protocol names for iAP
   * (read once and stored statically)
   */
  static const ProtocolNameContainer& IAPPoolProtocolNames();
  /**
   * @brief Pool (i.e. non-legacy and non-hub) protocol names for iAP2
   * (read once and stored statically)
   */
  static const ProtocolNameContainer& IAP2PoolProtocolNames();

 private:
  static const std::string iap_section_name;
  static const std::string iap2_section_name;

  static const ProtocolNameContainer ReadIAPLegacyProtocolNames();
  static const ProtocolNameContainer ReadIAP2LegacyProtocolNames();
  static const ProtocolNameContainer ReadIAPHubProtocolNames();
  static const ProtocolNameContainer ReadIAP2HubProtocolNames();
  static const ProtocolNameContainer ReadIAPPoolProtocolNames();
  static const ProtocolNameContainer ReadIAP2PoolProtocolNames();

  static const ProtocolNameContainer ReadProtocolNames(
      const std::string& config_file_name, const std::string& section_name,
      const std::string& protocol_mask, ProtocolNameContainer& protocol_names);
};

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_PROTOCOL_CONFIG_H_
