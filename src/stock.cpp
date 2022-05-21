#include "stock.h"

#include <cassert>
using namespace std;

// Feel free to change everything here.

Order::Order(StockShare _item, shared_ptr<Account> _usr, double _price, OrderType _type) {
    item = _item, usr = _usr, order_type = _type, this->price = _price;
}

void Order::add(int num_share) { item.second += num_share; }
void Order::del(int num_share) { item.second -= num_share; }
int Order::get_shares() { return item.second; }
double Order::get_price() { return price; }
shared_ptr<Account> Order::get_usr() { return usr; }
shared_ptr<Stock> Order::get_stock() { return item.first; }

#ifdef DEBUG
void Order::debug() {
    cout << "[Order.debug]" << endl;
    if (order_type == Order::BUY)
        cout << "\tOrderType: BUY, ";
    else
        cout << "\tOrderType: SELL, ";
    printf("\tUser ID: %d, Stock ID: %d, NumofShare: %d, Price: %.3lf\n", usr->get_id(), this->item.first->get_id(), this->item.second, price);
}
#endif  // DEBUG

Stock::Stock(int _id, shared_ptr<Logs> _logger) {
    stock_id = _id, this->logger = _logger;
}

int Stock::get_id() { return stock_id; }
bool Stock::is_freeze() { return freeze; }

double Stock::get_price() {
    if (sell_orders.empty()) {
        return 0;
    } else {
        return sell_orders.begin()->first;
    }
}

typedef set<pair<double, shared_ptr<Order>>>::iterator iter;

void Stock::add_buy_order(shared_ptr<Order> buyorder, int log_id) {
    buy_orders.insert(make_pair(-buyorder->get_price(), buyorder));
    if (log_id == -2) return;
    auto logger_strong = logger.lock();
    if (log_id == -1) check_freeze(log_id = logger_strong->get_id());
    auto txn = make_shared<AddOrderTransaction>(buyorder, log_id);
    logger_strong->push_back(static_pointer_cast<Transaction>(txn));
}
void Stock::add_sell_order(shared_ptr<Order> sellorder, int log_id) {
    sell_orders.insert(make_pair(sellorder->get_price(), sellorder));
    if (log_id == -2) return;
    auto logger_strong = logger.lock();
    if (log_id == -1) check_freeze(log_id = logger_strong->get_id());
    auto txn = make_shared<AddOrderTransaction>(sellorder, log_id);
    logger_strong->push_back(static_pointer_cast<Transaction>(txn));
}
void Stock::del_buy_order(shared_ptr<Order> buyorder, int log_id) {
    buy_orders.erase(buy_orders.find(make_pair(-buyorder->get_price(), buyorder)));
    if (log_id == -2) return;
    auto logger_strong = logger.lock();
    auto txn = make_shared<DelOrderTransaction>(buyorder, logger_strong->get_id());
    logger_strong->push_back(static_pointer_cast<Transaction>(txn));
}
void Stock::del_sell_order(shared_ptr<Order> sellorder, int log_id) {
    sell_orders.erase(sell_orders.find(make_pair(sellorder->get_price(), sellorder)));
    if (log_id == -2) return;
    auto logger_strong = logger.lock();
    auto txn = make_shared<DelOrderTransaction>(sellorder, logger_strong->get_id());
    logger_strong->push_back(static_pointer_cast<Transaction>(txn));
}

