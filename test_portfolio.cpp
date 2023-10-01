#include "pch.h"

#include "Hydra.h"
#include "Utils.h"
#include "AgisStrategy.h"
#include <ankerl/unordered_dense.h>

namespace portfolio_test {
	std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

	std::string exchange_id_complex = "SPY_DAILY";
	std::string portfolio_id_complex = "dummy";
	std::string strategy_id_complex = "dummy";

	double cash = 1000000.0;
}

using namespace portfolio_test;

class DummyStrategy : public AgisStrategy {
public:
	DummyStrategy(PortfolioPtr const portfolio_)
		: AgisStrategy(strategy_id_complex, portfolio_, 1) {}


	OrderPtr forward_market_order(
		size_t asset_index,
		double units,
		std::optional<TradeExitPtr> exit = std::nullopt)
	{
		auto order = this->create_market_order(asset_index, units, exit);
		return std::move(order);
	}

	void next() override {}
	void reset() override {}
	void build() override {
		this->exchange_subscribe(exchange_id_complex);
		this->exchange = this->get_exchange();
	}

	ExchangePtr exchange;
};


class BetaHedgeStrategy : public AgisStrategy {
public:
	BetaHedgeStrategy(PortfolioPtr const portfolio_)
		: AgisStrategy(strategy_id_complex, portfolio_, 1) {}

	void next() override {
		auto ev = this->exchange->get_exchange_view("Close");
		ev.uniform_weights(1.5);
		this->strategy_allocate(
			ev,
			0.0,
			true,
			std::nullopt,
			AllocType::PCT
		);
	}
	void reset() override {}
	void build() override {
		this->set_target_leverage(1.5);
		this->set_beta_hedge_positions(true);
		this->set_beta_trace(true);

		this->exchange_subscribe(exchange_id_complex);
		this->exchange = this->get_exchange();
	}

	ExchangePtr exchange;
	
};

class SimplePortfolioFixture : public ::testing::Test {
protected:
	std::unique_ptr<Hydra> hydra;
	std::vector<std::string> asset_ids = { "SPY", "MSFT", "ORCL" };

	void SetUp() override {
		this->hydra = std::make_unique<Hydra>(0);
		this->hydra->new_exchange(
			exchange_id_complex,
			exchange_complex_path,
			Frequency::Day1,
			"%Y-%m-%d",
			asset_ids
		);
		auto& portfolios = this->hydra->__get_portfolios();
		hydra->new_portfolio(
			portfolio_id_complex,
			cash
		);
	}
};




TEST_F(SimplePortfolioFixture, TestOrderValidate) {
	auto portfolio = this->hydra->get_portfolio(portfolio_id_complex);
	auto strategy = std::make_unique<DummyStrategy>(
		portfolio
	);
	this->hydra->register_strategy(std::move(strategy));
	hydra->build();
	auto& exchanges = this->hydra->get_exchanges();

	std::string msft_string = "MSFT"; std::string orcl_string = "ORCL";
	auto msft_id = exchanges.get_asset_index(msft_string);
	auto orcl_id = exchanges.get_asset_index(orcl_string);
	auto msft_price = exchanges.__get_market_price(msft_string, true);
	auto orcle_price = exchanges.__get_market_price(orcl_string, true);

	auto strat_ptr = hydra->__get_strategy(strategy_id_complex);
	auto strat = dynamic_cast<DummyStrategy*>(strat_ptr);

	strat->set_max_leverage(1.25);
	auto order = strat->forward_market_order(msft_id, cash / msft_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::PENDING);
	EXPECT_EQ(strat->get_net_leverage_ratio().value(), 1);

	order = strat->forward_market_order(msft_id, (.2499 * cash) / msft_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::PENDING);
	EXPECT_DOUBLE_EQ(strat->get_net_leverage_ratio().value(), 1.2499);

	order = strat->forward_market_order(msft_id, (.1 * cash) / msft_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::REJECTED);
	EXPECT_DOUBLE_EQ(strat->get_net_leverage_ratio().value(), 1.2499);
}


