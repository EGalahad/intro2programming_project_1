#include "stock.h"
using namespace std;

void test() {
    auto instance = make_shared<Market>();
    int N = 10;
    // ther market
    vector<int> stock_id(1, 0), usr_id(1, 0);
    for (int i = 1; i <= N; i++) {
        stock_id.push_back(i);
        instance->add_stock(stock_id[i], N + 1, i);
    }
    for (int i = 1; i <= N; i++) {
        usr_id.push_back(i);
        instance->add_usr(usr_id[i], (i + 1) * N);
    }
    // for (int i = 1; i <= N; i++)
    //     printf("usr: %d, evaluate: %lf\n", i, instance->evaluate(usr_id[i]));
    instance->new_day();
    for (int i = 1; i <= N; i++) {
        printf("usr: %d, bought %d\n", i, instance->usr_buy(usr_id[i], stock_id[i], N / 2, 2 * i + 1));
    }

    instance->undo();
    printf("usr: %d, bought %d\n", N - 1, instance->usr_buy(N - 1, N, 1, 20));

    for (int i = 1; i <= N; i++)
        printf("usr: %d, evaluate: %lf\n", i, instance->evaluate(usr_id[i]));
    /*
    usr: 1, evaluate: 15.000000
    usr: 2, evaluate: 20.000000
    usr: 3, evaluate: 25.000000
    usr: 4, evaluate: 30.000000
    usr: 5, evaluate: 35.000000
    usr: 6, evaluate: 40.000000
    usr: 7, evaluate: 45.000000
    usr: 8, evaluate: 50.000000
    usr: 9, evaluate: 45.000000
    usr: 10, evaluate: 110.000000
    */

    for (int i = 1; i <= N; i++) {
        printf("usr: %d, sold: %d\n", i, instance->usr_sell(usr_id[i], stock_id[i], N / 2, i - 1));
    }
    for (int i = 1; i <= N; i++)
        printf("usr: %d, evaluate: %lf\n", i, instance->evaluate(usr_id[i]));
    /*
    usr: 1, evaluate: 15.000000
    usr: 2, evaluate: 25.000000
    usr: 3, evaluate: 35.000000
    usr: 4, evaluate: 45.000000
    usr: 5, evaluate: 55.000000
    usr: 6, evaluate: 65.000000
    usr: 7, evaluate: 75.000000
    usr: 8, evaluate: 85.000000
    usr: 9, evaluate: 85.000000
    usr: 10, evaluate: 110.000000
    */
}

void init_market_ptr(Market* market, vector<int> &stock_list, vector<int> &user_list, int n_usr = 50, int n_stk = 60) {
    user_list.push_back(0);
    for (int i = 1; i <= n_usr; i++) {
        user_list.push_back(i);
        market->add_usr(i, i * n_stk * 40);
    }
    for (int i = 1; i <= n_stk; i++) {
        stock_list.push_back(i);
        market->add_stock(i, i * n_usr, i);
    }
}

void init_market(shared_ptr<Market> market, vector<int> &stock_list, vector<int> &user_list, int n_usr = 50, int n_stk = 60) {
    user_list.push_back(0);
    for (int i = 1; i <= n_usr; i++) {
        user_list.push_back(i);
        market->add_usr(i, i * n_stk * 40);
    }
    for (int i = 1; i <= n_stk; i++) {
        stock_list.push_back(i);
        market->add_stock(i, i * n_usr, i);
    }
}

void test_flunc() {
    auto market = make_shared<Market>();
    vector<int> stock_list, user_list;
    init_market(market, stock_list, user_list);
    market->new_day();
    for (int i : user_list) {
        if (i == 0) continue;
        market->usr_buy(i, 30, 2, 10 * i);
        market->check();
    }
    market->new_day();
    for (int i : user_list) {
        if (i == 0) continue;
        market->usr_sell(i, 30, 2, 10);
        market->check();
    }
    for (int i = 0; i < 3; i++) {
        market->undo();
    }
    market->new_day();
}

void test_add_sell_order_then_buy() {
    auto market = make_shared<Market>();
    vector<int> stock_list, user_list;
    init_market(market, stock_list, user_list);
    market->new_day();
    int first_half_stock_id = 30, first_half_num_share = 30 * 10 / 5 + 1;
    // check whether the sell order hold by the market will be removed
    double first_half_bought_price = 30.33;
    int second_half_stock_id = 40, second_half_num_share = 40 * 10 / 5 + 1;
    double second_half_bought_price = 40.33;
    for (int i = 1; i <= user_list.size() / 2; i++) {
        market->usr_buy(i, first_half_stock_id, first_half_num_share, first_half_bought_price);
        market->check();
    }
    for (int i = user_list.size() / 2 + 1; i < user_list.size(); i++) {
        market->usr_buy(i, second_half_stock_id, second_half_num_share, second_half_bought_price);
        market->check();
    }
    market->new_day();
    for (int i = 1; i <= user_list.size() / 2; i++) {
        market->usr_sell(i, first_half_stock_id, (int)first_half_num_share * 0.8, first_half_bought_price * 1.05);
        market->check();
    }
    for (int i = user_list.size() / 2 + 1; i < user_list.size(); i++) {
        market->usr_sell(i, second_half_stock_id, (int)(second_half_num_share * 0.9), second_half_bought_price * 0.98);
        market->check();
    }
    market->new_day();
    for (int i = 1; i <= user_list.size() / 2; i++) {
        market->usr_buy(i, second_half_stock_id, second_half_num_share / 2, second_half_bought_price * 0.98);
        market->check();
    }
    for (int i = user_list.size() / 2 + 1; i < user_list.size(); i++) {
        market->usr_buy(i, first_half_stock_id, (int)(first_half_num_share * 1.2), first_half_bought_price * 1.05);
        market->check();
    }
    market->new_day();
}