OrderResult Stock::buy(shared_ptr<Account> usr, int num_share, double price) {
#ifdef DEBUG
    cout << "[stock.buy]: usr " << usr->get_id() << " want to buy stock " << stock_id << endl;
    cout << "\tusr fluid: " << usr->get_fluid() << " price: " << price
         << " num_share: " << num_share << " stock price: " << get_price() << " freezed: " << freeze << endl;
#endif  // DEBUG
    shared_ptr<Order> result_buy_order = nullptr;
    if (usr->get_fluid() < price * num_share || freeze) {
#ifdef DEBUG
        cout << "\tnot enough money or stock is freezed, Aborting" << endl;
#endif  // DEBUG
        return make_pair(-1, result_buy_order);
    }

#ifdef DEBUG
    cout << "\titerating all sell orders" << endl;
    for (auto i : sell_orders) {
        cout << "\t";
        i.second->debug();
    }
    cout << endl;
#endif  // DEBUG

    auto logger_strong = logger.lock();
    int buy_log_id = logger_strong->get_id();
    int num_bought = 0;
    shared_ptr<Order> cheapest_sell_order;
    for (auto i : sell_orders) {
        if (num_bought >= num_share || (cheapest_sell_order = i.second)->get_price() > price) break;
        auto seller = cheapest_sell_order->get_usr();

        int all_buy = cheapest_sell_order->get_shares();
        int need_buy = num_share - num_bought;
        int available_buy = seller->get_share(stock_id);
        int actual_bought = min(all_buy, min(need_buy, available_buy));

        if (actual_bought == 0) continue;
        cheapest_sell_order->del(actual_bought);
        if (actual_bought == all_buy) {
            del_sell_order(cheapest_sell_order);
            seller->del_order(cheapest_sell_order);
            auto txn = make_shared<DelOrderTransaction>(cheapest_sell_order, buy_log_id);
            logger_strong->push_back(static_pointer_cast<Transaction>(txn));
        }
        auto txn = make_shared<BuyAndSellTransaction>(usr, seller, actual_bought, cheapest_sell_order, buy_log_id);
        logger_strong->push_back(static_pointer_cast<Transaction>(txn));
        num_bought += actual_bought;
        usr->add_money(-cheapest_sell_order->get_price() * actual_bought);
        usr->add_share(stock_id, +actual_bought);
        seller->add_money(+cheapest_sell_order->get_price() * actual_bought);
        seller->add_share(stock_id, -actual_bought);
    }
    if (num_bought < num_share) {
        result_buy_order = make_shared<Order>(make_pair(shared_from_this(), num_share - num_bought), usr, price, Order::BUY);
        usr->add_order(result_buy_order);
        add_buy_order(result_buy_order, buy_log_id);
#ifdef DEBUG
        cout << "\tcreating order for remaining buys" << endl;
        cout << "\tnow the buy order list is: " << endl;
        for (auto i : buy_orders) {
            cout << "\t";
            i.second->debug();
        }
        cout << endl;
#endif  // DEBUG
    }
    check_freeze(buy_log_id);
    return make_pair(num_bought, result_buy_order);
}

