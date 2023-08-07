#include "pch.h"
#include <chrono>
#include <memory>
#include "Asset.h"
#include "Hydra.h"
#include "Utils.h"

std::string exchange1_path_h = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange2";

std::string asset_id_1_h = "test1";
std::string asset_id_2_h = "test2";
std::string portfolio_id_1_h = "portfolio1";
std::string exchange_id_1_h = "exchange1";

constexpr long long t0 = 960181200000000000;
constexpr long long t1 = 960267600000000000;
constexpr long long t2 = 960354000000000000;
constexpr long long t3 = 960440400000000000;
constexpr long long t4 = 960526800000000000;
constexpr long long t5 = 960786000000000000;
constexpr double cash = 100000;

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;


class SimpleHydraFixture : public ::testing::Test {
protected:
	std::unique_ptr<Hydra> hydra;

	void SetUp() override {
		this->hydra = std::make_unique<Hydra>(0);
		this->hydra->new_exchange(
			exchange_id_1_h,
			exchange1_path_h,
			Frequency::Day1,
			"%Y-%m-%d"
		);

	}
};


class SimpleStrategy : public AgisStrategy {
public:
	SimpleStrategy(PortfolioPtr const& portfolio_) 
		: AgisStrategy("test",portfolio_, .5) {}

	int i = 0;

	void next() override{
		if (i == 0)
		{
			this->place_market_order(
				asset_id_2_h,
				100,
				std::make_unique<ExitBars>(3)
			);
			i++;
		}
	}

	void reset() override {
		this->i = 0;
	}
	void build() override {
		this->exchange_subscribe(exchange_id_1_h);
	}
	
};


class BandStrategy : public AgisStrategy {
public:
	BandStrategy(std::string id, PortfolioPtr const& portfolio_, double ub_, double lb_)
		: AgisStrategy(id, portfolio_, .5) {
		this->ub = ub_;
		this->lb = lb_;
	}

	void next() override {
		auto trade = this->get_trade(asset_id_2_h);
		if (!trade.has_value())
		{
			this->place_market_order(
				asset_id_2_h,
				100,
				std::make_unique<ExitBand>(this->ub, this->lb)
			);
		}
	}

	void reset() override {}
	void build() override {
		this->exchange_subscribe(exchange_id_1_h);
	}
	double ub;
	double lb;
};


class AllocStrategy : public AgisStrategy {
public:
	AllocStrategy(std::string id, PortfolioPtr const& portfolio_, double portfolio_allocation_) :
		AgisStrategy(id, portfolio_, portfolio_allocation_){
		
	}

	void build() override {
		this->exchange_subscribe(exchange_id_1_h);
		this->exchange = this->get_exchange(exchange_id_1_h);
	}

	ExchangePtr exchange;

	void next() override {
		auto view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::NLargest, 1);
		agis_realloc(&view, 1);
		this->strategy_allocate(&view, 0, true, std::nullopt, AllocType::PCT);
	}

	void reset() override {}
};

class EvenAllocStrategy : public AgisStrategy {
public:
	EvenAllocStrategy(
		std::string id, 
		PortfolioPtr const& portfolio_, 
		double portfolio_allocation_,
		bool is_long_) :
		AgisStrategy(id, portfolio_, portfolio_allocation_) {
		this->is_long = is_long_;
	}

	void build() override {
		this->exchange_subscribe(exchange_id_1_h);
		this->exchange = this->get_exchange(exchange_id_1_h);
	}

	void next() override {
		auto view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::Default);
		auto factor = static_cast<double>(1) / view.size();
		if (!this->is_long) { factor *= -1; }
		agis_realloc(&view, factor);
		this->strategy_allocate(&view, 0, true, std::nullopt, AllocType::PCT);
	}

	void reset() override {}

	bool is_long;
	ExchangePtr exchange;
};

