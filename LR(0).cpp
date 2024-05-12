#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <algorithm>
#include <iomanip>
using namespace std;

struct Grammar {
    string value; // 非终结符
    vector<vector<string>> right; // 产生式右部
};

struct Item {
    string head;
    vector<string> production;
    int dot; // 点的位置

    // 定义小于操作符，用于set和map等容器的排序
    bool operator<(const Item& other) const {
        if (head != other.head) return head < other.head;
        if (dot != other.dot) return dot < other.dot;
        return production < other.production;
    }

    // 定义等于操作符，可用于容器中的查找操作
    bool operator==(const Item& other) const {
        return head == other.head && production == other.production && dot == other.dot;
    }
};


struct State {
    set<Item> items;
};

// 全局变量
map<string, Grammar> grammars;
map<int,Grammar>grammarindex;
string startSymbol;
map<string, set<string>> first, follow;
set<string> terminals, nonTerminals;
vector<State> states;
map<pair<int, string>, int> transitions; // 存储DFA的转移
map<int, map<string, string>> actionTable;
map<int, map<string, int>> gotoTable;

// 计算项目集闭包
// 计算一个项目集的闭包。闭包包括了从初始项目集出发，
// 通过添加产生式右部的非终结符的扩展，可以到达的所有项目。
set<Item> closure(set<Item> items) {
    // 初始化闭包集为初始项目集
    set<Item> closure = items;
    // 工作列表，用于存储待处理的项目
    queue<Item> worklist;

    // 将初始项目集中的每一个项目加入工作列表
    for (const auto &item : items) {
        worklist.push(item);
    }

    // 处理工作列表中的项目，直到列表为空
    while (!worklist.empty()) {
        // 取出工作列表中的第一个项目
        Item current = worklist.front();
        worklist.pop();

        // 如果点的位置不在产生式的末尾
        if (current.dot < current.production.size()) {
            // 获取点右侧的符号
            string B = current.production[current.dot];

            // 如果该符号是非终结符
            if (nonTerminals.find(B) != nonTerminals.end()) {
                // 遍历该非终结符的所有产生式
                for (const auto &prod : grammars[B].right) {

                    if (prod.size() == 1 && prod[0] == "#") {
                        // 空串产生式直接规约，点移动到末尾
                        Item newItem{B, prod, 1};
                        if (closure.insert(newItem).second) {
                            worklist.push(newItem);
                        }
                    } else {
                        Item newItem{B, prod, 0};
                        if (closure.insert(newItem).second) {
                            worklist.push(newItem);
                        }
                    }
                    
                }
            }
        }
    }

    // 返回计算得到的闭包集
    return closure;
}
// 构建LR(0)分析表的DFA
void constructDFA() {
    // 创建并初始化DFA的初始状态，以开始符号的第一个产生式和点在最左侧为起点
    Item initialItem{startSymbol, grammars[startSymbol].right[0], 0};
    set<Item> initialSet = {initialItem};

    // 对初始状态应用闭包操作，以包含所有可能的项目
    states.push_back({closure(initialSet)});

    // 工作列表，用于管理还未处理的状态
    queue<int> worklist;
    worklist.push(0);  // 将初始状态索引加入工作列表

    // 循环处理工作列表中的状态，直到列表为空
    while (!worklist.empty()) {
        int current = worklist.front();  // 当前正在处理的状态
        worklist.pop();

        // 用于记录从当前状态出发的所有可能转移
        map<string, set<Item>> partitions;

        // 遍历当前状态中的所有项目
        for (const auto &item : states[current].items) {
            if (item.dot < item.production.size()) {  // 检查点是否未到达产生式末尾
                string symbol = item.production[item.dot];  // 点后的符号
                Item newItem = item;  // 复制当前项目
                newItem.dot++;  // 移动点的位置
                partitions[symbol].insert(newItem);  // 根据移动后的符号分类项目
            }
        }

        // 处理所有由当前状态通过不同符号转移得到的新项目集
        for (const auto &partition : partitions) {
            set<Item> newClosure = closure(partition.second);  // 对每个分区应用闭包操作
            bool found = false;  // 标记新状态是否已经存在
            int stateIndex = 0;

            // 检查新的闭包集是否已经是一个现有的状态
            for (int i = 0; i < states.size(); i++) {
                if (states[i].items == newClosure) {
                    found = true;
                    stateIndex = i;  // 记录现有状态的索引
                    break;
                }
            }

            // 如果新状态不存在，创建它并添加到状态列表和工作列表中
            if (!found) {
                states.push_back({newClosure});
                stateIndex = states.size() - 1;
                worklist.push(stateIndex);  // 新状态需要进一步处理
            }

            // 记录从当前状态通过符号到新状态的转移
            transitions[{current, partition.first}] = stateIndex;
        }
    }
}


