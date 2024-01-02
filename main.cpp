#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

int menu(){
    int choice = -1;
    cout << "Cac lua chon: "<< endl;
    cout << "1. Cap nhat du lieu theo 1 chieu" << endl;
    cout << "2. Cap nhat du lieu theo 2 chieu" << endl;
    cout << "3. Ket thuc" << endl;
    cout << "Lua chon cua ban: ";
    cin >> choice;
    if (choice < 1 or choice >3) choice = -1;
    return choice;
}

int main(){
    bool is_continue = true;
    int choice;
    string continue_choice;
    do{
        choice = menu();
        if (choice == -1) {
            cout << "Lua chon khong dung. Ban muon tiep tuc [y/N]? ";
            cin >> continue_choice;
            if (continue_choice == "y") {
            system("clear");
            continue;
            } else is_continue = false; 
        }
        if (choice == 3) {
            is_continue = false;
            break;
        }else{
            system("clear");
        }

        /// To: Process



        ///

        cout << "Ban muon tiep tuc? [y/N]? ";
        cin >> continue_choice;
        if (continue_choice == "y") {
            system("clear");
            continue;
        } else is_continue = false; 

    }while(is_continue);
    return 0;
}