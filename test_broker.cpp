#include "pch.h"
#include "helpers.h"
#include "Order.h"
#include "AgisStrategy.h"

#include "Broker/Broker.Base.h"

namespace AgisBrokerTest {

    std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1_futures";
    std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";
    std::string holidy_file_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\example\\holidays\\us_trading_holidays.csv";


    std::string asset_id_1 = "CLU2000";
    std::string asset_id_2 = "CLQ2000";
    std::string asset_id_3 = "CLN2000";
    std::string exchange_id_1 = "exchange1";
    std::string broker_id_1 = "broker1";
    std::string strategy_id_1 = "strategy1";
    std::string portfolio_id_1 = "portfolio1";
    std::string exchange_id_complex = "SPY_DAILY";
    constexpr double cash = 100000;
    constexpr long long t0 = 960181200000000000;
    constexpr long long t1 = 960267600000000000;
    constexpr long long t2 = 960354000000000000;
    constexpr long long t3 = 960440400000000000;
    constexpr long long t4 = 960526800000000000;
    constexpr long long t5 = 960786000000000000;

    std::string tradeable_asset = R"(
[
    {
        "asset_id": "CLN2000",
        "unit_multiplier": 1,
        "overnight_initial_margin": 1,
        "intraday_initial_margin": 1,
        "intraday_maintenance_margin": 1,
        "overnight_initial_margin": 1,
        "overnight_maintenance_margin": 1,
        "short_overnight_initial_margin": 0.7,
        "short_overnight_maintenance_margin": 0.7
    },
    {
        "asset_id": "CLQ2000",
        "unit_multiplier": 100,
        "overnight_initial_margin": 0.5,
        "intraday_initial_margin": 0.5,
        "intraday_maintenance_margin": 0.5,
        "overnight_initial_margin": 0.5,
        "overnight_maintenance_margin": 0.5,
        "short_overnight_initial_margin": 0.75,
        "short_overnight_maintenance_margin": 0.75

    },
    {
        "asset_id": "CLU2000",
        "unit_multiplier": 1,
        "overnight_initial_margin": 1,
        "intraday_initial_margin": 1,
        "intraday_maintenance_margin": 1,
        "overnight_initial_margin": 1,
        "overnight_maintenance_margin": 1,
        "short_overnight_initial_margin": 1,
        "short_overnight_maintenance_margin": 1
    }
    ]
    )";
};


using namespace Agis;
using namespace AgisBrokerTest;


class DummyStrategy : public AgisStrategy {
public:
    DummyStrategy(
        PortfolioPtr const portfolio_,
        BrokerPtr broker_
    )
        : AgisStrategy(strategy_id_1, portfolio_, broker_, 1) {}

    void next() override {}
    void reset() override {}
    void build() override {
        this->exchange_subscribe(exchange_id_1);
    }

    ExchangePtr exchange;
};


class BrokerTests : public ::testing::Test
{
protected:
    std::shared_ptr<Hydra> hydra;

    void SetUp() override {
        hydra = std::make_shared<Hydra>(0);
        hydra->new_exchange(
            AssetType::US_FUTURE,
            exchange_id_1,
            exchange1_path,
            Frequency::Day1,
            "%Y-%m-%d",
            std::nullopt,
            std::nullopt,
            holidy_file_path
        );
        // create and register new broker
        hydra->new_broker(broker_id_1);
        auto broker = hydra->get_broker(broker_id_1).value();
        auto res = broker->load_tradeable_assets(tradeable_asset);
        if (!res.has_value()) {
			std::cout << res.error().what() << std::endl;
		}

        // create and register new portfolio
        auto portfolio = hydra->new_portfolio(portfolio_id_1, cash);

        // create and register new dummy strategy
        auto strategy = std::make_unique<DummyStrategy>(portfolio, broker);
        hydra->register_strategy(std::move(strategy));
    }
};

// Test case to verify the construction of a Broker object
TEST_F(BrokerTests, BrokerConstruct) {
    auto broker = hydra->get_broker(broker_id_1).value();
    EXPECT_EQ(broker->get_id(), broker_id_1);
    EXPECT_EQ(broker->get_index(), 0);
}


// Test case to verify the registration and retrieval of Brokers in a BrokerMap
TEST_F(BrokerTests, BrokerRegister) {
    auto broker_res = hydra->get_broker(broker_id_1);
    EXPECT_TRUE(broker_res.has_value());
    broker_res = hydra->get_broker("broker2");
    EXPECT_FALSE(broker_res.has_value());
}


// Test case to verify the margin loader functionality of a Broker
TEST_F(BrokerTests, BrokerTradeableAsset) {
    auto broker = hydra->get_broker(broker_id_1).value();
    auto asset1 = hydra->get_asset("CLU2000").unwrap();
    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset1_index = asset1->get_asset_index();
    auto asset2_index = asset2->get_asset_index();

    auto req = broker->get_margin_requirement(asset2_index, MarginType::OVERNIGHT_INITIAL);
    EXPECT_TRUE(req.has_value());
    EXPECT_EQ(req.value(), 0.5);
    req = broker->get_margin_requirement(asset1_index, MarginType::OVERNIGHT_INITIAL);
    EXPECT_TRUE(req.has_value());
    EXPECT_EQ(req.value(), 1.0);
}


