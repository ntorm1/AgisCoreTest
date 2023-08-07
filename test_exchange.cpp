#include "pch.h"

#include "Asset.h"
#include "Exchange.h"
#include "Utils.h"
#include "AgisStrategy.h"


std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";
std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\Data\\DataBento\\TechTest\\hdf5\\data.h5";


std::string asset_id_1 = "test1";
std::string asset_id_2 = "test2";
std::string asset_id_3 = "test3";
std::string exchange_id_1 = "exchange1";
std::string exchange_id_complex = "TechMinute";

constexpr long long t0 = 960181200000000000;
constexpr long long t1 = 960267600000000000;
constexpr long long t2 = 960354000000000000;
constexpr long long t3 = 960440400000000000;
constexpr long long t4 = 960526800000000000;
constexpr long long t5 = 960786000000000000;

class SimpleExchangeFixture : public ::testing::Test {
protected:
	std::shared_ptr<ExchangeMap> exchanges;

	void SetUp() override {
		this->exchanges = std::make_shared<ExchangeMap>();
		this->exchanges->new_exchange(
			exchange_id_1,
			exchange1_path,
			Frequency::Day1,
			"%Y-%m-%d"
		);
	}
};

class ComplexExchangeFixture : public ::testing::Test {
protected:
	std::shared_ptr<ExchangeMap> exchanges;

	void SetUp() override {
		this->exchanges = std::make_shared<ExchangeMap>();
		this->exchanges->new_exchange(
			exchange_id_complex,
			exchange_complex_path,
			Frequency::Min1,
			"%Y-%m-%d"
		);
	}
};

TEST_F(SimpleExchangeFixture, TestIndex) {
	exchanges->__build();

	auto _asset = exchanges->get_asset(asset_id_2).value();
	auto _asset1 = exchanges->get_asset(asset_id_1).value();

	EXPECT_EQ(_asset->__is_aligned, true);
	EXPECT_EQ(_asset->__is_streaming, true);
	EXPECT_EQ(_asset1->__is_aligned, false);
	EXPECT_EQ(_asset1->__is_streaming, false);

	auto dt_index = exchanges->__get_dt_index();
	EXPECT_EQ(dt_index.size(), 6);
	EXPECT_EQ(dt_index[0], t0);
	EXPECT_EQ(dt_index[1], t1);
	EXPECT_EQ(dt_index[2], t2);
	EXPECT_EQ(dt_index[3], t3);
	EXPECT_EQ(dt_index[4], t4);
	EXPECT_EQ(dt_index[5], t5);
}

TEST_F(SimpleExchangeFixture, TestGetMarketPrice) {
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

TEST_F(SimpleExchangeFixture, TestGoto) {
	exchanges->__build();
	exchanges->__goto(t2);

	EXPECT_EQ(exchanges->get_datetime(), t2);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 98);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 97);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 102);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 103);

	exchanges->step();
	EXPECT_EQ(exchanges->get_datetime(), t3);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, false), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_2, true), 101.5);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 104);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 105);

}

TEST_F(SimpleExchangeFixture, TestAssetExpire) {
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

TEST_F(SimpleExchangeFixture, TestExchangeView) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_1);
	
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
	EXPECT_EQ(view[0].first, index);
	EXPECT_EQ(view[0].second,101.4);

	exchange_view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::NSmallest, 1);
	view = exchange_view.view;
	index = exchanges->get_asset_index(asset_id_2);
	EXPECT_EQ(view.size(), 1);
	EXPECT_EQ(view[0].first, index);
	EXPECT_EQ(view[0].second, 99);
}

TEST_F(SimpleExchangeFixture, TestExchangeViewLambda) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_1);

	exchanges->step();
	exchanges->step();
	exchanges->step();
	
	auto daily_return = [](const AssetPtr& asset) -> double {
		AgisAssetLambdaChain lambda_chain;
		AssetLambda l = AssetLambda(agis_init, [=](const AssetPtr& asset) {
			return asset_feature_lambda(asset, "CLOSE", 0).unwrap();
			});
		AssetLambdaScruct ls{ l,"CLOSE", 0 };
		lambda_chain.push_back(ls);
		l = AssetLambda(agis_divide, [=](const AssetPtr& asset) {
			return asset_feature_lambda(asset, "CLOSE", -1).unwrap();
			});
		ls = { l,"CLOSE",-1 };
		lambda_chain.push_back(ls);
		double result = asset_feature_lambda_chain(
			asset, 
			lambda_chain
		);
		return result;
	};

	auto exchange_view = exchange->get_exchange_view(
		daily_return, 
		ExchangeQueryType::NLargest
	);

	auto& view = exchange_view.view;
	auto index1 = exchanges->get_asset_index(asset_id_1);
	auto index2 = exchanges->get_asset_index(asset_id_2);
	EXPECT_EQ(view.size(), 3);
	EXPECT_EQ(view[0].first, index1);
	EXPECT_EQ(view[0].second, (103.0/101));
	EXPECT_EQ(view[1].first, index2);
	EXPECT_EQ(view[1].second, (97.0 / 99));
}


TEST_F(ComplexExchangeFixture, TestExchangeHDF5Load) {
	exchanges->__build();
	
	auto dt_index = exchanges->__get_dt_index();
	EXPECT_EQ(dt_index[0], 1686729600000000000);
	EXPECT_EQ(dt_index[dt_index.size()-1], 1689292740000000000);
	
	auto first_open = 1686749400000000000LL;
	exchanges->__goto(first_open);

	std::string msft = "MSFT"; std::string orcl = "ORCL";
	EXPECT_EQ(exchanges->__get_market_price(msft, false), 334.02);
	EXPECT_EQ(exchanges->__get_market_price(msft, true), 334.91);
	EXPECT_EQ(exchanges->__get_market_price(orcl, false), 116.50);
	EXPECT_EQ(exchanges->__get_market_price(orcl, true), 116.42);

	exchanges->step();
	EXPECT_EQ(exchanges->__get_market_price(msft, false), 334.84);
	EXPECT_EQ(exchanges->__get_market_price(msft, true), 335.22);
	EXPECT_EQ(exchanges->__get_market_price(orcl, false), 116.44);
	EXPECT_EQ(exchanges->__get_market_price(orcl, true), 116.80);
}