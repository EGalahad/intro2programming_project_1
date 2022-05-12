#include "stock.h"
using namespace std;

void test() {
    auto instance = make_shared<Market>();
    int N = 10;
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

int main() {
    test();
}