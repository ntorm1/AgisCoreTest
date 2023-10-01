#include "pch.h"
#include "Hydra.h"

#include <pybind11/embed.h>  // Include this for the Python embedding features
#include <pybind11/stl.h>     // Include this for Python list conversion

namespace py = pybind11;

namespace PyStrategyTest {

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

using namespace PyStrategyTest;


class PyTestFixture : public ::testing::Test {
protected:
	std::unique_ptr<Hydra> hydra;

	void SetUp() override {
		this->hydra = std::make_unique<Hydra>(0);
		hydra->new_portfolio(
			portfolio_id_1_h,
			cash
		);
		this->hydra->new_exchange(
			exchange_id_1_h,
			exchange1_path_h,
			Frequency::Day1,
			"%Y-%m-%d"
		);

	}
};

TEST_F(PyTestFixture, TestPyStrategyBuild)
{
	py::scoped_interpreter python;

	// Get the sys, os module
	py::module sys = py::module::import("sys");

	// Get the current path list
	py::list pathList = sys.attr("path");
	
#ifdef _DEBUG
	std::string pyd_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\x64\\Debug";
#else
	std::string pyd_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\x64\\Release";
#endif // DEBUG

	std::string strategy_path = "C:\\Users\\natha\\OneDrive\\Desktop\\C++\\Nexus\\AgisCoreTest";
	// Add your custom paths to the beginning of the list
	pathList.insert(0, py::str(strategy_path));
	pathList.insert(0, py::str(pyd_path));

	// Update the sys.path with the modified path list
	sys.attr("path") = pathList;

	// load in Agis Core
	py::module my_module = py::module::import("AgisCorePy");

	auto portfolio = this->hydra->get_portfolio(portfolio_id_1_h);

	// Access and create an instance of the Python-derived class
	auto _obj = py::module::import("test_py_strategy").attr("DummyStrategy")(
		"test_strategy",
		portfolio,
		1.0f
		);

	// Create a std::unique_ptr by transferring ownership of the raw pointer
	std::unique_ptr<AgisStrategy> strategy_unique_ptr = std::unique_ptr<AgisStrategy>(py::cast<AgisStrategy*>(_obj.release()));
	strategy_unique_ptr->set_strategy_type(AgisStrategyType::PY);

	auto x = strategy_unique_ptr->get_strategy_id();
	EXPECT_EQ(x, "test_strategy");

	hydra->register_strategy(std::move(strategy_unique_ptr));
	hydra->build();
	hydra->__step();

	auto id1 = hydra->get_exchanges().get_asset_index(asset_id_1_h);
	auto id2 = hydra->get_exchanges().get_asset_index(asset_id_2_h);
	auto id3 = hydra->get_exchanges().get_asset_index(asset_id_3_h);

	auto trade_opt_1 = portfolio->get_trade(id1, "test_strategy");
	auto trade_opt_2 = portfolio->get_trade(id2, "test_strategy");
	auto trade_opt_3 = portfolio->get_trade(id3, "test_strategy");

	EXPECT_EQ(trade_opt_1.has_value(), false);
	EXPECT_EQ(trade_opt_2.has_value(), true);
	EXPECT_EQ(trade_opt_3.has_value(), true);
	auto& trade_2 = trade_opt_2.value().get();
	auto& trade_3 = trade_opt_3.value().get();
	EXPECT_EQ(trade_2->units, .5 * cash / 101.5);
	EXPECT_EQ(trade_3->units, .5 * cash / 101.5);
}