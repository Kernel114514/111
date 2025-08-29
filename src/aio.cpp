#include <QApplication>
#include "gridwidget.h"
#include "defs.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#define CLEAR_COMMAND "cls"
#else
#define CLEAR_COMMAND "clear"
#endif

#include <cstdint>
#include <vector>
#include <fstream>
#include <regex>
#include <thread>

using namespace std;

void initdata(datastorage &data){
    data.auraxp = 0;
    data.boughtblocks = 0;
    data.boughthp = 0;
    data.level = 0;
}

bool configExists(){
    ifstream file("config.txt");
    return file.good();
}

const string BASE64_ALPHABET = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

string base64_encode(const string& input) {
    using namespace std;
    string encoded;
    int val = 0, valb = -6;
    for (char c : input) {
        val = (val << 8) + static_cast<uint8_t>(c);
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(BASE64_ALPHABET[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        encoded.push_back(BASE64_ALPHABET[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    return encoded;
}

string base64_decode(const string& encoded) {
    string decoded;
    vector<int> decode_table(256, -1);
    for (size_t i = 0; i < BASE64_ALPHABET.size(); ++i) {
        decode_table[BASE64_ALPHABET[i]] = i;
    }
    int val = 0, valb = -8;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '=') break;
        if (decode_table[encoded[i]] == -1) continue;
        val = (val << 6) + decode_table[encoded[i]];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

bool configisValid(){
    ifstream fin("config.txt");
    string s;
    fin >> s;
    if(s.length() % 4 != 0){
        return false;
    }
    regex base64_regex("^(?:[A-Za-z00-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$");
    bool isok = regex_match(s, base64_regex);
    if(isok){
        string dc = base64_decode(s);
        int sc = 0;
        for(char c : dc){
            if(c == ';') sc++;
        }
        if(sc == 3) return true;
        else return false;
    }
    return false;
}

vector<string> _split(const string& input){
    vector<string> parts;
    stringstream ss(input);
    string part;
    while(getline(ss, part, ';')){
        parts.push_back(part);
    }
    return parts;
}

void readconfig(datastorage &data){
    ifstream fin("config.txt");
    string s;
    fin >> s;
    string decoded = base64_decode(s);
    auto splited = _split(decoded);
    int size = splited.size();
    splited.reserve(size);
    data.auraxp = stoull(splited[0]);
    data.boughtblocks = stoull(splited[1]);
    data.boughthp = stoull(splited[2]);
    data.level = stoull(splited[3]);
    fin.close();
}

void writeconfig(datastorage &data){
    ofstream fout("config.txt");
    string s = to_string(data.auraxp) + ';' + to_string(data.boughtblocks) + ';' + to_string(data.boughthp) + ';' + to_string(data.level);
    string encoded = base64_encode(s);
    fout << encoded;
    fout.close();
}

void prmain(){
    system(CLEAR_COMMAND);
    cout << "========================================\n";
    cout << "          🐶 Save The Dogs 🐶          \n";
    cout << "========================================\n";
    cout << "Description: This is a game made by team X-Pathfinder\n";
    cout << "How To Play:\n";
    cout << "1. Draw a box(any shape) with the limited blocks in a limited time to prevent the dogs from getting stung by the bees.\n";
    cout << "2. Get XP Aura after each win, you can buy things using XP Aura in the Market.\n";
    cout << "\n";
    cout << "\n";
    cout << "Press S Key for start, P Key for Shop, E Key for exit\n";
    cout << "Remember to press ENTER after you input your choice.\n";
}

void displayshop(datastorage data){
    system(CLEAR_COMMAND);
    cout << " ░▒▓███████▓▒░▒▓█▓▒░░▒▓█▓▒░░▒▓██████▓▒░░▒▓███████▓▒░  \n";
    cout << "░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ \n";
    cout << "░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░ \n";
    cout << " ░▒▓██████▓▒░░▒▓████████▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓███████▓▒░  \n";
    cout << "       ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░        \n";
    cout << "       ░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░        \n";
    cout << "░▒▓███████▓▒░░▒▓█▓▒░░▒▓█▓▒░░▒▓██████▓▒░░▒▓█▓▒░        \n";
    cout << "\n";
    cout << "AuraXP: " << data.auraxp << "\nBought Extra Blocks: " << data.boughtblocks << "\nBought Extra HP: " << data.boughthp << "\nCurrent Level: " << data.level << '\n';
    cout << "Items: " << endl;
    cout << "1. 4 Extra Block（1000 AuraXP）" << endl;  
    cout << "2. 2 Extra Hp（400 AuraXP）" << endl;
    cout << "3. Level Up 1 Level（75 AuraXP）" << endl;
    cout << "R: Return to main menu" << endl;
    cout << "E: Exit" << endl;
    cout << "Please Enter your choice: ";
    return;
}

int main();

void gameshop(datastorage &data){
    displayshop(data);
    char c;
    while(true){
        cin >> c;
        if(c == '1'){
            if(data.auraxp >= 1000) {
                data.auraxp -= 1000;
                data.boughtblocks += 4;
                writeconfig(data);
                cout << "Purchase successful! You bought 5 Extra Block.\n";
                this_thread::sleep_for(chrono::seconds(1));
            } else {
                cout << "Sorry, but you don't have enough AuraXP to buy this item.\nSleeping 3.0 seconds...\n";
                this_thread::sleep_for(chrono::seconds(3));
            }
            system(CLEAR_COMMAND);
            displayshop(data);
            continue;
        }
        else if(c == '2'){
            if(data.auraxp >= 400) {
                data.auraxp -= 400;
                data.boughthp += 2;
                writeconfig(data);
                cout << "Purchase successful! You bought 2 Extra HP.\n";
            } else {
                cout << "Sorry, but you don't have enough AuraXP to buy this item.\nSleeping 3.0 seconds...\n";
            }
            this_thread::sleep_for(chrono::seconds(3));
            system(CLEAR_COMMAND);
            displayshop(data);
            continue;
        }
        else if(c == '3'){
            if(data.auraxp >= 75) {
                data.auraxp -= 75;
                data.level++;
                writeconfig(data);
                cout << "Purchase successful! You leveled up!\n";
            } else {
                cout << "Sorry, but you don't have enough AuraXP to buy this item.\nSleeping 3.0 seconds...\n";
            }
            this_thread::sleep_for(chrono::seconds(3));
            system(CLEAR_COMMAND);
            displayshop(data);
            continue;
        }
        else if(c == 'R' || c == 'r'){
            main();
            return;
        }
        else if(c == 'E' || c == 'e'){
            return;
        }
        else{
            cout << "Invalid Input: " << c << "\nSleeping 1.0 seconds";
            this_thread::sleep_for(chrono::seconds(1));
            system(CLEAR_COMMAND);
            displayshop(data);
            continue;
        }
    }
    return;
}

void gamemain(datastorage &data){
    data.blocks = data.boughtblocks + (rand() % 61 + 20);
    data.current_hp = data.boughthp + (rand() % 11 + 10);
    int argc = 1;
    char *argv[] = {(char*)"aio", nullptr};
    QApplication a(argc, argv);
    GridWidget w(data);
    w.show();
    a.exec();
    writeconfig(data);
    system(CLEAR_COMMAND);
    main();
}

int main(){
#ifdef _WIN32
    SetConsoleTitleA("Save The Dogs");
    SetConsoleOutputCP(CP_UTF8);
#else
    printf("\033]0;Save The Dogs\007"); 
#endif
    prmain();
    char key;
    bool choice;
    while(true){
        cin >> key;
        if(key == 'S' || key == 's'){
            choice = true;
            break;
        }
        else if(key == 'P' || key == 'p'){
            choice = false;
            break;
        }
        else if(key == 'E' || key == 'e'){
            system(CLEAR_COMMAND);
            return 0;
        }
        else{
            cout << "Invalid choice: " << key << '\n';
        }
    }
    datastorage data;
    initdata(data);
    if(!configExists()){
        writeconfig(data);
    }
    if(!configisValid()){
        cout << "Invalid config, regenerating...\n";
        this_thread::sleep_for(chrono::seconds(3));
        remove("config.txt");
        writeconfig(data);
    }
    readconfig(data);
    if(choice){
        gamemain(data);
    }
    else{
        gameshop(data);
    }
    return 0;
}