// 打印DFA的状态和转移关系
void printDFA() {
    cout << "DFA States and Transitions:" << endl; // 打印标题

    // 遍历所有状态
    for (int i = 0; i < states.size(); i++) {
        cout << "State " << i << ":" << endl;  // 打印当前状态的索引

        // 打印当前状态包含的所有项目
        for (const auto& item : states[i].items) {
            cout << "  " << item.head << " -> ";  // 打印产生式的左侧部分
            // 遍历产生式的右侧部分
            for (int j = 0; j < item.production.size(); j++) {
                if (j == item.dot) cout << ".";  // 在点的当前位置打印点符号
                cout << item.production[j];      // 打印产生式的符号
            }
            if (item.dot == item.production.size()) cout << ".";  // 如果点在末尾，打印点符号
            cout << endl;  // 换行，结束当前项目的打印
        }

        // 打印从当前状态出发的所有转移关系
        for (const auto& trans : transitions) {
            // 检查转移是否从当前状态开始
            if (trans.first.first == i) {
                // 打印转移使用的符号和目标状态
                cout << "  " << trans.first.second << " -> State " << trans.second << endl;
            }
        }
        cout << endl;  // 在每个状态的输出后添加空行，以便区分
    }
}
void constructLR0Table() {
    for (int i = 0; i < states.size(); i++) {
        for (const auto& item : states[i].items) {
            string symbol;
            int nextState;

            if (item.dot < item.production.size()) {
                symbol = item.production[item.dot];
                if (terminals.find(symbol) != terminals.end()) { // 如果是终结符
                    nextState = transitions[{i, symbol}];
                    string action = "S" + to_string(nextState);
                    if (actionTable[i].find(symbol) != actionTable[i].end() && actionTable[i][symbol] != action) {
                        cout << "Conflict detected at state " << i << " on symbol '" << symbol << "': "
                             << actionTable[i][symbol] << " and " << action << endl;
                    } else {
                        actionTable[i][symbol] = action;
                    }
                }
            }
            if (item.dot == item.production.size()) {
                if (item.head != startSymbol) { // 规约操作
                    for (int idx = 0; idx < grammarindex.size(); ++idx) {
                        if (grammarindex[idx].value == item.head && grammarindex[idx].right[0] == item.production) {
                            for (const string& terminal : terminals) {
                                string action = "R" + to_string(idx);
                                if (actionTable[i].find(terminal) != actionTable[i].end() && actionTable[i][terminal] != action) {
                                    cout << "Conflict detected at state " << i << " on symbol '" << terminal << "': "
                                         << actionTable[i][terminal] << " and " << action << endl;
                                } else {
                                    actionTable[i][terminal] = action;
                                }
                            }
                        }
                    }
                } else { // 接受动作

                        if (actionTable[i].find("$") != actionTable[i].end()) {
                        cout << "Conflict detected at state " << i << " on end symbol '$': "
                             << actionTable[i]["$"] << " and acc" << endl;
                    } else {
                        actionTable[i]["$"] = "acc"; // 接受状态
                    }
                    
                }
            }
        }

        // 填充Goto表
        for (const auto& nt : nonTerminals) {
            if (transitions.find({i, nt}) != transitions.end()) {
                gotoTable[i][nt] = transitions[{i, nt}];
            }
        }
    }
}



void printLR0Table() {
    cout << "LR(0) Analysis Table:" << endl;
    cout << "STATE\t| ";
    for (const auto& terminal : terminals) {
        cout << terminal << "\t";
    }
    cout << "| ";
    for (const auto& nt : nonTerminals) {
        cout << nt << "\t";
    }
    cout << endl;

    for (int i = 0; i < states.size(); i++) {
        cout << i << "\t ";
        for (const auto& terminal : terminals) {
            cout << (actionTable[i].find(terminal) != actionTable[i].end() ? actionTable[i][terminal] : "") << "\t";
        }
        // cout << (actionTable[i].find("$") != actionTable[i].end() ? actionTable[i]["$"] : "") << "\t| ";
        cout<<"| ";
        for (const auto& nt : nonTerminals) {
            cout << (gotoTable[i].find(nt) != gotoTable[i].end() ? to_string(gotoTable[i][nt]) : "") << "\t";
        }
        cout << endl;
    }
}