TEST_F(BrokerTests, BrokerFuturesSimpleFill) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset3 = hydra->get_asset("CLN2000").unwrap();
    auto asset3_index = asset3->get_asset_index();
    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, 10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    
    // units * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .5;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(),cash_impact);

    portfolio->__on_order_fill(order);
    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);

    //============================================================================
    order = strategy->create_market_order(asset3_index, -100);
    order->fill(10, 0);
    order->__asset = asset3;
    brokers->__on_order_fill(order);

    // units * unit_multiplier * average_price * margin req
    auto cash_impact_2 = 100 * 1 * 10 * .7;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact_2);

    portfolio->__on_order_fill(order);
    strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact - cash_impact_2);

    // orders should not affect the NLV
    auto nlv = strategy->get_nlv();
    EXPECT_DOUBLE_EQ(nlv, cash);
}


TEST_F(BrokerTests, BrokerFuturesShortSimpleFill) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset3 = hydra->get_asset("CLN2000").unwrap();
    auto asset3_index = asset3->get_asset_index();
    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, -10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);

    // abs(units) * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .75;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    portfolio->__on_order_fill(order);
    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);

    hydra->__step();
    auto nlv = cash + (-10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

    order = strategy->create_market_order(asset2_index, 10);
    order->fill(99, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), strategy->get_cash());
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), portfolio->get_cash());
    auto trade = strategy->get_trade("CLQ2000");
    EXPECT_FALSE(trade.has_value());
}


TEST_F(BrokerTests, BrokerFuturesLongIncrease) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, 10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);

    // abs(units) * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .5;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);
    
    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);
    auto new_cash = cash - cash_impact;

    hydra->__step();
    double nlv_adjust = static_cast<double>(10 * 100) * (99 - 100);
    auto nlv = cash + nlv_adjust;
    double collateral = static_cast <double>(10 * 100 * 99) * .5;
    double cash_adjust = cash_impact - collateral + nlv_adjust;
    new_cash += cash_adjust;
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(strategy->get_cash(), new_cash);
    EXPECT_DOUBLE_EQ(portfolio->get_cash(), new_cash);

    //============================================================================
    order = strategy->create_market_order(asset2_index, 5);
    order->fill(99, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);
    cash_impact = 5 * 100 * 99 * .5;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);
    EXPECT_DOUBLE_EQ(order->get_margin_impact(), cash_impact);

    // order should not impact nlv at the time of fill
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
    new_cash -= cash_impact;
    EXPECT_DOUBLE_EQ(strategy->get_cash(), new_cash);
    EXPECT_DOUBLE_EQ(portfolio->get_cash(), new_cash);

    hydra->__step();
    auto collateral_new = static_cast <double>(15 * 100 * 99) * .5;
    collateral_new -= static_cast <double>(15 * 100 * 97) * .5;
    nlv_adjust = static_cast<double>(15 * 100) * (97 - 99);
    new_cash += (collateral_new + nlv_adjust);
    EXPECT_DOUBLE_EQ(strategy->get_cash(), new_cash);
    EXPECT_DOUBLE_EQ(portfolio->get_cash(), new_cash);

    double average_price = static_cast<double>(10 * 100 + 5 * 99) / 15;
    double trade_nlv = static_cast <double>(15 * 100 * 97) * .5;
    auto trade = strategy->get_trade("CLQ2000").value();
    EXPECT_DOUBLE_EQ(trade->nlv, trade_nlv);
    EXPECT_DOUBLE_EQ(trade->collateral, trade_nlv);
    EXPECT_DOUBLE_EQ(trade->margin, trade_nlv);
    EXPECT_DOUBLE_EQ(trade->average_price, average_price);
    
    nlv_adjust = static_cast<double>(15 * 100) * (97 - average_price);
    nlv = cash + nlv_adjust;
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

}


