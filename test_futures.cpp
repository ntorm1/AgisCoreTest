#include "pch.h"
#include "helpers.h"

#include "Time/TradingCalendar.h"

#include "Broker/Broker.Base.h"

using namespace Agis;

namespace AgisFuturesTest {
    std::string holidy_file_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\example\\holidays\\us_trading_holidays.csv";
    std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1_futures";
    std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

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

using namespace AgisFuturesTest;

class FuturesTest : public ::testing::Test
{
protected:
    std::shared_ptr<Hydra> hydra;
    std::shared_ptr<TradingCalendar> calender;
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
        calender = hydra->get_exchanges().get_exchange(exchange_id_1).value()->get_trading_calendar();
        // create and register new broker
        hydra->new_broker(broker_id_1);
        auto broker = hydra->get_broker(broker_id_1).value();
        auto res = broker->load_tradeable_assets(tradeable_asset);
        if (!res.has_value()) {
            std::cout << res.error().what() << std::endl;
        }

        // create and register new portfolio
        auto portfolio = hydra->new_portfolio(portfolio_id_1, cash);
    }
};


TEST_F(FuturesTest, TestHolidays) {
    auto holidays = calender->holidays();
    EXPECT_EQ(holidays.size(), 503);
    auto first = holidays[0];
    EXPECT_EQ(first.year(), 1980);
    EXPECT_EQ(first.month(), 1);
    EXPECT_EQ(first.day(), 1);
    auto last = holidays[holidays.size() - 1];
    EXPECT_EQ(last.year(), 2030);
    EXPECT_EQ(last.month(), 12);
    EXPECT_EQ(last.day(), 25);

}


TEST_F(FuturesTest, TestESExpiration) {
    std::string contract = "ESM2019";
    long long expected = 1561123800LL * NS_CONST;
    auto res = calender->es_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

    contract = "ESZ2022";
    expected = 1671201000LL * NS_CONST;
    res = calender->es_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

    contract = "ESU2021";
    expected = 1631885400LL * NS_CONST;
    res = calender->es_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);
}


TEST_F(FuturesTest, TestZFExpiration) {
    std::string contract = "ZFM2021";
    long long expected = 1625068860LL * NS_CONST;
    auto res = calender->zf_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

    contract = "ZFU2019";
    expected = 1569859260LL * NS_CONST;
    res = calender->zf_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

    contract = "ZFZ2022";
    expected = 1672419660 * NS_CONST;
    res = calender->zf_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

}

TEST_F(FuturesTest, TestZFFirstIntention) {
    std::string contract = "ZFM2022";
    long long expected = 1653688800LL * NS_CONST;
    auto res = calender->zf_futures_contract_to_first_intention(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

}

TEST_F(FuturesTest, TestCLExpiration) {
    std::string contract = "CLF2019";
    long long expected = 1545260400LL * NS_CONST;
    auto res = calender->cl_future_contract_to_expiry(contract);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res.value(), expected);

}

TEST_F(FuturesTest, TestFutureTableCreate) {
    auto exchange = hydra->get_exchanges().get_exchange(exchange_id_1);
    hydra->build();
    auto table_opt = exchange.value()->get_asset_table<FutureTable>("CL");
    EXPECT_TRUE(table_opt.has_value());
    auto& table = table_opt.value();
    auto front_month_opt = table->front_month();
    EXPECT_TRUE(front_month_opt.has_value());
    auto& front_month = front_month_opt.value();
    EXPECT_EQ(front_month->get_asset_id(), "CLN2000");
}