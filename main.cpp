#include <algorithm>    // find_if
#include <exception>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>      // pair
#include <vector>
#include "Classes.h"
#include "Functions.h"

using namespace std;

bool     conversation();
string   prompt      (string = "");
void     menupg      (vector<Item>, int);
void     menu        (vector<Item>);
ifstream chk_openFile(string);
void     generateList(string, vector<string>&);
bool     test        (int, char**);

struct {
    bool           needsDelivery;
    float          budget;
    vector<string> mentionedStalls;
    vector<string> mentionedTypes;
} session;

struct {
    vector<string> stallNames;
    vector<string> dishTypes;
    vector<string> meatTypes;
    vector<string> keywords;
    vector<pair<string, vector<string>>> tags;
} lists;

struct Items {
    // Initialise menu
    Items() {
        ifstream file(chk_openFile("Food List.csv"));
        string line;
        getline(file, line);
        while (getline(file, line)) {
            if (line.back() == '\r')
                line.pop_back();
            items.push_back(Item(line));
        }
    }
    size_t       size()               { return items.size();  }
    Item         operator[](size_t i) { return items[i];      }
    vector<Item> operator()()         { return items;         }
    vector<Item>::iterator begin()    { return items.begin(); }
    vector<Item>::iterator end()      { return items.end();   }
private:
    vector<Item> items;
} items;

Sentence     sentence;
static bool  start = true;
Item         result;
vector<Item> results;
vector<Item> orders;

int main(int argc, char** argv) {
    try {
        {   
            // Generate lists
            for (Item item : items) {
                generateList(item._stallName(), lists.stallNames);
                generateList(item._dishType(),  lists.dishTypes);
                generateList(item._meatType(),  lists.meatTypes);
            }
            sort(lists.stallNames.begin(), lists.stallNames.end());
            sort(lists.dishTypes.begin(),  lists.dishTypes.end());
            sort(lists.meatTypes.begin(),  lists.meatTypes.end());
        }

        {   // Initialise keywords and tags
            ifstream file(chk_openFile("parse/responseTags.csv"));
            string line;
            while (getline(file, line)) {
                string keyword, tagStr;
                stringstream ss(line);
                if (line.back() == '\r') line.pop_back(); 
                getline(ss, keyword, ',');
                ss >> tagStr;
                lists.keywords.push_back(keyword);
                
                // Parse tags
                string tag;
                ss = stringstream(tagStr);
                vector<string> tags;
                while (getline(ss, tag, ';'))
                    tags.push_back(tag);
                
                // Add entry
                pair<string, vector<string>> entry(keyword, tags);
                lists.tags.push_back(entry);
            }
        }
        
        if (test(argc, argv)) return 0;

        while (conversation());
    }

    catch (const exception& e) {
        cerr << "Exception: " << e.what() << endl;
        return -1;
    }
    
    return 0;
}

bool conversation() {
    if (start) {
        // Initial state
        sentence.read(prompt("Hi, I'm Ken. How can I help you out today?"));
        start = false;
    } else {
        // Subsequent state
        sentence.read(prompt("Is there anything else I can help you with?"));
    }
    
    /*
    for all words, search through
    check keywords
    then 
        check each non-keyword word with remainng lists
        for words that return results, take intersection
    */

    // Parse keywords
    for (string word : sentence.get()) {
        if (inside(word, lists.keywords))
            sentence.addKey(word);
    }

    //  pseudocode
    /*  
    - "order"
            • replies what to order if there's no food specified
            • show's the menu and recommendation
    - picks food with ID(implement some type of ID system) or food name
            • adds food to cart
            • asks user if there's anything else to add (becomes a loop unless proceed to checkout/quit)
    - Chooses checkout
            • Shows order summary
            • option to add or remove foods from cart
            • option to confirm order
    - Confirms order
            • If there's food that's deliverable, asks if user wants it delivered.
                • Prompt delivery address
            • Sends receipt
    */

    if (sentence.is("order")) { // will change this to finding keywords
        // shows menu
        // gives recommendation
        cout << "Here's our menu.\nType page x to go to it.\nType numbers to select meal." << endl;
        menu(items());
        /*showMenu(1);
        sentence.read(prompt(""));
        while (!sentence.is("bye")){
            if (sentence.contains("page"))
                showMenu(1);
                // showMenu()
            // adds food procedure
        }
        sentence.println();*/
    }

    if (sentence.is("bye")) { // temp end condition
        cout << "Goodbye. Have a nice day!" << endl;
        return false;
    }

    return true;
}

string prompt(string str) {
    if (str != "") 
        cout << str << endl;
    cout << ">> ";
    string response;
    getline(cin, response);
    return response;
}

