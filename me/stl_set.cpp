#include <set>
#include <iostream>

using namespace std;

struct cmpStruct {
    bool operator()(int const &lhs, int const &rhs) const {
        return lhs < rhs;
    }
};

int main(int argc, char const *argv[]) {
    std::set<int, cmpStruct> myInverseSortedSet;
    for (int i = 0; i < 100; i++) {
        myInverseSortedSet.insert(-i);
    }
    for (int i : myInverseSortedSet) {
        cout <<i << " ";
    }
    cout << endl;
    return 0;
}
