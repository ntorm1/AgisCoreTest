#include "pch.h"
#include "helpers.h"
#include "AgisStrategy.h"

namespace AgisExchangeTest {

	std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";
	std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

	std::string asset_id_1 = "test1";
	std::string asset_id_2 = "test2";
	std::string asset_id_3 = "test3";
	std::string exchange_id_1 = "exchange1";
	std::string exchange_id_complex = "SPY_DAILY";

	constexpr long long t0 = 960181200000000000;
	constexpr long long t1 = 960267600000000000;
	constexpr long long t2 = 960354000000000000;
	constexpr long long t3 = 960440400000000000;
	constexpr long long t4 = 960526800000000000;
	constexpr long long t5 = 960786000000000000;
};

using namespace AgisExchangeTest;

class SimpleExchangeTests : public ::testing::Test
{
protected:
	std::shared_ptr<ExchangeMap> exchanges;

	void SetUp() override
	{
		exchanges = AgisTest::create_simple_exchange_map(exchange_id_1, exchange1_path);
	}
};


TEST_F(SimpleExchangeTests, TestIndex) {
	exchanges->__build();

	auto dt_index = exchanges->__get_dt_index();
	EXPECT_EQ(dt_index.size(), 6);
	EXPECT_EQ(dt_index[0], t0);
	EXPECT_EQ(dt_index[1], t1);
	EXPECT_EQ(dt_index[2], t2);
	EXPECT_EQ(dt_index[3], t3);
	EXPECT_EQ(dt_index[4], t4);
	EXPECT_EQ(dt_index[5], t5);
}


TEST_F(SimpleExchangeTests, TestGetMarketPrice) {
	exchanges->__build();
	exchanges->step();

	EXPECT_EQ(exchanges->get_datetime(), t0);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 0);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 0);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 101.5);

	exchanges->step();

	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 100);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 100);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 99);

	exchanges->__reset();
	exchanges->step();

	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 0);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 0);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 101.5);

	exchanges->step();

	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 100);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 100);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 99);
}


TEST_F(SimpleExchangeTests, TestAssetExpire) {
	exchanges->__build();
	exchanges->__goto(t4);

	EXPECT_EQ(exchanges->get_datetime(), t4);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 101.5);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 105);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 106);

	exchanges->step();
	EXPECT_EQ(exchanges->get_datetime(), t5);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 103);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 96);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 0);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 0);
}


TEST_F(SimpleExchangeTests, TestExchangeRemove) {
	exchanges->__build();

	EXPECT_EQ(exchanges->asset_exists(asset_id_2), true);
	EXPECT_EQ(exchanges->get_asset_ids(exchange_id_1).size(), 3);
	exchanges->remove_asset(asset_id_2);
	EXPECT_EQ(exchanges->asset_exists(asset_id_2), false);
	EXPECT_EQ(exchanges->get_asset_ids(exchange_id_1).size(), 2);

	auto exchange = exchanges->get_exchange(exchange_id_1).value();
	EXPECT_EQ(exchange->asset_exists(asset_id_2), false);
	EXPECT_EQ(exchange->get_asset_ids().size(), 2);

	exchanges->__goto(t4);

	EXPECT_EQ(exchanges->get_datetime(), t4);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 105);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 106);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_3, false), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_3, true), 102.4);
}


TEST_F(SimpleExchangeTests, TestExchangeView) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_1).value();

	exchanges->step();
	auto exchange_view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::Default);
	auto& view = exchange_view.view;
	EXPECT_EQ(view.size(), 2);

	exchanges->step();
	exchange_view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::Default);
	view = exchange_view.view;
	EXPECT_EQ(view.size(), 3);

	exchange_view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::NLargest, 1);
	view = exchange_view.view;
	auto index = exchanges->get_asset_index(asset_id_3);
	EXPECT_EQ(view.size(), 1);
	EXPECT_EQ(view[0].asset_index, index);
	EXPECT_EQ(view[0].allocation_amount, 101.4);

	exchange_view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::NSmallest, 1);
	view = exchange_view.view;
	index = exchanges->get_asset_index(asset_id_2);
	EXPECT_EQ(view.size(), 1);
	EXPECT_EQ(view[0].asset_index, index);
	EXPECT_EQ(view[0].allocation_amount, 99);
}


TEST_F(SimpleExchangeTests, TestExchangeViewLambda) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_1).value();

	exchanges->step();
	exchanges->step();
	exchanges->step();

	auto daily_return = [](const AssetPtr& asset) {
		AgisAssetLambdaChain lambda_chain;
		AssetLambda l = AssetLambda(agis_init, [=](const AssetPtr& asset) {
			return asset->get_asset_feature("CLOSE", 0);
			});
		AssetLambdaScruct asset_op{ l, agis_init, "CLOSE", 0 };
		lambda_chain.push_back(asset_op);

		l = AssetLambda(agis_divide, [=](const AssetPtr& asset) {
			return asset->get_asset_feature("CLOSE", -1);
			});
		AssetLambdaScruct asset_op_prev{ l, agis_init, "CLOSE", -1 };
		lambda_chain.push_back(asset_op_prev);
		return asset_feature_lambda_chain(
			asset,
			lambda_chain
		);
		};

	auto exchange_view = exchange->get_exchange_view(
		daily_return,
		ExchangeQueryType::NLargest
	);

	auto& view = exchange_view.view;
	auto index1 = exchanges->get_asset_index(asset_id_1);
	auto index2 = exchanges->get_asset_index(asset_id_2);
	EXPECT_EQ(view.size(), 3);
	EXPECT_EQ(view[0].asset_index, index1);
	EXPECT_EQ(view[0].allocation_amount, (103.0 / 101));
	EXPECT_EQ(view[1].asset_index, index2);
	EXPECT_EQ(view[1].allocation_amount, (97.0 / 99));
}