void menu(vector<Item> items) {
    int  off, range;
    int  cur  = 1;
    int  tot  = (items.size() - 1) / 10 + 1;
    bool pgin = false;

    const vector<string> keys_cancel   = {"cancel"};
    const vector<string> keys_previous = {"previous", "prev", "back", "left"};
    const vector<string> keys_next     = {"next", "forward", "right"};
    const vector<string> keys_page     = {"page", "pg", "go", "to", "goto"};

    auto menupg = [&]() {
        off = (cur - 1) * 10 + 1;
        range = items.size() - off + 1;
        for (int i = off; i < off + (range < 10 ? range : 10); i++)
            items[i - 1].list(i);
        cout << "Page " << cur << "/" << tot << endl;
        pgin = false;
    };

    auto iteminbounds = [&](int i) {
        if (i < off || i >= off + (range < 10 ? range : 10)) {
                cout << "Item not on page!" << endl;
                return true;
            } else {
                result = items[i - 1];
                return false;
            }
    };

    menupg(); // Always display first page

    if (items.size() <= 10) {
        while ([&]() -> bool {
            sentence.read(prompt());
            if (sentence.contains(keys_cancel)) {
                return false;
            } else if (sentence.contains(keys_previous)) {
                cout << "No previous page!" << endl;
            } else if (sentence.contains(keys_next)) {
                cout << "No next page!" << endl;
            } else {
                vector<string>::iterator it = find_if(sentence.begin(), sentence.end(), isnumstr);
                bool number = it != sentence.end();

                if (sentence.contains(keys_page)) {
                    if (number && stoi(*it) == 1) {
                        menupg();
                    } else {
                        cout << "No other pages to go to!" << endl;
                    }
                } else if (number) {
                    return iteminbounds(stoi(*it));
                } else {
                    // TO IMPLEMENT SEARCH ROUTINE
                    cout << "Search" << endl;
                    return false;
                }
            }
            return true;
        }());
    } else {
        while ([&]() -> bool {
            sentence.read(prompt());
            if (sentence.contains(keys_cancel)) {
                return false;
            } else if (sentence.contains(keys_previous)) {
                if (cur != 1) {
                    --cur;
                    menupg();
                } else {
                    cout << "No previous page!" << endl;
                }
            } else if (sentence.contains(keys_next)) {
                if (cur != tot) {
                    ++cur;
                    menupg();
                } else {
                    cout << "No next page!" << endl;
                }
            } else {
                auto pginbounds = [&](vector<string>::iterator it) {
                    if (stoi(*it) < 1 || stoi(*it) > tot) {
                        cout << "Page does not exist!" << endl;
                        pgin = true;
                    } else {
                        cur = stoi(*it);
                        menupg();
                    }
                };

                vector<string>::iterator it = find_if(sentence.begin(), sentence.end(), isnumstr);
                bool number = it != sentence.end();

                if (sentence.contains(keys_page)) {
                    if (!number) {
                        cout << "Which page would you like to go to?" << endl;
                        pgin = true;
                    } else {
                        pginbounds(it);
                    }
                } else if (number) {
                    if (pgin) {
                        pginbounds(it);
                    } else {
                        return iteminbounds(stoi(*it));
                    }
                } else {
                    // TO IMPLEMENT SEARCH ROUTINE
                    cout << "Search" << endl;
                    return false;
                /*  search sentence for item names found in items
                    if (match count == 1) {
                        result = matched item
                    } else if (match count > 1) {
                        cout << "I found these items, which one would you like?" << endl;
                        menu(matched items)
                    } else {
                        cout << "Sorry, I didn't understand that." << endl;
                        return true;
                    }
                    return false; */
                }
            }
            return true;
        }());
    }
}

ifstream chk_openFile(string fileName) {
    ifstream file(fileName);
    if (file.is_open()) {
        cout << fileName << " opened successfully." << endl;
    } else {
        throw runtime_error("\"" + fileName + "\" cannot be opened.");
    }
    return file;
}

void generateList(string str, vector<string>& v) {
    if (!inside(str, v))
        v.push_back(str);
}

bool test(int argc, char** argv) {
    cout << (argc > 1 ? "[TEST MODE]\n" : "");

    // Declare test functions
    auto tf_list = [](vector<string> v) {
        for (string str : v)
            cout << str << ", ";
        cout << endl;
    };

    // Return true to end program after test
    switch (argc) {
    case 1:
        return false;

    case 2:
        switch (::hash(argv[1])) {
        // Sandbox test area
        case ::hash("test"):
            
            return true;

        case ::hash("open"):
            return true;

        case ::hash("list"):
            // Print lists
            cout << "Stall names: "; tf_list(lists.stallNames);
            cout << "Dish types:  "; tf_list(lists.dishTypes);
            cout << "Meat types:  "; tf_list(lists.meatTypes);
            cout << "Keywords:    "; tf_list(lists.keywords);
            return true;

        case ::hash("items"):
            // Display items
            for (Item item : items)
                item.display();
            cout << "Item count: " << items.size() << endl;
            return true;

        case ::hash("menu"):
            // Display menu and result
            menu(vector(items.begin(), items.begin() + 10));
            result.display();
            return true;

        default:
            cout << "Unknown argument" << endl;
            return true;
        }

    default:
        cout << "Invalid argument count" << endl;
    }
    return true;
}