#include "pch.h"
#include <ranges>
#include <span>
#include "AgisRisk.h"
#include "Asset.h"
#include "Exchange.h"
#include "Portfolio.h"
#include "Utils.h"
#include "AgisStrategy.h"
#include <ankerl/unordered_dense.h>

namespace RiskTest {
	std::string exchange1_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\AgisCoreTest\\data\\exchange1";
	std::string exchange_complex_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest\\data\\SPY_DAILY\\data.h5";

	std::string exchange_id_complex = "SPY_DAILY";
}

using namespace RiskTest;

class RiskTests : public ::testing::Test {
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


TEST_F(RiskTests, TestIncrementalCov) {
	this->exchanges->__set_volatility_lookback(252);
	auto res = this->exchanges->init_covariance_matrix();
	EXPECT_FALSE(res.is_exception());
	this->exchanges->__build();

	auto& cov = this->exchanges->get_covariance_matrix();
	auto msft_id = this->exchanges->get_asset_index("MSFT");
	auto orcl_id = this->exchanges->get_asset_index("ORCL");
	auto msft_variance = cov(msft_id, msft_id);
	auto orcl_variance = cov(orcl_id, orcl_id);
	EXPECT_EQ(msft_variance, 0.0f);
	EXPECT_EQ(orcl_variance, 0.0f);

	this->exchanges->step();
	msft_variance = cov(msft_id, msft_id);
	orcl_variance = cov(orcl_id, orcl_id);
	auto cov_1 = cov(msft_id, orcl_id);
	auto cov_2 = cov(orcl_id, msft_id);
	EXPECT_DOUBLE_EQ(msft_variance, 0.0002062450145037565);
	EXPECT_DOUBLE_EQ(orcl_variance, 0.0003810102200150513);
	EXPECT_FLOAT_EQ(cov_1, 0.00018203125361158286);
	EXPECT_FLOAT_EQ(cov_2, 0.00018203125361158286);

	this->exchanges->step();
	msft_variance = cov(msft_id, msft_id);
	orcl_variance = cov(orcl_id, orcl_id);
	cov_1 = cov(msft_id, orcl_id);
	cov_2 = cov(orcl_id, msft_id);
	EXPECT_DOUBLE_EQ(msft_variance, 0.0002036289992996237);
	EXPECT_DOUBLE_EQ(orcl_variance, 0.00038259256400800535);
	EXPECT_FLOAT_EQ(cov_1, 0.00018255075523587025);
	EXPECT_FLOAT_EQ(cov_2, 0.00018255075523587025);

}