#include "pch.h"
#include "helpers.h"

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