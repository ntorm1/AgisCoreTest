#include "pch.h"

#include "Hydra.h"

namespace StrategyTest {

	std::string exchange1_path_h = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";

	std::string asset_id_1_h = "test1";
	std::string asset_id_2_h = "test2";
	std::string asset_id_3_h = "test3";
	std::string portfolio_id_1_h = "portfolio1";
	std::string exchange_id_1_h = "exchange1";

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
	constexpr double cash = 100000;
}

using namespace StrategyTest;

class SimpleStrategyFixture : public ::testing::Test {
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


class ConditionalSplitStrategyFixture : public AgisStrategy {
public:
	ConditionalSplitStrategyFixture(PortfolioPtr const portfolio_)
		: AgisStrategy("test", portfolio_, 1) {}

	int i = 0;

	void next() override {
		if (i <= 1) { i++; return; }
		// get asset returns
		auto view = this->exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::Default, -1);
		auto prev_close = this->exchange->get_exchange_view("CLOSE", -1, ExchangeQueryType::Default, -1);
		view -= prev_close;

		// conditional split on returns greater than 0
		view.conditional_split(1, 0.0);
		this->strategy_allocate(view,0.0,false,std::nullopt,AllocType::PCT);
	}

	void reset() override {
		this->i = 0;
	}
	void build() override {
		this->exchange_subscribe(exchange_id_1_h);
		this->exchange = this->get_exchange();
	}

	ExchangePtr exchange;
};


std::optional<ExchangeViewLambdaStruct> extract_ret_strategy(ExchangePtr exchange)
{
	AgisAssetLambdaChain lambda_chain;
	auto close_index = exchange->get_column_index("CLOSE").unwrap();
	AssetLambda l = AssetLambda(agis_init, [=](const AssetPtr& asset) {
		return asset->get_asset_feature(close_index, -0);
		});
	AssetLambdaScruct a{ l, agis_init, "", 0 };
	lambda_chain.push_back(a);

	l = AssetLambda(agis_subtract, [=](const AssetPtr& asset) {
		return asset->get_asset_feature(close_index, -1);
		});
	AssetLambdaScruct b{ l, agis_subtract, "", -1 };
	lambda_chain.push_back(b);

	l = AssetLambda(agis_divide, [=](const AssetPtr& asset) {
		return asset->get_asset_feature(close_index, -1);
		});
	AssetLambdaScruct c{ l, agis_divide, "", -1 };
	lambda_chain.push_back(c);

	ExchangeViewLambda ev_chain = [](
		AgisAssetLambdaChain const& lambda_chain,
		ExchangePtr const exchange,
		ExchangeQueryType query_type,
		int N) -> ExchangeView
	{
		// function that takes in serious of operations to apply to as asset and outputs
		// a double value that is result of said opps
		auto asset_chain = [&](AssetPtr const& asset) -> AgisResult<double> {
			return asset_feature_lambda_chain(
				asset,
				lambda_chain
			);
		};

		// function that takes an exchange an applys the asset chain to each element when 
		// generating the exchange view
		auto exchange_view = exchange->get_exchange_view(
			asset_chain,
			query_type,
			N,
			false,
			1
		);
		return exchange_view;
	};

	ExchangeViewLambdaStruct ev_lambda_struct = {
			2,
			1,
			lambda_chain,
			ev_chain,
			exchange,
			ExchangeQueryType::NLargest
	};

	StrategyAllocLambdaStruct _struct{
		.0,
		1,
		std::nullopt,
		std::nullopt,
		true,
		"UNIFORM",
		AllocType::PCT
	};
	ev_lambda_struct.strat_alloc_struct = _struct;
	return ev_lambda_struct;
}