void test_illegal() {
    auto market = make_shared<Market>();
    vector<int> stock_list, user_list;
    int n_usr = 40, n_stk = 60;
    init_market(market, stock_list, user_list, n_usr, n_stk);
    market->new_day();
    int target_stock_id = 50, target_num_share = 50 * 10 - 1;
    int party_a = 8, party_b = 9;
    for (int i = 0; i < 7; i++) {
        static const double factor = 1.02;
        static double price = target_stock_id;
        market->usr_buy(party_a, target_stock_id, target_num_share, price);
        price *= factor;
        market->usr_sell(party_a, target_stock_id, target_num_share, price);
        market->usr_buy(party_b, target_stock_id, target_num_share, price);
        price *= factor;
        market->usr_sell(party_b, target_stock_id, target_num_share, price);
        market->check();
    }
    vector<int> hold_list(1, 0);
    for (int i = 1; i < user_list.size(); i++) {
        int stock_id = (long long)(n_stk - 1) * rand() / RAND_MAX + 1;
        hold_list.push_back(stock_id);
        market->usr_buy(i, stock_id, i * 5, stock_id);
        market->check();
    }
    market->new_day();
    for (int i = 1; i < user_list.size(); i++) {
        market->usr_sell(i, hold_list[i], 4 * i, (double)hold_list[i] * 1.03);
        market->check();
        if (rand() > RAND_MAX / 2) market->undo();
        market->check();
    }
    market->new_day();
    for (int i = 1; i < user_list.size(); i++) {
        int stock_id = (long long)(n_stk - 1) * rand() / RAND_MAX + 1;
        double price = stock_id * (1 + (double)rand() / RAND_MAX * 0.2);
        market->usr_buy(i, stock_id, i * 5, price);
        market->check();
    }
    market->new_day();
}

void test_undo() {
    auto market = make_shared<Market>();
    // Market* market = new Market();
    vector<int> stock_list, user_list;
    int n_usr = 40, n_stk = 60;
    init_market(market, stock_list, user_list, n_usr, n_stk);
    market->new_day(); // this day usr i buy stock i, then undo randomly w.p 0.9
    int cnt = 0;
    for (int i = 1; i < user_list.size(); i++) {
        if (market->usr_buy(i, i, 0.9 * n_usr * i, i) != -1) cnt++;
        market->check();
    }
    for (int i = 1; i < cnt; i++) {
        if (rand() < 0.9 * RAND_MAX) market->undo();
        market->check();
    }
    market->new_day(); // this day usr i random choose a stock and buy 0.7 of its share, at original price
    vector<int> hold_list(1, 0);
    for (int i = 1; i < user_list.size(); i++) {
        int stock_id = (long long)(n_stk - 1) * rand() / RAND_MAX + 1;
        hold_list.push_back(stock_id);
        market->usr_buy(i, stock_id, 0.7 * n_usr * i, stock_id);
        market->check();
    }
    market->new_day(); // this day usr i sell a portion of the stock he choose yesterday
    for (int i = 1; i < user_list.size(); i++) {
        market->usr_sell(i, hold_list[i], 4 * i, (double)hold_list[i] * 1.05);
        market->check();
        if (rand() > RAND_MAX / 2) market->undo();
        market->check();
    }
    market->new_day();
    for (int i = 1; i < user_list.size(); i++) {
        int stock_id = (long long)(n_stk - 1) * rand() / RAND_MAX + 1;
        double price = stock_id * (1 + (double)rand() / RAND_MAX * 0.2);
        market->usr_buy(i, stock_id, i * 5, price);
        market->check();
    }
    market->new_day();
    int best_stock_id = market->best_stock().first;
    double best_price = market->best_stock().second;
    for (int i = 1; i < user_list.size(); i++) {
        double factor = 1 + (double)rand() / RAND_MAX * 0.2 - 0.08;
        market->usr_buy(i, best_stock_id, i, best_price * factor);
    }

    int worst_stock_id = market->worst_stock().first;
    double worst_price = market->worst_stock().second;
    for (int i = 1; i < user_list.size(); i++) {
        double factor = 1 + (double)rand() / RAND_MAX * 0.2 - 0.12;
        market->usr_sell(i, worst_stock_id, i, worst_price * factor);
    }
    market->new_day();
    // delete market;
}

int main() {
    // test();
    test_undo();
}