#include "pch.h"
#include "helpers.h"
#include "Portfolio.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace AgisTestJson {
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

using namespace AgisTestJson;
using namespace rapidjson;


class JsonTest : public ::testing::Test
{
protected:
    std::shared_ptr<Hydra> hydra;
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
        auto exchange = hydra->get_exchanges().get_exchange(exchange_id_1).value();

        // create and register new broker
        hydra->new_broker(broker_id_1);

        // create and register new portfolio
        auto portfolio = hydra->new_portfolio(portfolio_id_1, cash);
    }
};

TEST_F(JsonTest, TestPortfolioJson) {
    auto& portfolios = hydra->get_portfolios();
    auto& portfolio = portfolios.get_portfolio(portfolio_id_1).get();

    auto j_portfolio = portfolio->to_json().value();
    EXPECT_TRUE(j_portfolio.HasMember("starting_cash"));
    // get the starting cash
    auto starting_cash = j_portfolio["starting_cash"].GetDouble();
    EXPECT_DOUBLE_EQ(starting_cash, cash);

    auto j_opt = portfolios.to_json();
    EXPECT_TRUE(j_opt.has_value());
    rapidjson::Document j = std::move(j_opt.value());
    // dumpy j to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    j.Accept(writer);
    auto j_Str = buffer.GetString();

    // expect j to have key portfolio_id_1
    EXPECT_TRUE(j.HasMember(portfolio_id_1.c_str()));
}


TEST_F(JsonTest, TestHydraJson) 
{
    auto hydra_state_opt = hydra->save_state();
    EXPECT_TRUE(hydra_state_opt.has_value());
    auto& hydra_state = hydra_state_opt.value();

    // get portfolios json from hydra state
    auto& portfolios = hydra_state["portfolios"];
    EXPECT_TRUE(portfolios.IsObject());
    EXPECT_TRUE(portfolios.HasMember(portfolio_id_1.c_str()));
}