OrderResult Stock::sell(shared_ptr<Account> usr, int num_share, double price) {
#ifdef DEBUG
    cout << "[stock.sell]: usr " << usr->get_id() << " want to sell stock " << stock_id << endl;
    cout << "\tusr net share: " << usr->get_net_share(stock_id)
         << " num_share: " << num_share << " freezed: " << freeze << endl;
#endif  // DEBUG
    shared_ptr<Order> result_sell_order = nullptr;
    if (usr->get_net_share(stock_id) < num_share || freeze) {
#ifdef DEBUG
        cout << "\tnot enough shares to sell or stock is freezed" << endl;
#endif  // DEBUG
        return make_pair(-1, result_sell_order);
    }

#ifdef DEBUG
    cout << "\titerating all buy orders" << endl;
    for (auto i : buy_orders) {
        cout << "\t";
        i.second->debug();
    }
    cout << endl;
#endif  // DEBUG

    auto logger_strong = logger.lock();
    int sell_log_id = logger_strong->get_id();
    int num_sold = 0;
    shared_ptr<Order> top_buy_order;
    for (auto i : buy_orders) {
        if (num_sold >= num_share || (top_buy_order = i.second)->get_price() < price) break;
        auto buyer = top_buy_order->get_usr();

        int all_sell = top_buy_order->get_shares();
        int need_sell = num_share - num_sold;
        // the top_buyer should only be able to use up his fluid propriety
        int available_sell = buyer->get_fluid() / top_buy_order->get_price();
        int actual_sold = min(all_sell, min(need_sell, available_sell));

        if (actual_sold == 0) continue;
        top_buy_order->del(actual_sold);
        if (all_sell == actual_sold) {
            del_buy_order(top_buy_order);
            buyer->del_order(top_buy_order);
            auto txn = make_shared<DelOrderTransaction>(top_buy_order, sell_log_id);
            logger_strong->push_back(static_pointer_cast<Transaction>(txn));
        }
        auto txn = make_shared<BuyAndSellTransaction>(top_buy_order->get_usr(), usr, actual_sold, top_buy_order, sell_log_id);
        logger_strong->push_back(static_pointer_cast<Transaction>(txn));
        num_sold += actual_sold;
        usr->add_money(+top_buy_order->get_price() * actual_sold);
        usr->add_share(stock_id, -actual_sold);
        buyer->add_money(-top_buy_order->get_price() * actual_sold);
        buyer->add_share(stock_id, +actual_sold);
    }
    if (num_sold < num_share) {
        result_sell_order = make_shared<Order>(make_pair(shared_from_this(), num_share - num_sold), usr, price, Order::SELL);
        usr->add_order(result_sell_order);
        add_sell_order(result_sell_order, sell_log_id);
#ifdef DEBUG
        cout << "\tcreating order for remaining sells" << endl;
        cout << "\tnow the sell order list is: " << endl;
        for (auto i : sell_orders) {
            cout << "\t";
            i.second->debug();
        }
        cout << endl;
#endif  // DEBUG
    }
    check_freeze(sell_log_id);
    return make_pair(num_sold, result_sell_order);
}
void Stock::init_start_price() {
    st_price = (sell_orders.empty()) ? 0 : sell_orders.begin()->first;
}
bool Stock::check_freeze(int log_id) {
    if (freeze) return 1;
    if (st_price == 0) return 0;
    double cr_price = (sell_orders.empty()) ? 0 : sell_orders.begin()->first;
    if (cr_price > 1.1 * st_price || cr_price < 0.9 * st_price) {
        freeze = 1;
        if (log_id == -1) return 1;
        auto logger_strong = logger.lock();
        auto txn = make_shared<FreezeTransaction>(shared_from_this(), log_id);
        logger_strong->push_back(static_pointer_cast<Transaction>(txn));
        cout << "\tis now freezed" << endl;
    }
    return freeze;
}
void Stock::reset_freeze() {
    freeze = 0;
}

int Account::get_id() { return usr_id; }

Account::Account(int _id, double _remain) { usr_id = _id, remain = _remain; }

void Account::add_order(shared_ptr<Order> order) { orders.insert(order); }
void Account::del_order(shared_ptr<Order> order) { orders.erase(order); }

int Account::get_share(int stock_id) {
    auto it = holds.find(stock_id);
    if (it == holds.end())
        return 0;
    return it->second;
}
int Account::get_net_share(int stock_id) {
    int total = get_share(stock_id);
    for (auto order : orders) {
        if (order->get_stock()->get_id() == stock_id && order->order_type == Order::SELL) {
            total -= order->get_shares();
        }
    }
    return total;
}
double Account::get_remain() { return remain; }
double Account::get_fluid() {
    double fluid = remain;
    for (auto order : orders) {
        if (order->order_type == Order::BUY) fluid -= order->get_price() * order->get_shares();
        // if (order->order_type == Order::SELL) fluid += order->get_price() * order->get_shares();
    }
    return fluid;
}
double Account::evaluate() {
    double value = remain;
    for (auto order : orders) {
        // if (order->order_type == Order::BUY) real -= order->get_price() * order->get_shares();
        if (order->order_type == Order::SELL) value += order->get_price() * order->get_shares();
    }
    return value;
}

void Account::add_money(double x) { remain += x; }
void Account::add_share(int stock_id, int num) { holds[stock_id] += num; }

