#include "pch.h"

#include "Asset.h"
#include "Exchange.h"
#include "Portfolio.h"
#include "Utils.h"
#include "AgisStrategy.h"
#include <ankerl/unordered_dense.h>

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
		this->exchanges->restore_exchange(exchange_id_1);
	}
};

class ComplexExchangeFixture : public ::testing::Test {
protected:
	std::shared_ptr<ExchangeMap> exchanges;
	std::vector<std::string> asset_ids = { "SPY", "MSFT", "ORCL" };

	void SetUp() override {
		this->exchanges = std::make_shared<ExchangeMap>();
		this->exchanges->new_exchange(
			exchange_id_complex,
			exchange_complex_path,
			Frequency::Min1,
			"%Y-%m-%d"
		);
		this->exchanges->restore_exchange(
			exchange_id_complex,
			asset_ids,
			std::nullopt
		);
	}
};

class BetaHedgeStrategy : public AgisStrategy {
public:
	BetaHedgeStrategy(PortfolioPtr const portfolio_)
		: AgisStrategy("test", portfolio_, 1) {}

	void next() {
		auto view = this->exchange->get_exchange_view("CLOSE");
		view.beta_scale();
		view.beta_hedge(1.25);
		this->strategy_allocate(view, 0, true, std::nullopt, AllocType::PCT);

	};
	void reset() override {}
	void build() override {
		this->exchange_subscribe(exchange_id_complex);
		this->exchange = this->get_exchange();
	}

	ExchangePtr exchange;

};

TEST_F(SimpleExchangeFixture, TestIndex) {
	exchanges->__build();

	auto _asset = exchanges->get_asset(asset_id_2).unwrap();
	auto _asset1 = exchanges->get_asset(asset_id_1).unwrap();

	//EXPECT_EQ(_asset->__is_aligned, true);
	//EXPECT_EQ(_asset->__is_streaming, true);
	//EXPECT_EQ(_asset1->__is_aligned, false);
	//EXPECT_EQ(_asset1->__is_streaming, false);

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


TEST_F(SimpleExchangeFixture, TestExchangeRemove) {
	exchanges->__build();
	
	EXPECT_EQ(exchanges->asset_exists(asset_id_2), true);
	EXPECT_EQ(exchanges->get_asset_ids(exchange_id_1).size(), 3);
	exchanges->remove_asset(asset_id_2);
	EXPECT_EQ(exchanges->asset_exists(asset_id_2), false);
	EXPECT_EQ(exchanges->get_asset_ids(exchange_id_1).size(), 2);

	auto exchange = exchanges->get_exchange(exchange_id_1);
	EXPECT_EQ(exchange->asset_exists(asset_id_2), false);
	EXPECT_EQ(exchange->get_asset_ids().size(), 2);

	exchanges->__goto(t4);

	EXPECT_EQ(exchanges->get_datetime(), t4);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, false), 105);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_1, true), 106);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_3, false), 101);
	EXPECT_EQ(exchanges->__get_market_price(asset_id_3, true), 102.4);
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
	EXPECT_EQ(view[0].asset_index, index);
	EXPECT_EQ(view[0].allocation_amount,101.4);

	exchange_view = exchange->get_exchange_view("CLOSE", 0, ExchangeQueryType::NSmallest, 1);
	view = exchange_view.view;
	index = exchanges->get_asset_index(asset_id_2);
	EXPECT_EQ(view.size(), 1);
	EXPECT_EQ(view[0].asset_index, index);
	EXPECT_EQ(view[0].allocation_amount, 99);
}

TEST_F(SimpleExchangeFixture, TestExchangeViewLambda) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_1);

	exchanges->step();
	exchanges->step();
	exchanges->step();
	
	auto daily_return = [](const AssetPtr& asset) -> AgisResult<double> {
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
	EXPECT_EQ(view[0].allocation_amount, (103.0/101));
	EXPECT_EQ(view[1].asset_index, index2);
	EXPECT_EQ(view[1].allocation_amount, (97.0 / 99));
}


TEST_F(ComplexExchangeFixture, TestExchangeHDF5Load) {
	exchanges->__build();
	
	auto dt_index = exchanges->__get_dt_index();
	EXPECT_EQ(dt_index[0], 1287360000000000000);
	EXPECT_EQ(dt_index[dt_index.size()-1], 1692921600000000000);
	
	auto first_open = 1692576000000000000LL;
	exchanges->__goto(first_open);
	std::string msft = "MSFT"; std::string orcl = "ORCL";
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(msft, false), 317.929993);
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(msft, true), 321.880005);
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(orcl, false), 116.900002);
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(orcl, true), 116.589996);

	exchanges->step();
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(msft, false), 325.500000);
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(msft, true), 322.459991);
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(orcl, false), 117.510002);
	EXPECT_FLOAT_EQ(exchanges->__get_market_price(orcl, true), 116.540001);
}