void analyzeLR0(string& input) {
    stack<string> stateStack;  // 状态栈
    stateStack.push("$0");     // 初始状态压栈

    string buffer = input + "$";  // 输入缓冲区，末尾添加结束符
    size_t index = 0;             // 输入缓冲区的读取位置

    // 打印表头，使用 setw 来设置每列的宽度
    cout << left << setw(20) << "分析栈" << setw(20) << "输入" << setw(20) << "动作" << "\n";

    while (!stateStack.empty()) {
        string currentState = stateStack.top();  // 当前状态
        string currentSymbol(1, buffer[index]);  // 当前输入符号
        string action = actionTable[stoi(currentState.substr(1))][currentSymbol];  // 查找对应动作

        // 打印当前分析栈
        stack<string> tempStack = stateStack;  // 创建临时栈用于逆序打印
        vector<string> stackContents;  // 存储栈内容的向量
        while (!tempStack.empty()) {
            stackContents.push_back(tempStack.top());
            tempStack.pop();
        }
        reverse(stackContents.begin(), stackContents.end());  // 反转以正确顺序显示

        // 准备分析栈内容输出
        // stringstream stackStream;
        for (auto elem : stackContents) {
            cout << elem << "";
        }

        cout << left << "\t\t" << buffer.substr(index) << "\t\t" << action << "\n";

        // 处理动作
        if (action[0] == 'S') {  // 移进
            string newState = string(1, buffer[index]) + action.substr(1);
            stateStack.push(newState);  // 移进状态入栈
            index++;  // 读取下一个输入符号
        } else if (action[0] == 'R') {  // 规约
            int prodIndex = stoi(action.substr(1));
            string head = grammarindex[prodIndex].value;
            int bodyLength = grammarindex[prodIndex].right[0].size();
            for (int i = 0; i < bodyLength; ++i) {
                stateStack.pop();
            }
            string top = stateStack.top();
            int nextState = stoi(top.substr(1));
            string newState = head + to_string(gotoTable[nextState][head]);
            stateStack.push(newState);  // 规约后的转移状态入栈
        } else if (action == "acc") {  // 接受
            cout << "Accept\n";
            break;
        } else {  // 错误
            cout << "Error\n";
            break;
        }
    }
}
void inputGrammars(map<string, Grammar>& grammars) {
    cout << "请输入文法规则\n";
    string line;
    bool first=true;
    while (getline(cin, line)) {
        size_t pos = line.find("->");
        
        if (pos != string::npos) {
            string nonTerminal = line.substr(0, pos - 1);
            string productions = line.substr(pos + 3);
            vector<vector<string>> prodList;
            size_t start = 0, end;

            while ((end = productions.find('|', start)) != string::npos) {
                string production = productions.substr(start, end - start);
                vector<string> symbols;
                size_t s_start = 0, s_end;
                while ((s_end = production.find(' ', s_start)) != string::npos) {
                    if (s_end != s_start) {
                        symbols.push_back(production.substr(s_start, s_end - s_start));
                    }
                    s_start = s_end + 1;
                }
                symbols.push_back(production.substr(s_start));
                prodList.push_back(symbols);
                start = end + 1;
            }

            //一个产生式
            string production = productions.substr(start);
            vector<string> symbols;
            size_t s_start = 0, s_end;
            while ((s_end = production.find(' ', s_start)) != string::npos) {
                if (s_end != s_start) {
                    symbols.push_back(production.substr(s_start, s_end - s_start));
                }
                s_start = s_end + 1;
            }
            symbols.push_back(production.substr(s_start));
            prodList.push_back(symbols);

            grammars[nonTerminal] = {nonTerminal, prodList};
            if(first){
                vector<string>tt;
                tt.push_back(nonTerminal);
                vector<vector<string>>vv;
                vv.push_back(tt);
            grammars[string(nonTerminal+"~")]={string(nonTerminal+"~"),vv};
            startSymbol=string(nonTerminal+"~");
            first=false;
            }
        }
    }
}
void GetTerminal(map<string, Grammar>& grammars)
{

    // 将所有非终结符加入NON_TERMINAL集合中
    for(auto entry:grammars)
    {
        nonTerminals.insert( entry.first);
    }
    // 遍历文法规则，将产生式中的终结符加入TERMINAL集合中
    for (auto entry:grammars) {
        auto grammar = entry.second; // 获取文法规则的产生式列表
        for (auto vec : grammar.right) {
            for (auto str : vec) {
                // 如果产生式符号不在非终结符集合中且不是空串，则将其加入终结符集合
                if (nonTerminals.find(str) == nonTerminals.end() && str != "#") {
                    terminals.insert(str);
                }
            }
        }
    }

    // 添加结束符号$
    terminals.insert("$");
}
int main() {
    // // 初始化文法
    // grammars["S~"] = {"S~", {{"S"}}};
    // grammars["S"] = {"S", {{"a","A"}}};
    // grammars["A"] = {"A", {{"c", "A"}, {"d"}}};
    inputGrammars(grammars);
    // 分配唯一索引给每个产生式并填充 grammarindex
    int index = 0;
    for (auto& [nonTerminal, grammar] : grammars) {
        for (auto& production : grammar.right) {
            grammarindex[index++] = {nonTerminal, {production}};
        }
    }
    GetTerminal(grammars);

    // 构建DFA和分析表
    constructDFA();
    printDFA();
    constructLR0Table();
    printLR0Table();

    // 进行分析
    string input;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.clear();
    cout << "请输入分析串：";
    getline(cin, input); // 推荐使用 getline 以避免潜在问题

    analyzeLR0(input);
    return 0;
}






