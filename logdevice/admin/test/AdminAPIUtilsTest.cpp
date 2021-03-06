/**
 * Copyright (c) 2017-present, Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "logdevice/admin/AdminAPIUtils.h"

#include <folly/container/F14Map.h>
#include <gtest/gtest.h>

#include "logdevice/admin/Conv.h"

using namespace facebook::logdevice::configuration::nodes;

namespace facebook::logdevice {

namespace {

NodeLocation locationFromDomainString(const std::string&);

const std::string kTestAddress = "127.0.0.1";
const std::string kTestNodeName = "test-server";
const std::string kAnotherTestNodeName = "another-test-server";
const std::string kTestUnixPath = "/unix/socket/path";
const std::string kTestDomainString = "test.domain.string.five.scopes";
const node_index_t kTestNodeIndex = 1337;
const node_index_t kAnotherTestNodeIndex = 1007;
const in_port_t kTestDataPort = 4440;
const in_port_t kTestMediumPriorityPort = 4447;
const in_port_t kTestGossipPort = 4441;
const in_port_t kTestServerToServerPort = 4442;
const in_port_t kTestSslPort = 4443;
const in_port_t kTestClientThriftApiPort = 7440;
const in_port_t kTestServerThriftApiPort = 7441;
const in_port_t kTestAdminPort = 6440;
const Sockaddr kTestSocketAddress = Sockaddr{kTestAddress, kTestDataPort};
const Sockaddr kTestGossipSocketAddress =
    Sockaddr{kTestAddress, kTestGossipPort};
const Sockaddr kTestServerToServerSocketAddress =
    Sockaddr{kTestAddress, kTestServerToServerPort};
const Sockaddr kTestServerThriftApiSocketAddress =
    Sockaddr{kTestAddress, kTestServerThriftApiPort};
const Sockaddr kTestClientThriftApiSocketAddress =
    Sockaddr{kTestAddress, kTestClientThriftApiPort};
const folly::F14FastMap<NodeServiceDiscovery::ClientNetworkPriority, Sockaddr>
    kTestAddressesPerNetworkPriority{
        {NodeServiceDiscovery::ClientNetworkPriority::MEDIUM,
         Sockaddr{kTestAddress, kTestMediumPriorityPort}}};

const Sockaddr kTestSslSocketAddress = Sockaddr{kTestAddress, kTestSslPort};
const Sockaddr kTestAdminSocketAddress = Sockaddr{kTestAddress, kTestAdminPort};
const uint64_t kTestNodeVersion = 3147;
const NodeLocation kTestNodeLocation =
    locationFromDomainString(kTestDomainString);

thrift::SocketAddress toThrift(const Sockaddr& address) {
  facebook::logdevice::thrift::SocketAddress result;
  result.set_address(address.getAddress().str());
  result.set_port(address.port());
  return result;
}

NodeLocation locationFromDomainString(const std::string& domainString) {
  NodeLocation location;
  location.fromDomainString(domainString);
  return location;
}

} // namespace

TEST(AdminAPIUtilsTest, MatchNodeByName) {
  NodeServiceDiscovery nodeServiceDiscovery;
  nodeServiceDiscovery.name = kTestNodeName;

  thrift::NodeID thriftNodeId;

  thriftNodeId.set_name(kTestNodeName);
  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));

  thriftNodeId.set_name(kAnotherTestNodeName);
  EXPECT_FALSE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, MatchNodeByIndex) {
  NodeServiceDiscovery nodeServiceDiscovery;

  thrift::NodeID thriftNodeId;
  thriftNodeId.set_node_index(kTestNodeIndex);

  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));

  EXPECT_FALSE(
      nodeMatchesID(kAnotherTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, MatchNodeByAddressIpV4) {
  NodeServiceDiscovery nodeServiceDiscovery;
  nodeServiceDiscovery.default_client_data_address = kTestSocketAddress;

  thrift::NodeID thriftNodeId;

  thriftNodeId.set_address(toThrift(kTestSocketAddress));
  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));

  thriftNodeId.set_address(toThrift(kTestSslSocketAddress));
  EXPECT_FALSE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, MatchNodeByAddressIpV6WithCompression) {
  std::string compressedV6Address = "2001:4860:4860::8888";
  NodeServiceDiscovery nodeServiceDiscovery;
  nodeServiceDiscovery.default_client_data_address =
      Sockaddr{compressedV6Address, kTestDataPort};

  std::string uncompressedV6Address = "2001:4860:4860:0000:0000:0000:0000:8888";
  thrift::NodeID thriftNodeId;
  thriftNodeId.set_address(
      toThrift(Sockaddr{uncompressedV6Address, kTestDataPort}));

  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, MatchNodeByAddressUnixSocket) {
  NodeServiceDiscovery nodeServiceDiscovery;
  nodeServiceDiscovery.default_client_data_address = Sockaddr{kTestUnixPath};

  thrift::SocketAddress thriftSocketAddress;
  thriftSocketAddress.set_address(kTestUnixPath);
  thriftSocketAddress.set_address_family(thrift::SocketAddressFamily::UNIX);
  thrift::NodeID thriftNodeId;
  thriftNodeId.set_address(thriftSocketAddress);

  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, MatchByNameAndIndex) {
  NodeServiceDiscovery nodeServiceDiscovery;
  nodeServiceDiscovery.name = kTestNodeName;

  thrift::NodeID thriftNodeId;
  thriftNodeId.set_name(kTestNodeName);
  thriftNodeId.set_node_index(kTestNodeIndex);

  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, EmptyIDMatchesAnything) {
  NodeServiceDiscovery nodeServiceDiscovery;
  nodeServiceDiscovery.name = kTestNodeName;
  nodeServiceDiscovery.default_client_data_address =
      Sockaddr{kTestAddress, kTestDataPort};

  thrift::NodeID thriftNodeId;

  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));

  // Also matches any unix path
  nodeServiceDiscovery.default_client_data_address = Sockaddr{kTestUnixPath};
  EXPECT_TRUE(
      nodeMatchesID(kTestNodeIndex, nodeServiceDiscovery, thriftNodeId));
}

TEST(AdminAPIUtilsTest, FillNodeConfigPopulatesAllFields) {
  using ClientNetworkPriority =
      configuration::nodes::NodeServiceDiscovery::ClientNetworkPriority;

  // Build an input NodesConfiguration instance
  RoleSet roleSet;
  roleSet.set(static_cast<uint8_t>(NodeRole::STORAGE));
  roleSet.set(static_cast<uint8_t>(NodeRole::SEQUENCER));

  const NodeServiceDiscovery::TagMap tagMap = {
      {"test_key_1", "value_1"}, {"key_2", "value_2"}};

  NodeServiceDiscovery nodeServiceDiscovery{kTestNodeName,
                                            kTestNodeVersion,
                                            kTestSocketAddress,
                                            kTestGossipSocketAddress,
                                            kTestSslSocketAddress,
                                            kTestAdminSocketAddress,
                                            kTestServerToServerSocketAddress,
                                            kTestServerThriftApiSocketAddress,
                                            kTestClientThriftApiSocketAddress,
                                            kTestAddressesPerNetworkPriority,
                                            kTestNodeLocation,
                                            std::move(roleSet),
                                            tagMap};

  ServiceDiscoveryConfig::NodeUpdate nodeUpdate{
      ServiceDiscoveryConfig::UpdateType::PROVISION,
      std::make_unique<NodeServiceDiscovery>(std::move(nodeServiceDiscovery))};

  ServiceDiscoveryConfig::Update serviceDiscoveryUpdate;
  serviceDiscoveryUpdate.addNode(kTestNodeIndex, std::move(nodeUpdate));

  NodesConfiguration::Update nodesConfigUpdate{
      std::make_unique<ServiceDiscoveryConfig::Update>(
          std::move(serviceDiscoveryUpdate))};

  std::shared_ptr<const NodesConfiguration> nodesConfiguration =
      NodesConfiguration().applyUpdate(std::move(nodesConfigUpdate));

  // Build expected Thrift NodeConfig
  thrift::NodeConfig expected;
  expected.set_node_index(kTestNodeIndex);
  expected.set_name(kTestNodeName);
  expected.set_data_address(toThrift(kTestSocketAddress));

  thrift::Addresses otherAddresses;
  otherAddresses.set_gossip(toThrift(kTestGossipSocketAddress));
  otherAddresses.set_ssl(toThrift(kTestSslSocketAddress));
  otherAddresses.set_admin(toThrift(kTestAdminSocketAddress));
  otherAddresses.set_server_to_server(
      toThrift(kTestServerToServerSocketAddress));
  otherAddresses.set_server_thrift_api(
      toThrift(kTestServerThriftApiSocketAddress));
  otherAddresses.set_client_thrift_api(
      toThrift(kTestClientThriftApiSocketAddress));

  std::map<ClientNetworkPriority, thrift::SocketAddress> addresses_per_priority;
  for (auto& [priority, address] : kTestAddressesPerNetworkPriority) {
    addresses_per_priority[priority] = toThrift(address);
  }
  otherAddresses.set_addresses_per_priority(std::move(addresses_per_priority));

  expected.set_other_addresses(std::move(otherAddresses));

  expected.set_location(kTestDomainString);
  expected.set_location_per_scope(
      toThrift<thrift::Location>(folly::make_optional(kTestNodeLocation)));

  expected.roles_ref()->emplace(thrift::Role::STORAGE);
  expected.roles_ref()->emplace(thrift::Role::SEQUENCER);
  expected.tags_ref()->insert(tagMap.begin(), tagMap.end());

  // Test
  thrift::NodeConfig actual;
  fillNodeConfig(actual, kTestNodeIndex, *nodesConfiguration);

  EXPECT_EQ(expected, actual);
}

} // namespace facebook::logdevice
