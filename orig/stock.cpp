#include "stock.h"

#include <cassert>
using namespace std;
// #define DEBUG

// Feel free to change everything here.

Order::Order(StockShare _item, shared_ptr<Account> _usr, double _price, OrderType _type) {
    item = _item, usr = _usr, order_type = _type, this->price = _price;
}

void Order::debug() {
    printf("---------------------------------------------\nOrder debug: ");
    if (order_type == Order::BUY)
        printf("OrderType: BUY, ");
    else
        printf("OrderType: SELL, ");
    printf("User ID: %d, Stock ID: %d, NumofShare: %d, Price: %.3lf\n", usr->get_id(), this->item.first->get_id(), this->item.second, price);
    printf("\n---------------------------------------------\n");
}

void Order::add(int num_share) { item.second += num_share; }
void Order::del(int num_share) { item.second -= num_share; }
int Order::get_shares() { return item.second; }
double Order::get_price() { return price; }
shared_ptr<Account> Order::get_usr() { return usr; }
shared_ptr<Stock> Order::get_stock() { return item.first; }

Stock::Stock(int _id, shared_ptr<Logs> _logger) {
    // TODO
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
    auto txn = make_shared<AddOrderTransaction>(buyorder, (log_id == -1) ? logger->get_id() : log_id);
    logger->push_back(static_pointer_cast<Transaction>(txn));
    buy_orders.insert(make_pair(-buyorder->get_price(), buyorder));
    check_freeze();
}
void Stock::add_sell_order(shared_ptr<Order> sellorder, int log_id) {
    // TODO
    auto txn = make_shared<AddOrderTransaction>(sellorder, (log_id == -1) ? logger->get_id() : log_id);
    logger->push_back(static_pointer_cast<Transaction>(txn));
    sell_orders.insert(make_pair(sellorder->get_price(), sellorder));
    check_freeze();
}
void Stock::del_buy_order(shared_ptr<Order> buyorder) {
    // TODO
    auto txn = make_shared<DelOrderTransaction>(buyorder, logger->get_id());
    logger->push_back(static_pointer_cast<Transaction>(txn));
    buy_orders.erase(buy_orders.find(make_pair(-buyorder->get_price(), buyorder)));
}
void Stock::del_sell_order(shared_ptr<Order> sellorder) {
    // TODO
    auto txn = make_shared<DelOrderTransaction>(sellorder, logger->get_id());
    logger->push_back(static_pointer_cast<Transaction>(txn));
    sell_orders.erase(sell_orders.find(make_pair(sellorder->get_price(), sellorder)));
}

OrderResult Stock::buy(shared_ptr<Account> usr, int num_share, double price) {
#ifdef DEBUG
    cout << "in stock::buy, usr " << usr->get_id() << " want to buy stock " << stock_id << endl;
#endif  // DEBUG
    shared_ptr<Order> result = nullptr;
    if (usr->get_fluid() < price * num_share || freeze) {
#ifdef DEBUG
        cout << "not enough money or freeze" << endl;
#endif  // DEBUG
        return make_pair(-1, result);
    }
    // TODO
    int buy_log_id = logger->get_id();
    int num_buy = 0;
#ifdef DEBUG
    if (sell_orders.empty()) cout << "sell orders empty!" << endl;
#endif  // DEBUG
    shared_ptr<Order> cheapest_sell_order;
    for (auto i : sell_orders) {
        #ifdef DEBUG
        cout << "this sell order: " << endl;
        i.second->debug();
        #endif // DEBUG
        if (num_buy >= num_share || (cheapest_sell_order = i.second)->get_price() > price) break;

        auto seller = cheapest_sell_order->get_usr();
        int all_buy = cheapest_sell_order->get_shares();
        int remaining_buy = num_share - num_buy;
        int available_buy = seller->get_share(stock_id);
        int this_buy = min(all_buy, min(remaining_buy, available_buy));

        cheapest_sell_order->del(this_buy);
#ifdef DEBUG
        cout << "remaining_buy" <<  remaining_buy << "all_buy" << all_buy << "available_buy" << available_buy << endl;
        cout << "\tbought " << this_buy << " stocks" << endl;
#endif  // DEBUG
        if (all_buy == this_buy) del_sell_order(cheapest_sell_order);
        auto txn = make_shared<BuyAndSellTransaction>(usr, seller, this_buy, cheapest_sell_order, buy_log_id);
        logger->push_back(static_pointer_cast<Transaction>(txn));
        num_buy += this_buy;
        usr->add_money(-cheapest_sell_order->get_price() * this_buy);
        usr->add_share(stock_id, +this_buy);
        seller->add_money(+cheapest_sell_order->get_price() * this_buy);
        seller->add_share(stock_id, -this_buy);
    }
    if (num_buy < num_share) {
        cout << "creating order for remainning buys" << endl;
        result = make_shared<Order>(make_pair(shared_from_this(), num_share - num_buy), usr, price, Order::BUY);
        usr->add_order(result);
        add_buy_order(result, buy_log_id);
    }
    check_freeze();
    return make_pair(num_buy, result);
}

