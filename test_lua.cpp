#include "pch.h"

#include "Hydra.h"
#include "AgisLuaStrategy.h"

namespace LuaTest {

	std::string exchange1_path_h = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";

	std::string asset_id_1_h = "test1";
	std::string asset_id_2_h = "test2";
	std::string asset_id_3_h = "test3";
	std::string portfolio_id_1_h = "portfolio1";
	std::string exchange_id_1_h = "exchange1";
	std::string strategy_id_1_h = "test_strat";

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
	constexpr double cash = 100000;
}

using namespace LuaTest;

class SimpleLuaFixture : public ::testing::Test {
protected:
	std::unique_ptr<Hydra> hydra;

	void SetUp() override {
		this->hydra = std::make_unique<Hydra>(0, true);
		this->hydra->new_exchange(
			exchange_id_1_h,
			exchange1_path_h,
			Frequency::Day1,
			"%Y-%m-%d"
		);

	}
};

TEST_F(SimpleLuaFixture, TestLuaStrategyBuild) {
	hydra->new_portfolio(
		portfolio_id_1_h,
		cash
	);
	auto portfolio = this->hydra->get_portfolio(portfolio_id_1_h);
	std::string script = R"(
function test_strat_next(strategy)
	-- Custom Lua implementation of next()
end

function test_strat_reset(strategy)   
 -- Custom Lua implementation of reset()
end

function test_strat_build(strategy)
    -- Custom Lua implementation of build() 
	-- set the strategy params
	strategy:set_net_leverage_trace(true)
	strategy:exchange_subscribe("exchange1")
	
	-- build the strategy asset logic
	exchange = strategy:get_exchange("exchange1")
	exchange_node = create_exchange_node(exchange)
	lambda_init = create_asset_lambda_read("CLOSE", 0)
	lambda_prev = create_asset_lambda_read("CLOSE", -1)
	lambda_opp = create_asset_lambda_opp(lambda_init, lambda_prev, "SUBTRACT")
	lambda_div = create_asset_lambda_read("CLOSE", -1)
	lambda_opp = create_asset_lambda_opp(lambda_opp, lambda_div, "DIVIDE")
	
	-- build the exchange view logic
	ev_opp = create_exchange_view_node(exchange_node, lambda_opp)
	sort_node = create_sort_node(ev_opp, 2, ExchangeQueryType.NLargest)
	
	-- build the strategy allocation logic
	gen_alloc_node = create_gen_alloc_node(sort_node, ExchangeViewOpp.UNIFORM, 1.0, nil)
	strategy_alloc_node = create_strategy_alloc_node(strategy, gen_alloc_node, 0.0, true, nil, AllocType.PCT)
	strategy:set_allocation_node(strategy_alloc_node)
end
)";
	auto strategy = std::make_unique<AgisLuaStrategy>(
		portfolio,
		strategy_id_1_h,
		1,
		script
	);

	hydra->register_strategy(std::move(strategy));
	auto strategy_ref = hydra->__get_strategy(strategy_id_1_h);
	auto res = hydra->build();
	EXPECT_EQ(res.is_exception(), false);

	// cast strat ref to lua strat
	auto lua_strat = dynamic_cast<AgisLuaStrategy*>(strategy_ref);

	ASSERT_NE(strategy_ref, nullptr);
	EXPECT_TRUE(strategy_ref->__is_net_lev_trace());
	EXPECT_TRUE(strategy_ref->__is_exchange_subscribed());

	auto& trade_history = portfolio->get_trade_history();
	auto strat = hydra->get_strategy(strategy_id_1_h);
	auto id1 = hydra->get_exchanges().get_asset_index(asset_id_1_h);
	auto id2 = hydra->get_exchanges().get_asset_index(asset_id_2_h);
	auto id3 = hydra->get_exchanges().get_asset_index(asset_id_3_h);

	hydra->__step();
	
	auto pos_count = portfolio->get_strategy_positions(strat->get_strategy_index());
	EXPECT_EQ(pos_count.size(), 0);

	hydra->__step();
	auto trade_opt_2 = portfolio->get_trade(id2, strategy_id_1_h);
	auto trade_opt_3 = portfolio->get_trade(id3, strategy_id_1_h);
	EXPECT_EQ(trade_opt_2.has_value(), true);
	EXPECT_EQ(trade_opt_3.has_value(), true);
	auto& trade_2 = trade_opt_2.value().get();
	auto& trade_3 = trade_opt_3.value().get();
	EXPECT_EQ(trade_2->units, .5 * cash / 99);
	EXPECT_EQ(trade_3->units, .5 * cash / 101.4);
	auto u2 = trade_2->units;
	auto u3 = trade_3->units;

	hydra->__step();
	auto nlv = cash + ((88 - 101.4) * u3) + ((97 - 99) * u2);
	EXPECT_TRUE(abs(portfolio->get_nlv() - nlv) < 1e-10);
	EXPECT_EQ(trade_history.size(), 1);
	EXPECT_EQ(portfolio->position_exists(id1), true);
	EXPECT_EQ(portfolio->position_exists(id2), true);
	EXPECT_EQ(portfolio->position_exists(id3), false);
	auto t = trade_history[0];
	EXPECT_EQ(t->units, u3);
	EXPECT_EQ(t->unrealized_pl, 0);
	EXPECT_EQ(t->realized_pl, u3 * (88 - 101.4));
	auto trade_opt_1 = portfolio->get_trade(id1, strategy_id_1_h);
	EXPECT_EQ(trade_opt_1.has_value(), true);
	auto& trade_1 = trade_opt_1.value().get();

	auto u2_new = .5 * nlv / 97;
	auto u1 = trade_1->units;
	auto p1 = portfolio->get_position(id1);
	auto p2 = portfolio->get_position(id2);
	EXPECT_EQ(trade_1->units, .5 * nlv / 103);
	EXPECT_EQ(trade_2->units, u2_new);
	EXPECT_EQ(p1.value().get()->get_units(), .5 * nlv / 103);
	EXPECT_EQ(p2.value().get()->get_units(), u2_new);
	EXPECT_EQ(p2.value().get()->get_units(), trade_2->units);
	EXPECT_EQ(trade_1->realized_pl, 0);
	EXPECT_EQ(trade_2->realized_pl, (97 - 99) * (u2 - trade_2->units));
	EXPECT_EQ(trade_2->average_price, 99); // average price doesn't change as we sold units

	hydra->__step();
	nlv = nlv + ((105 - 103) * u1) + ((101.5 - 97) * u2_new);
	EXPECT_TRUE(abs(portfolio->get_nlv() - nlv) < 1e-10);
}