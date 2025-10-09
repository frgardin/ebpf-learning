#include <iostream> 

using namespace std;

int main() {

    char v[8] = "abacate";
    char* p = &v[3];
    cout << p << endl;
    
    char x = *p;
    cout << x << endl;

    for (auto& c : v) {
        cout << c << ' ';   
    }
}