OrderResult Stock::sell(shared_ptr<Account> usr, int num_share, double price) {
    shared_ptr<Order> result = nullptr;
    if (usr->get_share(stock_id) < num_share || freeze)
        return make_pair(-1, result);
    // TODO
    int sell_log_id = logger->get_id();
    int num_sell = 0;
    shared_ptr<Order> top_buy_order;
    for (auto i : buy_orders) {
        if (num_sell >= num_share || (top_buy_order = i.second)->get_price() < price) break;

        int all_sell = top_buy_order->get_shares();
        int remaining_sell = num_share - num_sell;
        // the top_buyer should only be able to use up his fluid propriety
        int available_sell = top_buy_order->get_usr()->get_fluid() / top_buy_order->get_price();
        int this_sell = min(all_sell, min(remaining_sell, available_sell));

        top_buy_order->del(this_sell);
        if (all_sell == this_sell) del_buy_order(top_buy_order);
        auto txn = make_shared<BuyAndSellTransaction>(top_buy_order->get_usr(), usr, this_sell, top_buy_order, sell_log_id);
        logger->push_back(static_pointer_cast<Transaction>(txn));
        num_sell += this_sell;
    }
    if (num_sell < num_share) {
        result = make_shared<Order>(make_pair(shared_from_this(), num_share - num_sell), usr, price, Order::SELL);
        usr->add_order(result);
        add_sell_order(result, sell_log_id);
    }
    check_freeze();
    return make_pair(num_sell, result);
}
void Stock::set_start_price() {
    // TODO
    st_price = (sell_orders.empty()) ? 0 : sell_orders.begin()->first;
}
bool Stock::check_freeze() {
    // TODO
    if (freeze) return 1;
    if (st_price == 0) return 0;
    double cr_price = (sell_orders.empty()) ? 0 : sell_orders.begin()->first;
    if (cr_price > 1.1 * st_price || cr_price < 0.9 * st_price) {
        auto txn = make_shared<FreezeTransaction>(shared_from_this(), logger->get_id());
        logger->push_back(txn);
        freeze = 1;
    }
    return freeze;
}
void Stock::reset_freeze() {
    // TODO
    freeze = 0;
}

Transaction::Transaction(int txn_id) {
    this->txn_id = txn_id;
}
BuyAndSellTransaction::BuyAndSellTransaction(shared_ptr<Account> _buyer, shared_ptr<Account> _seller, int _num, shared_ptr<Order> _order, int _id) : Transaction(_id) {
    this->buyer = _buyer, this->seller = _seller;
    this->num_share = _num, this->order = _order;
    this->txn_type = Transaction::BUY_AND_SELL;
}
void BuyAndSellTransaction::undo() {
    double price = order->get_price();
    order->add(num_share);
    buyer->add_money(num_share * price);
    buyer->add_share(order->get_stock()->get_id(), -num_share);
    seller->add_money(-num_share * price);
    seller->add_share(order->get_stock()->get_id(), num_share);
}
AddOrderTransaction::AddOrderTransaction(shared_ptr<Order> _order, int _id) : Transaction(_id) {
    this->order = _order;
    this->txn_type = Transaction::ADD_ORDER;
}
void AddOrderTransaction::undo() {
    // TODO
    auto usr = order->get_usr();
    auto stock = order->get_stock();
    if (order->order_type == Order::BUY) stock->del_buy_order(order);
    if (order->order_type == Order::SELL) stock->del_sell_order(order);
    usr->del_order(order);
}

DelOrderTransaction::DelOrderTransaction(shared_ptr<Order> _order, int _id) : Transaction(_id) {
    this->order = _order;
    this->txn_type = Transaction::DEL_ORDER;
}
void DelOrderTransaction::undo() {
    // TODO
    auto usr = order->get_usr();
    auto stock = order->get_stock();
    if (order->order_type == Order::BUY) stock->add_buy_order(order, txn_id);
    if (order->order_type == Order::SELL) stock->add_sell_order(order, txn_id);
    usr->add_order(order);
}
FreezeTransaction::FreezeTransaction(shared_ptr<Stock> _stock, int _id) : Transaction(_id) {
    this->stock = _stock;
    this->txn_type = Transaction::FREEZE;
}
void FreezeTransaction::undo() {
    // TODO
    stock->reset_freeze();
}

