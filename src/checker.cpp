#include "stock.h"

bool Market::check() {
    bool res = 0;
#ifdef DEBUG
    res |= check_buy_order_smaller_than_sell_order();
    res |= check_ciruit_breaker();
    res |= check_usr_money_not_negative();
    res |= check_usr_does_not_sell_more_than_he_have();
    res |= check_buyer_is_not_seller();
#endif  // DEBUG
#ifdef DEBUG_MEMORY
    res |= check_account_unique();
#endif  // DEBUG_MEMORY
    if (res) {
        cout << "ERROR: running check failed" << endl;
        exit(0);
    }
    return res;
}

#ifdef DEBUG_MEMORY
bool Market::check_account_unique() {
    for (auto account : accounts) {
        if (account.second.use_count() != 2) {
            cout << "\taccount " << account.second->get_id() << " use count = " << account.second.use_count() << ", expected 2" << endl;
            return 1;
        }
    }
    return 0;
}
#endif  // DEBUG_MEMORY

#ifdef DEBUG
void Market::log() {
    for (auto log : logger->contents) {
        log->log();
    }
}

bool Market::check_buy_order_smaller_than_sell_order() {
    for (auto stock : stocks) {
        if (stock.second->buy_orders.empty()) continue;
        if (stock.second->sell_orders.empty()) continue;
        auto max_buy = stock.second->buy_orders.begin();
        auto min_sell = stock.second->sell_orders.begin();
        if (-max_buy->first >= min_sell->first) {
            cout << "[check_buy_order_smaller_than_sell_order]: stock_id: " << stock.second->get_id() << endl;
            cout << "the max buy order: " << endl;
            cout << "\t";
            max_buy->second->debug();
            cout << "the min sell order: " << endl;
            cout << "\t";
            min_sell->second->debug();
            return 1;
        }
    }
    return 0;
}

bool Market::check_ciruit_breaker() {
    for (auto stock : stocks) {
        if (!stock.second->is_freeze() && stock.second->check_freeze(-1)) {
            cout << "[market.check_ciruit_breaker]: " << endl;
            cout << "\tERROR" << endl;
            cout << "\tstock_id" << stock.second->get_id()
                 << "st_price: " << stock.second->st_price
                 << " cr_price: " << stock.second->get_price() << endl;
            return 1;
        }
    }
    return 0;
}

bool Market::check_usr_money_not_negative() {
    for (auto account : accounts) {
        if (account.second->get_remain() < 0) {
            cout << "[market.check_usr_money_not_negative]: usr_id: " << account.first << endl;
            return 1;
        }
    }
    return 0;
}

bool Market::check_usr_does_not_sell_more_than_he_have() {
    for (auto account : accounts) {
        unordered_map<int, int> selling;
        for (auto order : account.second->orders) {
            if (order->order_type == Order::SELL)
                selling[order->get_stock().lock()->get_id()] += order->get_shares();
        }
        for (auto hold : selling) {
            auto iter = account.second->holds.find(hold.first);
            if (iter == account.second->holds.end()) {
                cout << "[market.check]: selling never owned stock, usr id: " << account.second->get_id() << endl;
                return 1;
            }
            if (iter->second < hold.second) {
                cout << "[market.check]: selling more than owned, usr id: " << account.second->get_id() << endl;
                cout << "\towned: " << hold.second << " sold: " << iter->second << endl;
                return 1;
            }
        }
    }
    return 0;
}

bool Market::check_buyer_is_not_seller() {
    for (auto txn : logger->contents) {
        if (txn->txn_type != Transaction::BUY_AND_SELL) continue;
        auto bas_txn = dynamic_pointer_cast<BuyAndSellTransaction>(txn);
        if (bas_txn->seller.lock() == bas_txn->buyer.lock()) {
            cout << "[market.check_buyer_is_not_seller]: bas txn has identical buyer and seller" << endl;
            return 1;
        }
    }
    return 0;
}

#endif  // DEBUG