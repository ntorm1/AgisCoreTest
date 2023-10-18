#include "pch.h"
#include "helpers.h"

#include "Time/TradingCalendar.h"
#include "Exchange.h"
#include "Broker/Broker.Base.h"

using namespace Agis;

namespace AgisFuturesComplexTest {
    std::string holidy_file_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\example\\holidays\\us_trading_holidays.csv";
	std::string exchange_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\FUTURES_DAILY\\data.h5";
	constexpr double cash = 100000;

    std::string asset_id_1 = "CLU2000";
    std::string asset_id_2 = "CLQ2000";
    std::string asset_id_3 = "CLN2000";
    std::string exchange_id_1 = "exchange1";
    std::string broker_id_1 = "broker1";
    std::string strategy_id_1 = "strategy1";
    std::string portfolio_id_1 = "portfolio1";

    std::string tradeable_asset = R"(
[
    {
        "contract_id": "CL",
		"exchange_id": "exchange1",
        "unit_multiplier": 1000,
        "overnight_initial_margin":  0.07,
        "intraday_initial_margin":  0.07,
        "intraday_maintenance_margin":  0.07,
        "overnight_initial_margin":  0.07,
        "overnight_maintenance_margin":  0.07,
        "short_overnight_initial_margin": 0.07,
        "short_overnight_maintenance_margin":  0.07
    },
    {
        "contract_id": "ES",
        "exchange_id": "exchange1",
        "unit_multiplier": 50,
        "overnight_initial_margin": 0.05,
        "intraday_initial_margin": 0.05,
        "intraday_maintenance_margin": 0.05,
        "overnight_initial_margin": 0.05,
        "overnight_maintenance_margin": 0.05,
        "short_overnight_initial_margin": 0.05,
        "short_overnight_maintenance_margin": 0.05
    }
    ]
    )";
}

using namespace AgisFuturesComplexTest;

class FuturesComplexTest : public ::testing::Test
{
protected:
    std::shared_ptr<Hydra> hydra;
    std::shared_ptr<TradingCalendar> calender;
    void SetUp() override {
        hydra = std::make_shared<Hydra>(0);
        hydra->new_exchange(
            AssetType::US_FUTURE,
            exchange_id_1,
            exchange_path,
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

TEST_F(FuturesComplexTest, TestComplexFutureTableCreate) {
    auto& exchanges = hydra->__get_exchanges();
    auto exchange = hydra->get_exchanges().get_exchange(exchange_id_1);
    auto res = hydra->build();
    CHECK_EXCEPT(res);
    EXPECT_TRUE(res.has_value());
    auto table_opt = exchange.value()->get_asset_table<FutureTable>("CL");
    EXPECT_TRUE(table_opt.has_value());
    auto& table = table_opt.value();
    auto front_month_opt = table->front_month();
    EXPECT_TRUE(front_month_opt.has_value());
    auto& front_month = front_month_opt.value();
    EXPECT_EQ(front_month->get_asset_id(), "CLX2018");

    exchanges.__goto(1587387600LL * 1000000000LL);
    front_month_opt = table->front_month();
    EXPECT_TRUE(front_month_opt.has_value());
    front_month = front_month_opt.value();
    EXPECT_EQ(front_month->get_asset_id(), "CLK2020");
}