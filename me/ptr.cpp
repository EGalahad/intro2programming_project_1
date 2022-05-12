#include <iostream>
#include <memory>
using namespace std;

class A {
    int data;

   public:
    A(int a) : data(a) {
        cout << "constructed @" << (void *)this << endl;
    };
    ~A() { cout << "des @" << this << endl; }
};

int main(int argc, char const *argv[]) {
    auto p = make_shared<A>(1);
    cout << "now try to init to another make shared" << endl;
    p = make_shared<A>(2);
    // p.reset(new A(2));
    cout << "changed" << endl;
    return 0;
}
