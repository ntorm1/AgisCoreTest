#include "pch.h"
#include "helpers.h"

#include "Time/TradingCalendar.h"
#include "Exchange.h"
#include "AgisStrategy.h"
#include "Broker/Broker.Base.h"

constexpr double VOL_TARGET = 0.1f;

using namespace Agis;

namespace AgisTestAst {
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

using namespace AgisTestAst;

class FuturesASTTest : public ::testing::Test
{
protected:
    std::shared_ptr<Hydra> hydra;
    std::shared_ptr<TradingCalendar> calender;
    BrokerPtr broker;
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
        calender = exchange->get_trading_calendar();

        // create and register new broker
        hydra->new_broker(broker_id_1);

        // create and register new portfolio
        auto portfolio = hydra->new_portfolio(portfolio_id_1, cash);
    }
};

class AgisASTStrategy : public AgisStrategy {
public:
    AgisASTStrategy(
        std::string id,
        PortfolioPtr const portfolio_,
        BrokerPtr broker_,
        double portfolio_allocation_
    ) : AgisStrategy(id, portfolio_, broker_, portfolio_allocation_) {}

    void load_node(std::unique_ptr<AbstractStrategyAllocationNode> allocation_node) {
        this->allocation_node = std::move(allocation_node);
    }
    void next() override{
        auto res = this->allocation_node->execute();
        if (!res.has_value()) {
            throw std::runtime_error("FAILED ON AST NEXT");
        }
    }

    void build() override{
        this->exchange_subscribe(exchange_id_1);
        this->warmup = this->allocation_node->get_warmup();
    }

    void reset() override{}
private:
    std::unique_ptr<AbstractStrategyAllocationNode> allocation_node = nullptr;
};

TEST_F(FuturesASTTest, TestASTCreate) {
    auto exchange = hydra->get_exchange(exchange_id_1).value();
    auto portfolio = hydra->get_portfolio(portfolio_id_1);
    auto strategy = std::make_unique<AgisASTStrategy>(
		strategy_id_1,
        portfolio,
        hydra->get_broker(broker_id_1).value(),
		1.0
	);
    hydra->register_strategy(std::move(strategy));
    auto strategy_ptr = hydra->__get_strategy(strategy_id_1);
    strategy_ptr->set_vol_trace(true);

    //============================================================================
    auto exchange_node = create_exchange_node(exchange);
    auto cl_future_table_node = create_future_table_node(
        exchange_node,
        "CL",
        TableExtractMethod::FRONT
    );

    //============================================================================
    auto lambda_init = create_asset_lambda_read("close", 0);
    auto lambda_prev = create_asset_lambda_read("close", -1);
    auto lambda_opp = create_asset_lambda_opp_r(
        std::move(lambda_init),
        std::move(lambda_prev),
        AgisOpperationType::SUBTRACT
    );
    auto lambda_div = create_asset_lambda_read("close", -1);
    auto lambda_final = create_asset_lambda_opp_r(
        std::move(lambda_opp),
        std::move(lambda_div),
        AgisOpperationType::DIVIDE
    );

    //============================================================================
    auto table_view = create_future_view_node(
        cl_future_table_node,
        lambda_final
    );
    auto alloc_node = create_table_gen_alloc_node(
        table_view,
        ExchangeViewOpp::CONDITIONAL_SPLIT,
        1,
        0.0f
    );
    alloc_node->set_ev_scaler_type(ExchangeViewScaler::VOLATILITY);
    alloc_node->set_vol_target(VOL_TARGET);
    auto strat_alloc_node = create_strategy_alloc_node(
        strategy_ptr,
        alloc_node,
        0.0f,
        true,
        std::nullopt,
        AllocType::PCT
    );
    auto ast_strategy = static_cast<AgisASTStrategy*>(strategy_ptr);
    ast_strategy->load_node(std::move(strat_alloc_node));

    hydra->build();
    broker = hydra->get_broker(broker_id_1).value();
    auto res = broker->load_tradeable_assets(tradeable_asset);
    if (!res.has_value()) {
        std::cout << res.error().what() << std::endl;
    }
    exchange->__set_volatility_lookback(252);
    hydra->build();
    
    hydra->__step();
    hydra->__step();

    auto vol_opt = strategy_ptr->get_portfolio_volatility();
    EXPECT_TRUE(vol_opt.has_value());
    EXPECT_DOUBLE_EQ(vol_opt.value(), VOL_TARGET);
}