TEST_F(SimpleStrategyFixture, TestStrategyBuild) {
	hydra->new_portfolio(
		portfolio_id_1_h,
		cash
	);
	auto portfolio = this->hydra->get_portfolio(portfolio_id_1_h);
	auto strategy = std::make_unique<AbstractAgisStrategy>(
		portfolio,
		"test",
		1
	);
	auto exchange = hydra->get_exchanges().get_exchange(exchange_id_1_h);
	strategy->set_abstract_ev_lambda([&]() {
		return extract_ret_strategy(exchange);
		});
	strategy->extract_ev_lambda();
	this->hydra->register_strategy(std::move(strategy));

	hydra->build();
	auto& trade_history = portfolio->get_trade_history();
	auto strat = hydra->get_strategy("test");
	auto id1 = hydra->get_exchanges().get_asset_index(asset_id_1_h);
	auto id2 = hydra->get_exchanges().get_asset_index(asset_id_2_h);
	auto id3 = hydra->get_exchanges().get_asset_index(asset_id_3_h);
		
	hydra->__step();
	auto pos_count = portfolio->get_strategy_positions(strat->get_strategy_index());
	EXPECT_EQ(pos_count.size(), 0);
		
	hydra->__step();
	auto trade_opt_2 = portfolio->get_trade(id2, "test");
	auto trade_opt_3 = portfolio->get_trade(id3, "test");
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
	EXPECT_FLOAT_EQ(portfolio->get_nlv(), nlv);
	EXPECT_EQ(trade_history.size(), 1);
	EXPECT_EQ(portfolio->position_exists(id1),true);
	EXPECT_EQ(portfolio->position_exists(id2), true);
	EXPECT_EQ(portfolio->position_exists(id3), false);
	auto t = trade_history[0];
	EXPECT_EQ(t->units,u3);
	EXPECT_EQ(t->unrealized_pl, 0);
	EXPECT_EQ(t->realized_pl, u3 * (88-101.4));
	auto trade_opt_1 = portfolio->get_trade(id1, "test");
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
	EXPECT_EQ(trade_2->realized_pl, (97 - 99)*(u2 - trade_2->units));
	EXPECT_EQ(trade_2->average_price,99); // average price doesn't change as we sold units

	hydra->__step();
	nlv = nlv + ((105 - 103) * u1) + ((101.5 - 97) * u2_new);
	EXPECT_FLOAT_EQ(portfolio->get_nlv(), nlv);
}

TEST_F(SimpleStrategyFixture, TestStrategySplitAlloc) {
	hydra->new_portfolio(
		portfolio_id_1_h,
		cash
	);
	auto portfolio = this->hydra->get_portfolio(portfolio_id_1_h);
	auto strategy = std::make_unique<ConditionalSplitStrategyFixture>(
		portfolio
	);
	hydra->register_strategy(std::move(strategy));

	auto strategy_ref = hydra->get_strategy("test");
	auto& trade_history = portfolio->get_trade_history();

	auto id1 = hydra->get_exchanges().get_asset_index(asset_id_1_h);
	auto id2 = hydra->get_exchanges().get_asset_index(asset_id_2_h);
	auto id3 = hydra->get_exchanges().get_asset_index(asset_id_3_h);

	hydra->build();
	hydra->__step();
	hydra->__step();
	hydra->__step();

	auto trade_opt_1 = portfolio->get_trade(id1, "test");
	auto trade_opt_2 = portfolio->get_trade(id2, "test");
	auto trade_opt_3 = portfolio->get_trade(id3, "test");
	EXPECT_EQ(trade_opt_1.has_value(), true);
	EXPECT_EQ(trade_opt_2.has_value(), true);
	EXPECT_EQ(trade_opt_3.has_value(), true);
	auto& trade_1 = trade_opt_1.value().get();
	auto& trade_2 = trade_opt_2.value().get();
	auto& trade_3 = trade_opt_3.value().get();

	auto u1 = (1.0f / 3) * cash / 103;
	auto u2 = (-1.0f / 3) * cash / 97;
	auto u3 = (-1.0f / 3) * cash / 88;
	EXPECT_FLOAT_EQ(trade_1->units, u1);
	EXPECT_FLOAT_EQ(trade_2->units, u2);
	EXPECT_FLOAT_EQ(trade_3->units, u3);
	
	EXPECT_FLOAT_EQ(portfolio->get_nlv(), cash);
	auto new_cash = cash - (u1*103.0f + u2*97.0f + u3*88.0f);
	EXPECT_FLOAT_EQ(portfolio->get_cash(), new_cash);
	EXPECT_EQ(trade_history.size(), 0);

	hydra->__step();
	EXPECT_EQ(trade_history.size(), 2);

	double nlv = cash + ((105 - 103) * u1);
	nlv += ((101.5 - 97) * u2);
	nlv += ((89.3 - 88) * u3);
	EXPECT_FLOAT_EQ(portfolio->get_nlv(), nlv);
	EXPECT_FLOAT_EQ(strategy_ref->get_nlv(), nlv);

	trade_opt_1 = portfolio->get_trade(id1, "test");
	trade_opt_2 = portfolio->get_trade(id2, "test");
	trade_opt_3 = portfolio->get_trade(id3, "test");
	EXPECT_EQ(trade_opt_1.has_value(), true);
	EXPECT_EQ(trade_opt_2.has_value(), true);
	EXPECT_EQ(trade_opt_3.has_value(), true);
	auto& trade_1_next = trade_opt_1.value().get();
	auto& trade_2_next = trade_opt_2.value().get();
	auto& trade_3_next = trade_opt_3.value().get();


	auto u1_new = (1.0f / 3) * nlv / 105;
	auto u2_new = (1.0f / 3) * nlv / 101.5;
	auto u3_new = (1.0f / 3) * nlv / 89.3;
	EXPECT_FLOAT_EQ(trade_1_next->units, u1_new);
	EXPECT_FLOAT_EQ(trade_2_next->units, u2_new);
	EXPECT_FLOAT_EQ(trade_3_next->units, u3_new);
}