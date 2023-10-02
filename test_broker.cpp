#include "pch.h"
#include "helpers.h"
import Broker;

using namespace Agis;


class BrokerTests : public ::testing::Test
{};


// Test case to verify the construction of a Broker object
TEST(BrokerTests, BrokerConstruct) {
    // Create a shared pointer to a Broker object with the ID "broker1"
    auto broker = std::make_shared<Broker>("broker1");

    // Check if the ID of the created Broker matches "broker1"
    EXPECT_EQ(broker->get_id(), "broker1");

    // Check if the index of the created Broker is 0 (default value)
    EXPECT_EQ(broker->get_index(), 0);
}

// Test case to verify the registration and retrieval of Brokers in a BrokerMap
TEST(BrokerTests, BrokerRegister) {
    // Create an empty BrokerMap
    BrokerMap brokers;

    // Create a shared pointer to a Broker object with the ID "broker1"
    auto broker = std::make_shared<Broker>("broker1");

    // Register the Broker in the BrokerMap
    brokers.register_broker(broker);

    // Attempt to retrieve a Broker with the ID "broker1" from the BrokerMap
    auto broker_res = brokers.get_broker("broker1");

    // Verify that the retrieval was successful (the Broker exists)
    EXPECT_TRUE(broker_res.has_value());

    // Attempt to retrieve a Broker with the ID "broker2" from the BrokerMap
    broker_res = brokers.get_broker("broker2");

    // Verify that the retrieval was unsuccessful (the Broker does not exist)
    EXPECT_FALSE(broker_res.has_value());
}

// Test case to verify the functionality of adding tradeable assets to a Broker
TEST(BrokerTests, BrokerTradeableAsset) {
    // Create an empty BrokerMap
    BrokerMap brokers;

    // Create a shared pointer to a Broker object with the ID "broker1"
    auto broker = std::make_shared<Broker>("broker1");

    // Register the Broker in the BrokerMap
    brokers.register_broker(broker);

    // Attempt to retrieve a Broker with the ID "broker1" from the BrokerMap
    auto broker_res = brokers.get_broker("broker1");

    // Retrieve the Broker and add a tradeable asset with index 0
    broker_res.value()->add_tradeable_assets(0);

    // Create an Order object with asset index 0 and a price of 1.0
    auto order = AgisTest::create_order(0, 1.0);

    // Validate the order with the BrokerMap
    brokers.__validate_order(order);

    // Verify that the order state is PENDING after validation
    EXPECT_EQ(order->get_order_state(), OrderState::PENDING);

    // Test order rejection on an invalid asset index
    order = AgisTest::create_order(1, 1.0);

    // Validate the order with the BrokerMap
    brokers.__validate_order(order);

    // Verify that the order state is REJECTED after validation
    EXPECT_EQ(order->get_order_state(), OrderState::REJECTED);
}