Transaction::Transaction(int txn_id) { this->txn_id = txn_id; }

BuyAndSellTransaction::BuyAndSellTransaction(shared_ptr<Account> _buyer, shared_ptr<Account> _seller, int _num, shared_ptr<Order> _order, int _id) : Transaction(_id) {
    this->buyer = _buyer, this->seller = _seller;
    this->num_share = _num, this->order = _order;
    this->txn_type = Transaction::BUY_AND_SELL;
}
void BuyAndSellTransaction::undo() {
#ifdef DEBUG
    cout << "\tundo buy and sell" << endl;
#endif  // DEBUG
    double price = order->get_price();
    auto buyer_strong = buyer.lock();
    auto seller_strong = buyer.lock();
    order->add(num_share);
    buyer_strong->add_money(num_share * price);
    buyer_strong->add_share(order->get_stock()->get_id(), -num_share);
    seller_strong->add_money(-num_share * price);
    seller_strong->add_share(order->get_stock()->get_id(), num_share);
}

AddOrderTransaction::AddOrderTransaction(shared_ptr<Order> _order, int _id) : Transaction(_id) {
    this->order = _order;
    this->txn_type = Transaction::ADD_ORDER;
}
void AddOrderTransaction::undo() {
#ifdef DEBUG
    cout << "\tundo add order" << endl;
#endif  // DEBUG
    auto usr = order->get_usr();
    auto stock = order->get_stock();
    if (order->order_type == Order::BUY) stock->del_buy_order(order, -2);
    if (order->order_type == Order::SELL) stock->del_sell_order(order, -2);
    usr->del_order(order);
}

DelOrderTransaction::DelOrderTransaction(shared_ptr<Order> _order, int _id) : Transaction(_id) {
#ifdef DEBUG
    cout << "\tundo del order" << endl;
#endif  // DEBUG
    this->order = _order;
    this->txn_type = Transaction::DEL_ORDER;
}
void DelOrderTransaction::undo() {
    auto usr = order->get_usr();
    auto stock = order->get_stock();
    if (order->order_type == Order::BUY) stock->add_buy_order(order, -2);
    if (order->order_type == Order::SELL) stock->add_sell_order(order, -2);
    usr->add_order(order);
}
FreezeTransaction::FreezeTransaction(shared_ptr<Stock> _stock, int _id) : Transaction(_id) {
    this->stock = _stock;
    this->txn_type = Transaction::FREEZE;
}
void FreezeTransaction::undo() {
#ifdef DEBUG
    cout << "\tundo freeze" << endl;
#endif  // DEBUG
    auto stock_strong = stock.lock();
    stock_strong->reset_freeze();
}

void Logs::push_back(shared_ptr<Transaction> log) { contents.push_back(log); }
shared_ptr<Transaction> Logs::back() { return contents.back(); }
void Logs::pop_back() { contents.pop_back(); }
int Logs::get_id() { return idx++; }
void Logs::clear() {
    idx = 0;
    contents.clear();
}
int Logs::size() { return contents.size(); }

Market::Market() {
    logger = make_shared<Logs>();
    market_account = make_shared<Account>(0, 0);
}

Market::~Market() {
    // logger.reset();
    // market_account.reset();
    // for (auto account : accounts) {
    //     account.second.reset();
    // }
    // for (auto stock : stocks) {
    //     stock.second.reset();
    // }
}

void Market::new_day() {
#ifdef DEBUG
    cout << "[Market.new_day]: new day!" << endl;
#endif  // DEBUG
    for (auto stock : stocks) {
        stock.second->init_start_price();
        stock.second->reset_freeze();
    }
    logger->clear();
}

void Market::add_usr(int usr_id, double remain) {
    accounts[usr_id] = make_shared<Account>(usr_id, remain);
}