int Account::get_id() { return usr_id; }

Account::Account(int _id, double _remain) {
    // TODO
    usr_id = _id;
    remain = _remain;
}
void Account::add_order(shared_ptr<Order> order) {
    orders.insert(order);
}
void Account::del_order(shared_ptr<Order> order) {
    orders.erase(order);
}
int Account::get_share(int stock_id) {
    auto it = holds.find(stock_id);
    if (it == holds.end())
        return 0;
    else
        return it->second;
}
double Account::get_remain() { return remain; }
double Account::get_fluid() {
    // TODO
    double fluid = remain;
    for (auto order : orders) {
        if (order->order_type = Order::BUY) fluid -= order->get_price() * order->get_shares();
        // if (order->order_type = Order::SELL) fluid += order->get_price() * order->get_shares();
    }
    return fluid;
}
void Account::add_money(double x) { remain += x; }
void Account::add_share(int stock_id, int num) { holds[stock_id] += num; }
double Account::evaluate() {
    // TODO
    double real = remain;
    for (auto order : orders) {
        // if (order->order_type = Order::BUY) real -= order->get_price() * order->get_shares();
        if (order->order_type = Order::SELL) real += order->get_price() * order->get_shares();
    }
    return real;
}

void Logs::push_back(shared_ptr<Transaction> log) { contents.push_back(log); }
shared_ptr<Transaction> Logs::back() { return contents.back(); }
void Logs::pop_back() { contents.pop_back(); }
int Logs::get_id() { return idx++; }
void Logs::clear() {
    idx = 0;
    contents.clear();
}
int Logs::size() {
    return contents.size();
}

Market::Market() {
    // TODO
    logger = make_shared<Logs>();
    market_account = make_shared<Account>(0, 0);
}

void Market::new_day() {
    // TODO
    for (auto stock : stocks) {
        stock.second->reset_freeze();
    }
    logger->clear();
}

void Market::add_usr(int usr_id, double remain) {
    // TODO
    accounts[usr_id] = make_shared<Account>(usr_id, remain);
}

void Market::add_stock(int stock_id, int num_share, double price) {
    // TODO
    stocks[stock_id] = make_shared<Stock>(stock_id, logger);
    auto initial_sell_order = make_shared<Order>(make_pair(stocks[stock_id], num_share), market_account, price, Order::SELL);
    market_account->add_order(initial_sell_order);
    market_account->add_money(num_share * price);
    market_account->add_share(stock_id, num_share);
    stocks[stock_id]->add_sell_order(initial_sell_order);
}

bool Market::add_order(int usr_id, int stock_id, int num_share, double price, bool order_type)  // retrun success or not
{
    // TODO
    auto usr = accounts[usr_id];
    if (order_type == 0) {
        if (usr->get_fluid() < num_share * price) return false;
        auto buy_order = make_shared<Order>(make_pair(stocks[stock_id], num_share), accounts[usr_id], price, Order::BUY);
        stocks[stock_id]->add_buy_order(buy_order);
        accounts[usr_id]->add_order(buy_order);
        return true;
    } else {
        if (usr->get_share(stock_id) < num_share) return false;
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
    // TODO
    return stocks[stock_id]->buy(accounts[usr_id], num_share, price).first;
}

int Market::usr_sell(int usr_id, int stock_id, int num_share, double price) {
    // TODO
    return stocks[stock_id]->sell(accounts[usr_id], num_share, price).first;
}

pair<int, double> Market::best_stock() {
    // TODO
    double max_price = stocks.begin()->second->get_price();
    int max_id = -1;
    for (auto stock : stocks) {
        auto s = stock.second;
        if (s->get_price() > max_price) max_price = s->get_price(), max_id = s->get_id();
    }
    return make_pair(max_id, max_price);
}

pair<int, double> Market::worst_stock() {
    // TODO
    double worst_price = stocks.begin()->second->get_price();
    int worst_id = -1;
    for (auto stock : stocks) {
        auto s = stock.second;
        if (s->get_price() < worst_price) worst_price = s->get_price(), worst_id = s->get_id();
    }
    return make_pair(worst_id, worst_price);
}

void Market::undo() {
    int _id = logger->back()->txn_id;
    while (logger->size()) {
        auto txn = logger->back();
        if (txn->txn_id != _id)
            break;
        // TODO
        logger->pop_back();
        txn->undo();
    }
}

void Market::log() {
    for (auto log : logger->contents) {
        log->log();
    }
}