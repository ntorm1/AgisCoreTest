#pragma once


import Asset;

using namespace Agis;

#include "Order.h"
#include "Exchange.h"
#include "Utils.h"

namespace AgisTest {

	inline std::shared_ptr<ExchangeMap> create_simple_exchange_map(
		std::string exchange_id,
		std::string exchange_path
	)
	{
		auto exchanges = std::make_shared<ExchangeMap>();
		exchanges->new_exchange(
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