void test_simple(Hydra* hydra, bool r) {
	auto& portfolio = hydra->get_portfolio(portfolio_id_1_h);
	if (r)
	{
		auto strategy = std::make_unique<SimpleStrategy>(portfolio);
		hydra->register_strategy(std::move(strategy));
	}
	hydra->build();
	hydra->__step();

	// verift the order was filled
	auto& order_history = hydra->get_order_history();
	EXPECT_EQ(order_history.size(), 1);
	auto& order = order_history[0];
	EXPECT_EQ(order->get_order_state(), OrderState::FILLED);
	EXPECT_EQ(order->get_average_price(), 101.5);
	EXPECT_EQ(order->get_fill_time(), t0);

	// verify that the order altered portfolio cash levels, but not nlv. Order was placed
	// at close so no effect is taken as the eval price is the fill price
	auto x = cash - (101.5 * 100);
	EXPECT_EQ(portfolio->get_cash(), x);
	EXPECT_EQ(portfolio->get_nlv(), cash);

	hydra->__step();
	EXPECT_EQ(portfolio->get_cash(), x);
	EXPECT_EQ(portfolio->get_nlv(), cash - ((101.5 - 99)*100) );

	// verify the position is as expected
	auto asset_index = hydra->get_exchanges().get_asset_index(asset_id_2_h);
	auto position = portfolio->get_position(asset_index);
	EXPECT_TRUE(position.has_value());

	// verify position stats
	auto& p = position.value().get();
	EXPECT_EQ(p->realized_pl, 0);
	EXPECT_EQ(p->average_price, 101.5);
	EXPECT_EQ(p->last_price, 99.0);
	EXPECT_EQ(p->unrealized_pl, (99-101.5)*100);
	EXPECT_EQ(p->bars_held, 2);

	// make sure the trade exit was completed correctly
	hydra->__step();
	position = portfolio->get_position(asset_index);
	EXPECT_TRUE(!position.has_value());
	EXPECT_EQ(order_history.size(), 2);
	auto& order2 = order_history[1];
	EXPECT_EQ(order2->get_order_state(), OrderState::FILLED);
	EXPECT_EQ(order2->get_average_price(), 97);
	EXPECT_EQ(order2->get_fill_time(), t2);

	// make sure object values are correct
	double pl = (97 - 101.5) * 100;
	auto& trade_history = portfolio->get_trade_history();
	auto& position_history = portfolio->get_position_history();
	EXPECT_EQ(trade_history.size(), 1);
	EXPECT_EQ(position_history.size(), 1);
	auto& trade = trade_history[0];
	auto& position2 = position_history[0];
	EXPECT_EQ(trade->trade_close_time, t2);
	EXPECT_EQ(position2->position_close_time, t2);
	EXPECT_EQ(trade->close_price, 97);
	EXPECT_EQ(position2->close_price, 97);
	EXPECT_EQ(trade->realized_pl, pl);
	EXPECT_EQ(position2->realized_pl, pl);
	EXPECT_EQ(trade->unrealized_pl, 0);
	EXPECT_EQ(position2->unrealized_pl, 0);

	EXPECT_EQ(portfolio->get_nlv(), cash + pl);
	EXPECT_EQ(portfolio->get_cash(), cash + pl);
	
	EXPECT_EQ(true, true);
}


TEST_F(SimpleHydraFixture, TestHydraBuild) {
	hydra->new_portfolio(
		portfolio_id_1_h,
		cash
	);
	test_simple(hydra.get(), true);
	hydra->__reset();
	test_simple(hydra.get(), false);
}


