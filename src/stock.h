#ifndef TAX_FREE_STOCK_MARKET
#define TAX_FREE_STOCK_MARKET
// #include <algorithm>
#include <iostream>
// #include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;

#define DEBUG

class Stock;
class Account;
class Transaction;
class Logs;

// const double inf = 1e10;
// const double float_eps = 1e-7;

/**
 * @brief   some amount of a stock
 */
typedef pair<shared_ptr<Stock>, int> StockShare;

/**
 * @brief   item(i.e. stock and num shares), usr, price, type
 *
 */
class Order {
   public:
    typedef enum {
        BUY,
        SELL
    } OrderType;

   protected:
    double price;
    StockShare item;
    shared_ptr<Account> usr;

   public:
    Order(StockShare _item, shared_ptr<Account> _usr, double _price, OrderType _type);
    OrderType order_type;
    void add(int num_share);
    void del(int num_share);
    int get_shares();
    double get_price();
    shared_ptr<Account> get_usr();
    shared_ptr<Stock> get_stock();
#ifdef DEBUG
    void debug();
#endif  // DEBUG
};

typedef vector<shared_ptr<Order>> OrderList;
typedef pair<int, shared_ptr<Order>> OrderResult;

class Stock : public enable_shared_from_this<Stock> {
    set<pair<double, shared_ptr<Order>>> buy_orders, sell_orders;
    weak_ptr<Logs> logger;
    bool freeze;
    int stock_id;
    double st_price;
    Stock() = delete;
#ifdef DEBUG
    friend class Market;
#endif  // DEBUG

   public:
    Stock(int _id, shared_ptr<Logs> _logger);

    /**
     *@brief            may perform BuyAndSellTransaction and/or AddOrderTransaction
     *
     * @param usr       buyer
     * @param num_share expected number of share
     * @param price     maximum price to be tolerented
     *
     * @attention       always assigned a new log_id
     */
    OrderResult buy(shared_ptr<Account> usr, int num_share, double price);

    /**
     *@brief            may perform BuyAndSellTransaction and/or AddOrderTransaction
     *
     * @param usr       seller
     * @param num_share expected number of share
     * @param price     minimum price to be tolerented
     *
     * @attention       always assigned a new log_id
     */
    OrderResult sell(shared_ptr<Account> usr, int num_share, double price);

    /**
     * @brief           create an AddOrderTransaction, or not, if specify don't create transaction
     *
     * @param buyorder  buy order
     * @param log_id    default -1, specify the log_id of this AddOrderTransaction, if -1, then allocate a new id, if -2 then this acton will not be logged
     */
    void add_buy_order(shared_ptr<Order> buyorder, int log_id = -1);

    /**
     * @brief           create an AddOrderTransaction, or not, if specify don't create transaction
     *
     * @param sellorder sell order
     * @param log_id    default -1, specify the log_id of this AddOrderTransaction, if -1, then allocate a new id, if -2 then this acton will not be logged
     */
    void add_sell_order(shared_ptr<Order> sellorder, int log_id = -1);

    /**
     * @brief           create a new DeleteOrderTransaction log
     *
     * @param buyorder  the shared pointer of the order to delete
     * @param log_id    default 0, if -2 then this action will not be logged
     */
    void del_buy_order(shared_ptr<Order> buyorder, int logg_id = 0);

    /**
     * @brief           create a new DeleteOrderTransaction log
     *
     * @param sellorder the shared pointer of the order to delete
     * @param log_id    default 0, if -2 then this action will not be logged
     */
    void del_sell_order(shared_ptr<Order> sellorder, int logg_id = 0);
    void add_orders(OrderList orders);
    int get_id();
    double get_price();
    void init_start_price();
    bool is_freeze();

    /**
     * @brief   check condition and return whether freeze, may create a FreezeTransaction log
     *
     */
    bool check_freeze();
    void reset_freeze();
};

class Account {
    int usr_id;
    double remain;
    unordered_map<int, int> holds;  // (stock_id, num_share)
    unordered_set<shared_ptr<Order>> orders;

   public:
    Account() = delete;
    Account(int _id, double _remain);
    int get_id();
    void add_order(shared_ptr<Order> order);
    void del_order(shared_ptr<Order> order);
    double get_fluid();
    double evaluate();
    double get_remain();
    void add_money(double x);
    void add_share(int stock_id, int num);
    int get_share(int stock_id);
};

class Transaction {
   public:
    typedef enum {
        BUY_AND_SELL,
        ADD_ORDER,
        DEL_ORDER,
        FREEZE,
    } TxnType;
    int txn_id;
    TxnType txn_type;
    Transaction() = delete;
    Transaction(int txn_id);
    virtual void undo() = 0;
    virtual void log() = 0;
};

