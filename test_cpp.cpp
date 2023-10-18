#pragma once
#include "pch.h"
#include "helpers.h"

#include "Exchange.h"
#include "AgisStrategy.h"
#include "Broker/Broker.Base.h"

using namespace Agis;

class ExampleStrategy : public AgisStrategy {
public:
    ExampleStrategy(
        PortfolioPtr const portfolio_,
        BrokerPtr broker_
    ) : AgisStrategy("CL_TREND", portfolio_, broker_, 1.0f) {}

    void next() override {
        // get the front month contract
        auto front_month_opt = this->_table->front_month();
        if (!front_month_opt.has_value()) return;
        auto& front_month = front_month_opt.value();

        // allocate the strategy to go long if the front month is up 1% over the previous close
        auto prev_close = front_month->get_asset_feature("close", -1).value();
        auto close = front_month->get_asset_feature("close",0).value();
        bool factor = (close - prev_close) / prev_close > 0.01f;
        this->_allocation.view = { {front_month->get_asset_index(), factor * 1.0f} };
        this->strategy_allocate(
            _allocation,        // alocation of the strategy
            0.0f,               // minimum epsilon to trigger a new order
            true,               // whether to clear trades not in the allocation
            std::nullopt,       // no exit strategy
            AllocType::PCT      // allocation type is percentage
        );
    }

    void build() override {
        this->exchange_subscribe("exchange1");  // subsribe to the example exchange
        auto exchange = this->get_exchange();   // pull out the futures table
        this->_table = exchange->get_asset_table<FutureTable>(_contract_id).value(); 
        this->warmup = 1;                       // set the warmup needed for strategy
        this->_allocation.vol_target(0.10f);    // set target vol to 10%
    }

    void reset() override {}

private:
    std::string _contract_id = "CL";
    FutureTablePtr _table;
    ExchangeView _allocation;
};