TEST_F(SimpleHydraFixture, TestHydraMultiStrat) {
	int n = 100;
	int counter = 0;
	for (int i = 0; i < n; i++) {
		hydra->new_portfolio(
			std::to_string(i),
			cash
		);

		auto& portfolio = hydra->get_portfolio(std::to_string(i));
		auto strategy1 = std::make_unique<BandStrategy>(
			std::to_string(counter), portfolio, 103, 95);
		counter++;
		auto strategy2 = std::make_unique<BandStrategy>(
			std::to_string(counter), portfolio, 103, 97);
		counter++;
		hydra->register_strategy(std::move(strategy1));
		hydra->register_strategy(std::move(strategy2));
	}

	hydra->build();
	hydra->__run();

	for (int i = 0; i < n; i++) 
	{
		auto& portfolio = hydra->get_portfolio(std::to_string(i));
		auto& trade_history = portfolio->get_trade_history();
		auto& position_history = portfolio->get_position_history();
		//auto& order_history = hydra->get_order_history();

		//EXPECT_EQ(order_history.size(), 5);
		EXPECT_EQ(trade_history.size(), 2);
		EXPECT_EQ(position_history.size(), 0);

		double nlv = (cash)+((97 - 101.5) * 100) + 2 * ((96 - 101.5) * 100);
		double _cash = (cash)-(101.5 * 100) + ((97-101.5)*100) +((96 - 101.5) * 100);
		EXPECT_EQ(portfolio->get_nlv(), nlv);
		EXPECT_EQ(portfolio->get_cash(), _cash);
	}

	for (int i = 0; i < counter; i++)
	{
		auto strategy_ref = hydra->get_strategy(std::to_string(i));
		auto& strategy = strategy_ref.get();
		std::vector<OrderRef> const& order_history = strategy->get_order_history();
		if (i % 2 == 0)
		{
			EXPECT_EQ(order_history.size(), 1);
		}
		else
		{
			EXPECT_EQ(order_history.size(), 4);
		}
	}

	EXPECT_EQ(true, true);
}

TEST_F(SimpleHydraFixture, TestHydraRealloc) {
	auto& portfolio = hydra->new_portfolio(
		"test",
		cash
	);
	auto strategy1 = std::make_unique<AllocStrategy>(
		"p1", portfolio, 1);
	auto& trade_history = strategy1->get_trade_history();

	hydra->register_strategy(std::move(strategy1));
	hydra->build();
	hydra->__step();
	
	auto id1 = hydra->get_exchanges().get_asset_index(asset_id_1_h);
	auto id2 = hydra->get_exchanges().get_asset_index(asset_id_2_h);
	auto trade_opt = portfolio->get_trade(id2, "p1");
	EXPECT_EQ(trade_opt.has_value(), true);
	auto& trade = trade_opt.value().get();
	EXPECT_EQ(trade->units, cash / 101.5);
	EXPECT_EQ(trade->unrealized_pl, 0);
	EXPECT_EQ(trade->realized_pl, 0);

	hydra->__step();

	EXPECT_EQ(trade_history.size(), 1);
	std::shared_ptr<Trade> trade2 = trade_history.at(0);
	auto units = cash / 101.5;
	EXPECT_EQ(trade2->units, units);
	EXPECT_EQ(trade2->realized_pl, units*(99 - 101.5));
	EXPECT_EQ(trade2->unrealized_pl, 0);
	EXPECT_EQ(trade2->trade_open_time, t0);
	EXPECT_EQ(trade2->trade_close_time, t1);
	EXPECT_EQ(trade2->bars_held, 1);
}

