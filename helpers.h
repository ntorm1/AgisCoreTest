#pragma once

#define NS_CONST (long long) 1e9

#include "Asset/Asset.h"

using namespace Agis;

#include "Order.h"
#include "Hydra.h"
#include "Portfolio.h"
#include "ExchangeMap.h"
#include "Utils.h"

#define CHECK_EXCEPT(res) \
    do { \
        if (!res.has_value()) { \
            std::cout << res.error().what() << std::endl; \
        } \
    } while (false)

namespace AgisTest {

	inline std::shared_ptr<ExchangeMap> create_simple_exchange_map(
		std::string exchange_id,
		std::string exchange_path
	)
	{
		auto exchanges = std::make_shared<ExchangeMap>();
		exchanges->new_exchange(
			AssetType::US_EQUITY,
			exchange_id,
			exchange_path,
			Frequency::Day1,
			"%Y-%m-%d"
		);
		exchanges->restore_exchange(exchange_id);
		return exchanges;
	}

	inline OrderPtr create_order(
		size_t asset_index,
		double units,
		OrderType type = OrderType::MARKET_ORDER,
		size_t strategy_index = 0,
		size_t portfolio_index = 0,
		size_t broker_index = 0
	)
	{
		return std::make_unique<Order>(
			type,
			asset_index,
			units,
			strategy_index,
			portfolio_index,
			broker_index
		);
	}
}