class BuyAndSellTransaction : public Transaction {
   public:
    weak_ptr<Account> buyer, seller;
    int num_share;
    shared_ptr<Order> order;
    BuyAndSellTransaction(shared_ptr<Account> _buyer, shared_ptr<Account> _seller, int _num, shared_ptr<Order> order, int _id);
    void undo() override;
    void log() override {
        auto buyer_strong = buyer.lock();
        auto seller_strong = buyer.lock();
        cout << "[logging] type: buy and sell" << endl
             << "buyer id:" << buyer_strong->get_id()
             << " seller id: " << seller_strong->get_id()
             << " num_share: " << num_share
             << " stock_id: " << order->get_stock()->get_id()
             << endl;
    }
};

class AddOrderTransaction : public Transaction {
   public:
    shared_ptr<Order> order;
    AddOrderTransaction(shared_ptr<Order> _order, int _id);
    void undo() override;
    void log() override {
        cout << "[logging] type: add order" << endl
             << " stock_id: " << order->get_stock()->get_id()
             << endl;
    }
};

class DelOrderTransaction : public Transaction {
   public:
    shared_ptr<Order> order;
    DelOrderTransaction(shared_ptr<Order> _order, int _id);
    void undo() override;
    void log() override {
        cout << "[logging] type: del order" << endl
             << " stock_id: " << order->get_stock()->get_id()
             << endl;
    }
};

class FreezeTransaction : public Transaction {
   public:
    weak_ptr<Stock> stock;
    FreezeTransaction(shared_ptr<Stock> _stock, int _id);
    void undo() override;
    void log() override {
        auto stock_strong = stock.lock();
        cout << "[logging] type: freeze" << endl
             << " stock_id: " << stock_strong->get_id()
             << endl;
    }
};

class Logs {
   private:
    vector<shared_ptr<Transaction>> contents;  // Operation stack
    int idx;                                   // Assign id to transactions. The same ids indicate they belong to the same transaction.

   public:
    Logs() = default;
    void push_back(shared_ptr<Transaction> log);
    shared_ptr<Transaction> back();
    void pop_back();
    void clear();
    int get_id();
    int size();
#ifdef DEBUG
    friend class Market;
#endif  // DEBUG
};

class Market  // Please DO NOT change the interface here
{
    unordered_map<int, shared_ptr<Stock>> stocks;
    unordered_map<int, shared_ptr<Account>> accounts;
    shared_ptr<Logs> logger;
    shared_ptr<Account> market_account;

   public:
    /**
     * @brief Construct a new Market object. Do something here
     *
     */
    Market();

    /**
     * @brief Begin a new day
     *
     */
    void new_day();

    /**
     * @brief Add a new user
     *
     * @param usr_id user id
     * @param remain The initial money user holds.
     */
    void add_usr(int usr_id, double remain);

    /**
     * @brief Add a new stock
     *
     * @param stock_id  stock id
     * @param num_share the number of shares initially holded by market account
     * @param price     initial price
     */
    void add_stock(int stock_id, int num_share, double price);

    /**
     * @brief Add an order under certain stock
     *
     * @param usr_id        user id
     * @param stock_id      stock id
     * @param num_share     the number of shares
     * @param price         price of order
     * @param order_type    0 for buy, 1 for sell
     * @return              true: successfully added the order, false: fail to add the order
     */
    bool add_order(int usr_id, int stock_id, int num_share, double price, bool order_type);

    /**
     * @brief Buy a stock, the rest is left as an order
     *
     * @param usr_id    user id
     * @param stock_id  stock id
     * @param num_share the number of shares
     * @param price     the desire price
     * @return          the amount of shares successfully trades, -1 for aborting.
     */
    int usr_buy(int usr_id, int stock_id, int num_share, double price);  // buy a stock, return the amount of shares successfully traded, the rest is left as an order. -1 for Abort.

    /**
     * @brief Sell a stock, the rest is left as an order
     *
     * @param usr_id        user id
     * @param stock_id      stock id
     * @param num_share     the number of shares
     * @param price         the desire price
     * @return              the amount of shares successfully trades, -1 for aborting.
     */
    int usr_sell(int usr_id, int stock_id, int num_share, double price);  // sell a stock, return the amount of shares successfully traded, the rest is left as an order. -1 for Abort

    /**
     * @brief the stock with highest price
     *
     * @return (stock_id, price)
     */
    pair<int, double> best_stock();

    /**
     * @brief the stock with lowest price
     *
     * @return (stock_id, price)
     */
    pair<int, double> worst_stock();

    /**
     * @brief Evaluate the value of an account
     * (how much money it could hold suppose it could successfully sell all existing selling orders, neglect the buying orders and shares not for selling yet)
     *
     * @param usr_id    user id
     * @return          the value of an account
     */
    double evaluate(int usr_id);  //

    /**
     * @brief Undo the last transaction
     *
     */
    void undo();

#ifdef DEBUG
    void log();

    bool check();

   private:
    bool check_buy_order_smaller_than_sell_order();

    bool check_ciruit_breaker();

    bool check_usr_money_not_negative();
#endif  // DEBUG
};
#endif