TEST_F(SimpleHydraFixture, TestHydraMultAlloc) {
	auto& portfolio = hydra->new_portfolio(
		"test",
		cash
	);
	auto strategy1 = std::make_unique<EvenAllocStrategy>(
		"long", portfolio, .5, true);
	auto strategy2 = std::make_unique<EvenAllocStrategy>(
		"short", portfolio, .5, false);

	auto& trade_history = strategy1->get_trade_history();
	auto& order_history = hydra->get_order_history();

	hydra->register_strategy(std::move(strategy1));
	hydra->register_strategy(std::move(strategy2));

	hydra->build();
	hydra->__step();

	auto id1 = hydra->get_exchanges().get_asset_index(asset_id_1_h);
	auto id2 = hydra->get_exchanges().get_asset_index(asset_id_2_h);

	EXPECT_FLOAT_EQ(portfolio->get_nlv(), cash);
	EXPECT_EQ(portfolio->get_unrealized_pl(), 0);

	auto trade_opt_l = portfolio->get_trade(id2, "long");
	auto trade_opt_S = portfolio->get_trade(id2, "short");
	auto p2 = portfolio->get_position(id2).value();
	EXPECT_EQ(trade_opt_l.has_value(), true);
	auto& trade_l = trade_opt_l.value().get();
	EXPECT_EQ(trade_opt_S.has_value(), true);
	auto& trade_s = trade_opt_S.value().get();

	EXPECT_EQ(trade_l->units, .5*cash / 101.5);
	EXPECT_EQ(trade_s->units, -.5*cash / 101.5);
	EXPECT_EQ(trade_l->unrealized_pl,0);
	EXPECT_EQ(trade_s->unrealized_pl,0);
	EXPECT_EQ(p2.get()->get_nlv(), 0);
	EXPECT_EQ(p2.get()->get_units(), 0);

	hydra->__step();
	EXPECT_EQ(portfolio->get_unrealized_pl(), 0);
	auto x = portfolio->get_nlv();
	EXPECT_EQ(x, cash);

	auto u0 = gmp_div(gmp_mult(.5, cash), 101.5);
	auto u2 = gmp_div(gmp_mult(.25, cash), 99);
	auto u1 = -1*gmp_sub(u2,u0);

	EXPECT_EQ(trade_l->units, u2);
	EXPECT_EQ(
		trade_l->realized_pl, 
		(u1 * (99 -101.5))
	);
	EXPECT_EQ(
		trade_l->unrealized_pl,
		(u2 * (99 - 101.5))
	);
	EXPECT_EQ(trade_s->units, -1*u2);
	EXPECT_EQ(
		trade_s->realized_pl,
		(-1*u1 * (99 - 101.5))
	);
	EXPECT_EQ(
		trade_s->unrealized_pl,
		(-1*u2*(99 - 101.5))
	);

	p2 = portfolio->get_position(id2).value();
	EXPECT_EQ(p2.get()->get_nlv(), 0);
	EXPECT_EQ(p2.get()->get_units(), 0);
	auto trade2_opt_l = portfolio->get_trade(id1, "long");
	auto trade2_opt_S = portfolio->get_trade(id1, "short");
	EXPECT_EQ(trade_opt_l.has_value(), true);
	auto& trade2_l = trade_opt_l.value().get();
	EXPECT_EQ(trade_opt_S.has_value(), true);
	auto& trade2_s = trade_opt_S.value().get();
	//EXPECT_EQ(trade2_l->units, .25 * cash / 101);
	//EXPECT_EQ(trade2_s->units, -1*.25 * cash / 101);
	
	//EXPECT_EQ(trade_s->realized_pl, (-.25 * cash / 101.5) * (99 - 101.5));
	/*

	auto trade_opt = portfolio->get_trade(id2, "p1");
	EXPECT_EQ(trade_opt.has_value(), true);
	auto& trade = trade_opt.value().get();
	EXPECT_EQ(trade->units, cash / 101.5);
	EXPECT_EQ(trade->unrealized_pl, 0);
	EXPECT_EQ(trade->realized_pl, 0);
	*/
	//hydra->__step();

	/*
	EXPECT_EQ(trade_history.size(), 1);
	std::shared_ptr<Trade> trade2 = trade_history.at(0);
	auto units = cash / 101.5;
	EXPECT_EQ(trade2->units, units);
	EXPECT_EQ(trade2->realized_pl, units * (99 - 101.5));
	EXPECT_EQ(trade2->unrealized_pl, 0);
	EXPECT_EQ(trade2->trade_open_time, t0);
	EXPECT_EQ(trade2->trade_close_time, t1);
	EXPECT_EQ(trade2->bars_held, 1);
	*/
}