TEST_F(SimplePortfolioFixture, TestMultiAssetOrderValidate) {
	auto portfolio = this->hydra->get_portfolio(portfolio_id_complex);
	auto strategy = std::make_unique<DummyStrategy>(
		portfolio
	);
	this->hydra->register_strategy(std::move(strategy));
	hydra->build();
	auto& exchanges = this->hydra->get_exchanges();

	std::string msft_string = "MSFT"; std::string orcl_string = "ORCL";
	auto msft_id = exchanges.get_asset_index(msft_string);
	auto orcl_id = exchanges.get_asset_index(orcl_string);
	auto msft_price = exchanges.__get_market_price(msft_string, true);
	auto orcle_price = exchanges.__get_market_price(orcl_string, true);

	auto strat_ptr = hydra->__get_strategy(strategy_id_complex);
	auto strat = dynamic_cast<DummyStrategy*>(strat_ptr);

	strat->set_max_leverage(1.25);
	auto order = strat->forward_market_order(msft_id, .5*cash / msft_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::PENDING);
	EXPECT_EQ(strat->get_net_leverage_ratio().value(), .5);
	order = strat->forward_market_order(orcl_id, .5*cash / orcle_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::PENDING);
	EXPECT_EQ(strat->get_net_leverage_ratio().value(), 1);

	order = strat->forward_market_order(orcl_id, -.25 * cash / orcle_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::PENDING);
	EXPECT_EQ(strat->get_net_leverage_ratio().value(), .75);

	order = strat->forward_market_order(msft_id, .4 * cash / msft_price);
	strat->__validate_order(order);
	EXPECT_EQ(order->get_order_state(), OrderState::PENDING);
	EXPECT_EQ(strat->get_net_leverage_ratio().value(), 1.15);
}

TEST_F(SimplePortfolioFixture, TestChildOrderLeverageValidate) {
	auto portfolio = this->hydra->get_portfolio(portfolio_id_complex);
	auto strategy = std::make_unique<DummyStrategy>(
		portfolio
	);
	this->hydra->register_strategy(std::move(strategy));
	hydra->build();
	auto& exchanges = this->hydra->get_exchanges();

	std::string msft_string = "MSFT"; std::string orcl_string = "ORCL";
	auto msft_id = exchanges.get_asset_index(msft_string);
	auto orcl_id = exchanges.get_asset_index(orcl_string);
	auto msft_price = exchanges.__get_market_price(msft_string, true);
	auto orcle_price = exchanges.__get_market_price(orcl_string, true);

	auto strat_ptr = hydra->__get_strategy(strategy_id_complex);
	auto strat = dynamic_cast<DummyStrategy*>(strat_ptr);
}

TEST_F(SimplePortfolioFixture, TestBetaHedge) {
	auto portfolio = this->hydra->get_portfolio(portfolio_id_complex);
	auto& exchanges = this->hydra->get_exchanges();
	auto strategy = std::make_unique<BetaHedgeStrategy>(
		portfolio
	);
	this->hydra->register_strategy(std::move(strategy));
	hydra->set_market_asset(exchange_id_complex, "SPY", true, 252);
	hydra->build();
	hydra->__step();

	std::string msft_string = "MSFT"; std::string orcl_string = "ORCL"; std::string spy_string = "SPY";
	auto msft_id = exchanges.get_asset_index(msft_string);
	auto orcl_id = exchanges.get_asset_index(orcl_string);
	auto spy_id = exchanges.get_asset_index(spy_string);

	auto trade_opt_1 = portfolio->get_trade(msft_id, strategy_id_complex);
	auto trade_opt_2 = portfolio->get_trade(orcl_id, strategy_id_complex);
	auto trade_opt_3 = portfolio->get_trade(spy_id, strategy_id_complex);
	EXPECT_EQ(trade_opt_1.has_value(), true);
	EXPECT_EQ(trade_opt_2.has_value(), true);
	EXPECT_EQ(trade_opt_3.has_value(), true);
	auto& trade_1 = trade_opt_1.value().get();
	auto& trade_2 = trade_opt_2.value().get();
	auto& trade_3 = trade_opt_3.value().get();
	auto msft_price = exchanges.__get_market_price(msft_string, true);
	auto orcl_price = exchanges.__get_market_price(orcl_string, true);
	auto spy_price = exchanges.__get_market_price(spy_string, true);
	auto msft = exchanges.get_asset("MSFT").unwrap();
	auto orcl = exchanges.get_asset("ORCL").unwrap();
	EXPECT_FLOAT_EQ(trade_1->average_price, 26.980000);
	EXPECT_FLOAT_EQ(msft_price, 26.980000);
	EXPECT_FLOAT_EQ(trade_2->average_price, 31.230000);
	EXPECT_FLOAT_EQ(orcl_price, 31.230000);
	EXPECT_FLOAT_EQ(trade_3->average_price, 120.23000335693359);
	EXPECT_FLOAT_EQ(spy_price, 120.23000335693359);

	auto strat_ptr = hydra->__get_strategy(strategy_id_complex);
	auto net_leverage = strat_ptr->get_net_leverage_ratio();
	EXPECT_DOUBLE_EQ(net_leverage.value(), 1.5);

	auto beta = strat_ptr->get_net_beta();
	EXPECT_TRUE(abs(beta.value() - 0.0) < 1e-10);

	for (int i = 0; i < 2000; i++) {
		hydra->__step();
		net_leverage = strat_ptr->get_net_leverage_ratio();
		EXPECT_TRUE(abs(net_leverage.value() - 1.5) < 1e-9);

		beta = strat_ptr->get_net_beta();
		EXPECT_TRUE(abs(beta.value() - 0.0) < 1e-8);
	}
}