void Market::add_stock(int stock_id, int num_share, double price) {
    stocks[stock_id] = make_shared<Stock>(stock_id, logger);
    auto initial_sell_order = make_shared<Order>(make_pair(stocks[stock_id], num_share), market_account, price, Order::SELL);
    market_account->add_order(initial_sell_order);
    market_account->add_money(num_share * price);
    market_account->add_share(stock_id, num_share);
    stocks[stock_id]->add_sell_order(initial_sell_order);
}

bool Market::add_order(int usr_id, int stock_id, int num_share, double price, bool order_type) {
    auto usr = accounts[usr_id];
    if (order_type == 0) {
        if (usr->get_fluid() < num_share * price) return false;
        auto buy_order = make_shared<Order>(make_pair(stocks[stock_id], num_share), accounts[usr_id], price, Order::BUY);
        stocks[stock_id]->add_buy_order(buy_order);
        accounts[usr_id]->add_order(buy_order);
        return true;
    } else {
        if (usr->get_net_share(stock_id) < num_share) return false;
        auto sell_order = make_shared<Order>(make_pair(stocks[stock_id], num_share), accounts[usr_id], price, Order::SELL);
        stocks[stock_id]->add_sell_order(sell_order);
        accounts[usr_id]->add_order(sell_order);
        return true;
    }
}

double Market::evaluate(int usr_id) {
    return accounts[usr_id]->evaluate();
}

int Market::usr_buy(int usr_id, int stock_id, int num_share, double price) {
    return stocks[stock_id]->buy(accounts[usr_id], num_share, price).first;
}

int Market::usr_sell(int usr_id, int stock_id, int num_share, double price) {
    return stocks[stock_id]->sell(accounts[usr_id], num_share, price).first;
}

pair<int, double> Market::best_stock() {
    double max_price = stocks.begin()->second->get_price();
    int max_id = stocks.begin()->second->get_id();
    for (auto stock : stocks) {
        auto s = stock.second;
        if (s->get_price() > max_price) max_price = s->get_price(), max_id = s->get_id();
    }
    return make_pair(max_id, max_price);
}

pair<int, double> Market::worst_stock() {
    double worst_price = stocks.begin()->second->get_price();
    int worst_id = stocks.begin()->second->get_id();
    for (auto stock : stocks) {
        auto s = stock.second;
        if (s->get_price() < worst_price) worst_price = s->get_price(), worst_id = s->get_id();
    }
    return make_pair(worst_id, worst_price);
}

void Market::undo() {
    if (logger->size() == 0) {
        cout << "cannot undo anymore!" << endl;
        exit(0);
    }
#ifdef DEBUG
    cout << "[Market.undo()]" << endl;
#endif  // DEBUG
    int _id = logger->back()->txn_id;
    while (logger->size()) {
        auto txn = logger->back();
        if (txn->txn_id != _id)
            break;
        logger->pop_back();
        txn->undo();
    }
}

#ifdef DEBUG
void Market::log() {
    for (auto log : logger->contents) {
        log->log();
    }
}

bool Market::check() {
    bool res = 0;
    res |= check_buy_order_smaller_than_sell_order();
    res |= check_ciruit_breaker();
    res |= check_usr_money_not_negative();
    res |= check_usr_does_not_sell_more_than_he_have();
    if (res) exit(0);
    return res;
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
    cout << "[check_ciruit_breaker]: " << endl;
    for (auto stock : stocks) {
        if (!stock.second->is_freeze() && stock.second->check_freeze(-1)) {
            cout << "\tERROR" << endl;
            cout << "\tstock_id" << stock.second->get_id()
                 << "st_price: " << stock.second->st_price
                 << " cr_price: " << stock.second->get_price() << endl;
            return 1;
        }
    }
    cout << "\tPASS" << endl;
    return 0;
}

bool Market::check_usr_money_not_negative() {
    for (auto account : accounts) {
        if (account.second->get_remain() < 0) {
            cout << "check_usr_money_not_negative]: usr_id: " << account.first << endl;
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
                selling[order->get_stock()->get_id()] += order->get_shares();
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

#endif  // DEBUG