TEST_F(BrokerTests, BrokerFuturesLongReduce) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, 10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);

    // abs(units) * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .5;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);
    auto new_cash = cash - cash_impact;

    hydra->__step();
    auto nlv = cash + (10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

    //============================================================================
    order = strategy->create_market_order(asset2_index, -5);
    order->fill(99, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);
    cash_impact = -5 * 100 * 99 * .5;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    // order should not impact nlv at the time of fill
    nlv = cash + (10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
    new_cash = new_cash - cash_impact;

    hydra->__step();
    double trade_nlv = static_cast <double>(5 * 100 * 97) * .5;
    auto trade = strategy->get_trade("CLQ2000").value();
    EXPECT_DOUBLE_EQ(trade->margin, static_cast <double>(5 * 100 * 97) * .5);
    EXPECT_DOUBLE_EQ(trade->nlv, trade_nlv);

    double nlv_adjust = static_cast<double>(5 * 100) * (97 - 99);
    nlv += nlv_adjust;
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

}


TEST_F(BrokerTests, BrokerFuturesShortReduce) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, -10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);

    // abs(units) * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .75;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);
    auto new_cash = cash - cash_impact;

    hydra->__step();
    auto nlv = cash + (-10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

    //============================================================================
    order = strategy->create_market_order(asset2_index, 5);
    order->fill(99, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);
    cash_impact = -5 * 100 * 99 * .75;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    // order should not impact nlv at the time of fill
    nlv = cash + (-10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
    new_cash = new_cash - cash_impact;

    hydra->__step();
    double trade_nlv = static_cast <double>(5 * 100 * 97) * .75;
    auto trade = strategy->get_trade("CLQ2000").value();
    EXPECT_DOUBLE_EQ(trade->margin, static_cast <double>(5 * 100 * 97) * .25);
    EXPECT_DOUBLE_EQ(trade->nlv, trade_nlv);

    double nlv_adjust = static_cast<double>(-5 * 100) * (97 - 99);
    nlv += nlv_adjust;
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
}

TEST_F(BrokerTests, BrokerFuturesShortIncrease) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, -10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);

    // abs(units) * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .75;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);
    auto new_cash = cash - cash_impact;

    hydra->__step();
    auto nlv = cash + (-10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

    //============================================================================
    order = strategy->create_market_order(asset2_index, -5);
    order->fill(99, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);
    cash_impact = 5 * 100 * 99 * .75;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    // order should not impact nlv at the time of fill
    nlv = cash + (-10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
    new_cash = new_cash - cash_impact;

    hydra->__step();
    double trade_nlv = static_cast <double>(15 * 100 * 97) * .75;
    auto trade = strategy->get_trade("CLQ2000").value();
    EXPECT_DOUBLE_EQ(trade->margin, static_cast <double>(15 * 100 * 97) * .25);
    EXPECT_DOUBLE_EQ(trade->nlv, trade_nlv);

    double nlv_adjust = static_cast<double>(-15 * 100) * (97 - 99);
    nlv += nlv_adjust;
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
}

TEST_F(BrokerTests, BrokerFuturesShortFlip) {
    hydra->build();
    hydra->__step();
    auto brokers = hydra->__get_brokers();
    auto strategy = hydra->__get_strategy(strategy_id_1);
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto router = hydra->__get_router();

    auto asset2 = hydra->get_asset("CLQ2000").unwrap();
    auto asset2_index = asset2->get_asset_index();

    //============================================================================
    auto order = strategy->create_market_order(asset2_index, -10);
    order->fill(100, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);

    // abs(units) * unit_multiplier * average_price * margin req
    auto cash_impact = 10 * 100 * 100 * .75;
    EXPECT_DOUBLE_EQ(order->get_cash_impact(), cash_impact);

    auto strategy_cash = strategy->get_cash();
    EXPECT_DOUBLE_EQ(strategy_cash, cash - cash_impact);
    auto new_cash = cash - cash_impact;

    hydra->__step();
    auto nlv = cash + (-10 * 100) * (99 - 100);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

    //============================================================================
    order = strategy->create_market_order(asset2_index, 15);
    order->fill(99, 0);
    order->__asset = asset2;
    brokers->__validate_order(order);
    // a reverse order will cause the passed order to be set to null and the 
    // broker object automatically splits the order into two orders and sends to the router.
    EXPECT_FALSE(order);
    router->__process();

    auto trade_opt = strategy->get_trade("CLQ2000");
    EXPECT_TRUE(trade_opt.has_value());
    auto& trade = trade_opt.value();
    EXPECT_DOUBLE_EQ(trade->units, 5);
    hydra->__step();
    double trade_nlv = static_cast <double>(5 * 100 * 97) * .5;
    auto trade_new = strategy->get_trade("CLQ2000").value();
    EXPECT_DOUBLE_EQ(trade_new->margin, static_cast <double>(5 * 100 * 97) * .5);
    EXPECT_DOUBLE_EQ(trade_new->nlv, trade_nlv);

    double nlv_adjust = static_cast<double>(5 * 100) * (97 - 99);
    nlv += nlv_adjust;
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);

    order = strategy->create_market_order(asset2_index, -5);
    order->fill(97, 0);
    order->__asset = asset2;
    brokers->__on_order_fill(order);
    portfolio->__on_order_fill(order);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), nlv);
    EXPECT_DOUBLE_EQ(strategy->get_nlv(), strategy->get_cash());
    EXPECT_DOUBLE_EQ(portfolio->get_nlv(), portfolio->get_cash());
    trade_opt = strategy->get_trade("CLQ2000");
    EXPECT_FALSE(trade_opt.has_value());
}