TEST_F(ComplexExchangeFixture, TestExchangeBeta) {
	exchanges->__build();
	auto market_time = exchanges->__get_market_time();
	EXPECT_EQ(market_time, 1287360000000000000LL);
	auto exchange = exchanges->get_exchange(exchange_id_complex);
	EXPECT_TRUE(exchange->asset_exists("SPY"));
	auto SPY = exchanges->set_market_asset(exchange_id_complex,"SPY", true, 252);
	EXPECT_FALSE(SPY.is_exception());

	exchanges->__build();

	market_time = exchanges->__get_market_time();
	EXPECT_EQ(market_time, 1318809600000000000LL);

	exchanges->step();

	auto msft = exchanges->get_asset("MSFT").unwrap();
	auto orcl = exchanges->get_asset("ORCL").unwrap();

	auto msft_beta = msft->get_beta();
	auto orcle_beta = orcl->get_beta();
	std::string msft_string = "MSFT"; std::string orcl_string = "ORCL";
	auto msft_price = exchanges->__get_market_price(msft_string, true);
	EXPECT_FLOAT_EQ(msft_price, 26.980000);
	EXPECT_FALSE(msft_beta.is_exception());
	EXPECT_FALSE(msft_beta.is_exception());
	EXPECT_FLOAT_EQ(msft_beta.unwrap(), 0.8677395535794543);
	EXPECT_FLOAT_EQ(orcle_beta.unwrap(), 1.2332540646896322);

	exchanges->step();
	msft_beta = msft->get_beta();
	orcle_beta = orcl->get_beta();
	EXPECT_FALSE(msft_beta.is_exception());
	EXPECT_FALSE(msft_beta.is_exception());
	EXPECT_FLOAT_EQ(msft_beta.unwrap(), 0.8604228929228622);
	EXPECT_FLOAT_EQ(orcle_beta.unwrap(), 1.2356849380454107);
}


TEST_F(ComplexExchangeFixture, TestBetaHedgeSimple) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_complex);
	auto SPY = exchanges->set_market_asset(exchange_id_complex, "SPY", true, 252);
	exchanges->__build();

	exchanges->step();
	auto view = exchange->get_exchange_view("Close");
	std::string msft_string = "MSFT"; std::string orcl_string = "ORCL";
	auto msft_price = exchanges->__get_market_price(msft_string, true);
	EXPECT_FLOAT_EQ(msft_price, 26.980000);
	auto orcle_price = exchanges->__get_market_price(orcl_string, true);
	EXPECT_FLOAT_EQ(orcle_price, 31.230000);
	view.uniform_weights(1.0);

	auto msft_id = exchanges->get_asset_index(msft_string);
	auto orcl_id = exchanges->get_asset_index(orcl_string);
	auto& msft_pair = view.get_allocation_by_asset_index(msft_id);
	auto& orcl_pair = view.get_allocation_by_asset_index(orcl_id);
	msft_pair.allocation_amount = 0.75;
	view.beta_scale();

	EXPECT_FLOAT_EQ(msft_pair.allocation_amount, 0.8508734914016446);
	EXPECT_FLOAT_EQ(orcl_pair.allocation_amount, 0.3991265085983553);

	view.beta_hedge(1.25);

	EXPECT_FLOAT_EQ(view.sum_weights(true, true), 1.25);
	EXPECT_FLOAT_EQ(view.net_beta().unwrap(), 0.0f);
}


TEST_F(ComplexExchangeFixture, TestSetVolatility) {
	exchanges->__build();
	auto exchange = exchanges->get_exchange(exchange_id_complex);
	exchange->__set_volatility_lookback(252);
	
	auto msft = exchanges->get_asset("MSFT").unwrap();
	auto orcl = exchanges->get_asset("ORCL").unwrap();
	EXPECT_TRUE(msft->get_volatility().is_exception());
	EXPECT_TRUE(msft->get_volatility().is_exception());

	exchanges->__build();
	exchanges->step();
	auto msft_vol = msft->get_volatility().unwrap();
	auto orcl_vol = orcl->get_volatility().unwrap();

	EXPECT_TRUE(msft_vol - 0.22797750690571786 < 1e-6);
	EXPECT_TRUE(orcl_vol - 0.30986218782515706 < 1e-6);
}