#include <cmath>
#include <vector>
#include <cstdio>
#include <random>
#include <fstream>
#include <istream>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include "cadical.hpp"

using namespace std;

enum GateType {
    Buf, Not,
    And, Or, Xor,
    Nand, Nor, Xnor
};

GateType gate_from_str(const string& s) {
    if (s == "buf") return Buf;
    if (s == "not") return Not;
    if (s == "and") return And;
    if (s == "or") return Or;
    if (s == "xor") return Xor;
    if (s == "nand") return Nand;
    if (s == "nor") return Nor;
    if (s == "xnor") return Xnor;
    throw std::runtime_error("Invalid gate type: " + s);
}

string gate_to_str(const GateType g) {
    switch (g) {
    case Buf: return "buf";
    case Not: return "not";
    case And: return "and";
    case Or: return "or";
    case Xor: return "xor";
    case Nand: return "nand";
    case Nor: return "nor";
    case Xnor: return "xnor";
    }
    throw std::runtime_error("Invalid gate type");
}

struct Gate {
    GateType type;
    string name;
    vector<int> inputs_n;
    int output_n;
};

struct Node {
    int num = 0;
    int set_index = -1;
    vector<int> node_to = {};
    vector<int> support_set = {};
    vector<int> support_set_index = {}; // 支持集所在的分割序号
    int random_2_num = 0;
    int random_3_num = 0;
};

struct CircuitData {
    vector<Gate> gates;

    unordered_map<string, int> string_to_int;
    unordered_map<int, string> int_to_string;
    vector<int> inputs_n;
    vector<int> outputs_n;
    vector<int> wires_n;

    vector<vector<int>> input_set;
    vector<vector<int>> output_set;

    int ports_n = 0;

    static CircuitData read_input(istream& in, int cir_n);
    void find_support_set_one(int u, vector<int>& support_set, int u_temp);
    void find_support_set_all();
    void build_support_set();
    void initial_support_set_index();
};

vector<Node*> nodes;
vector<bool> visited;
vector<int> p; // 上次循环的索引

CircuitData CircuitData::read_input(istream& in, int cir_n) {
    unordered_map<string, int> string_to_int;
    unordered_map<int, string> int_to_string;
    int index = 1;
    string token;
    //     module   top      (
    in >> token >> token >> token;
    while (in >> token, token != ")") {
        if (token == ",") continue;
        Node* node = new Node;
        node->num = cir_n + index++;
        nodes.push_back(node);
    }
    index = 1;
    //      ;
    in >> token;
    //    input    a1 , a2 , ...
    in >> token;

    vector<int> inputs_n;
    while (in >> token, token != ";") {
        if (token == ",") continue;
        inputs_n.push_back(cir_n + (index++));
        string_to_int[token] = cir_n + index - 1;
        int_to_string[cir_n + index - 1] = token;
    }
    //    output   o1 , o2 , ...
    in >> token;

    vector<int> outputs_n;
    while (in >> token, token != ";") {
        if (token == ",") continue;
        outputs_n.push_back(cir_n + (index++));
        string_to_int[token] = cir_n + index - 1;
        int_to_string[cir_n + index - 1] = token;
    }
    //    wire     w1 , w2 , ...
    //    or maybe no wire
    in >> token;

    vector<int> wires_n;
    if (token == "wire") {
        while (in >> token, token != ";") {
            if (token == ",") continue;
            wires_n.push_back(cir_n + (index++));
            Node* node = new Node;
            node->num = cir_n + index - 1;
            nodes.push_back(node);
            string_to_int[token] = cir_n + index - 1;
            int_to_string[cir_n + index - 1] = token;
        }
        in >> token;
    }
    vector<Gate> gates;
    while (token != "endmodule") {
        auto type = gate_from_str(token);
        string name;
        string output;
        //    g1        (        wi        ,
        in >> name >> token >> output >> token;
        vector<int> inputs_n;
        int output_n = string_to_int[output];
        while (in >> token, token != ")") {
            if (token == ",") continue;
            int input_n = string_to_int[token];
            inputs_n.push_back(input_n);
            nodes[input_n]->node_to.push_back(output_n);
        }
        //     ;
        in >> token;
        auto gate = Gate{ type, name, inputs_n, output_n };
        gates.push_back(gate);
        in >> token;
    }
    int ports_n = inputs_n.size() + outputs_n.size() + wires_n.size();
    vector<vector<int>> input_set(inputs_n.size());
    vector<vector<int>> output_set(outputs_n.size());
    return CircuitData{ gates, string_to_int, int_to_string, inputs_n, outputs_n, wires_n, input_set, output_set, ports_n };
}

void CircuitData::find_support_set_one(int u, vector<int>& support_set, int u_temp) {
    visited[u] = true;
    for (auto v : nodes[u]->node_to) {
        if (!visited[v]) {
            if (find(outputs_n.begin(), outputs_n.end(), v) != outputs_n.end()) { // 如果是output节点
                visited[v] = true;
                support_set.push_back(v);
                nodes[v]->support_set.push_back(u_temp);
            }
            find_support_set_one(v, support_set, u_temp);
        }
    }
}

void CircuitData::find_support_set_all() {
    for (auto input : inputs_n) {
        fill(visited.begin(), visited.end(), false);
        find_support_set_one(input, nodes[input]->support_set, input);
    }
}

bool support_set_size_compare(const int& a, const int& b) {
    return nodes[a]->support_set.size() < nodes[b]->support_set.size();
}

void CircuitData::build_support_set() {
    vector<int> inputs_copy = inputs_n;
    sort(inputs_copy.begin(), inputs_copy.end(), support_set_size_compare);
    int index = 0;
    input_set[index].push_back(inputs_copy[0]);
    nodes[inputs_copy[0]]->set_index = index;
    for (int i = 1; i < inputs_n.size(); i++) {
        if (nodes[inputs_copy[i]]->support_set.size() > nodes[inputs_copy[i - 1]]->support_set.size()) {
            index += 1;
            input_set[index].push_back(inputs_copy[i]);
            nodes[inputs_copy[i]]->set_index = index;
        }
        else {
            input_set[index].push_back(inputs_copy[i]);
            nodes[inputs_copy[i]]->set_index = index;
        }
    }

    vector<int> outputs_copy = outputs_n;
    sort(outputs_copy.begin(), outputs_copy.end(), support_set_size_compare);
    index = 0;
    output_set[index].push_back(outputs_copy[0]);
    nodes[outputs_copy[0]]->set_index = index;
    for (int i = 1; i < outputs_n.size(); i++) {
        if (nodes[outputs_copy[i]]->support_set.size() > nodes[outputs_copy[i - 1]]->support_set.size()) {
            index += 1;
            output_set[index].push_back(outputs_copy[i]);
            nodes[outputs_copy[i]]->set_index = index;
        }
        else {
            output_set[index].push_back(outputs_copy[i]);
            nodes[outputs_copy[i]]->set_index = index;
        }
    }
}

void CircuitData::initial_support_set_index() {
    for (int i = 0; i < inputs_n.size(); i++) {
        int u = inputs_n[i];
        for (auto v : nodes[u]->support_set) {
            nodes[u]->support_set_index.push_back(nodes[v]->set_index);
        }
        sort(nodes[u]->support_set_index.begin(), nodes[u]->support_set_index.end());
    }

    for (int i = 0; i < outputs_n.size(); i++) {
        int u = outputs_n[i];
        for (auto v : nodes[u]->support_set) {
            nodes[u]->support_set_index.push_back(nodes[v]->set_index);
        }
        sort(nodes[u]->support_set_index.begin(), nodes[u]->support_set_index.end());
    }
}

void solver_add_gate(CaDiCaL::Solver& solver, const Gate& g) {
    switch (g.type) {
    case Not: {
        solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(0);
    }
            break;
    case Buf: {
        solver.add(-1 * g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
        solver.add(g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(0);
    }
            break;
    case Nand: {
        solver.add(-1 * g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(-1 * g.inputs_n[1]); solver.add(0);
        solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
        solver.add(g.output_n); solver.add(g.inputs_n[1]); solver.add(0);
    }
             break;
    case And: {
        solver.add(g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(-1 * g.inputs_n[1]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(g.inputs_n[1]); solver.add(0);
    }
            break;
    case Nor: {
        solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(-1 * g.inputs_n[1]); solver.add(0);
    }
            break;
    case Or: {
        solver.add(-1 * g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
        solver.add(g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(0);
        solver.add(g.output_n); solver.add(-1 * g.inputs_n[1]); solver.add(0);
    }
           break;
    case Xnor: {
        solver.add(-1 * g.output_n); solver.add(g.inputs_n[0]); solver.add(-1 * g.inputs_n[1]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
        solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
        solver.add(g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(-1 * g.inputs_n[1]); solver.add(0);
    }
             break;
    case Xor: {
        solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(-1 * g.inputs_n[1]); solver.add(0);
        solver.add(g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
        solver.add(-1 * g.output_n); solver.add(-1 * g.inputs_n[0]); solver.add(-1 * g.inputs_n[1]); solver.add(0);
    }
            break;
    default:
        break;
    };
    return;
}

void solver_add_match(CaDiCaL::Solver& solver, CircuitData cir_1, CircuitData cir_2) {
    for (auto g : cir_1.gates) {
        solver_add_gate(solver, g);
    }
    for (auto g : cir_2.gates) {
        solver_add_gate(solver, g);
    }
    return;
}

bool support_index_compare(const int& a, const int& b) {
    return nodes[a]->support_set_index < nodes[b]->support_set_index;
}

void support_set_refine(vector<vector<int>>& x) {
    for (int i = 0; i < x.size(); i++) {
        vector<int>& set = x[i];
        if (set.size() != 0 && set.size() != 1) {
            // 字典序排序
            sort(set.begin(), set.end(), support_index_compare);
            // 分割set
            int index = 1;
            vector<int> insert_x = {};
            x.insert(x.begin() + i + (index++), insert_x); // 插入第一个元素
            x[i + index - 1].push_back(x[i][0]);
            for (int j = 1; j < x[i].size(); j++) {
                if (nodes[x[i][j]]->support_set_index == nodes[x[i][j - 1]]->support_set_index) { // 如果两个元素的支持集编号相同
                    x[i + index - 1].push_back(x[i][j]);
                }
                else {
                    vector<int> insert_x = {};
                    x.insert(x.begin() + i + (index++), insert_x); // 插入下一个元素
                    x[i + index - 1].push_back(x[i][j]);
                }
            }
            x.erase(x.begin() + i);
            i += index - 2; // 更新编号
        }
    }
}

int get_set_num(vector<vector<int>>& x) {
    int set_size = 0;
    for (int i = 0; i < x.size(); i++) {
        if (x[i].size() != 0) {
            set_size += 1;
        }
    }
    return set_size;
}

void update_set_index(vector<vector<int>>& x) {
    for (int i = 0; i < x.size(); i++) {
        if (x[i].size() != 0) {
            for (auto u : x[i]) {
                nodes[u]->set_index = i;
            }
        }
    }
}

// 更新support_index;
void update_support_set_index(vector<vector<int>>& x) {
    for (int i = 0; i < x.size(); i++) {
        if (x[i].size() != 0) {
            for (auto u : x[i]) {
                nodes[u]->support_set_index.clear(); // 清除原本的支持集编号
                for (auto v : nodes[u]->support_set) {
                    nodes[u]->support_set_index.push_back(nodes[v]->set_index); // 更新每一个节点的支持集编号
                }
                sort(nodes[u]->support_set_index.begin(), nodes[u]->support_set_index.end()); // 更新后对进行排序
            }
        }
    }
}

vector<int> random_set; // 记录生成的随机序列
vector<int> set_value; // 记录set产生的结果

// 生成k个-1或1
vector<int> generateRandomOnes(int k) {
    vector<int> numbers(k);
    // 初始化随机数生成器
    random_device rd;
    mt19937 gen(rd());
    bernoulli_distribution dis(0.5); // 50% 概率生成 1, 50% 概率生成 -1

    // 生成 k 个随机数
    for (int i = 0; i < k; i++) {
        numbers[i] = dis(gen) ? 1 : -1;
    }
    return numbers;
}

void random_simulation_refine_1(CircuitData& cir_1, CircuitData& cir_2) {
    CaDiCaL::Solver solver;
    solver_add_match(solver, cir_1, cir_2);
    set_value.clear();
    set_value.push_back(0);  
    vector<vector<int>>& x = cir_1.input_set;
    vector<vector<int>>& y = cir_2.input_set;
    vector<vector<int>>& z = cir_1.output_set;
    vector<vector<int>>& w = cir_2.output_set;
    int set_size = get_set_num(x);
    random_set = generateRandomOnes(set_size); // 生成一组随机数

    for (int i = 0; i < set_size; i++) {
        for (auto u : x[i]) {
            solver.add(u * random_set[i]);
            solver.add(0);
        }
    }
    for (int i = 0; i < set_size; i++) {
        for (auto u : y[i]) {
            solver.add(u * random_set[i]);
            solver.add(0);
        }
    }
    
    int result = solver.solve();
    if (result == 10) { // SAT
        for (int i = 1; i <= cir_1.ports_n + cir_2.ports_n; i++) {
            set_value.push_back(solver.val(i));
        }
        for (int i = 0; i < z.size(); i++) {
            if (z[i].size() != 0 && z[i].size() != 1) {
                vector<int> insert_z = {};
                z.insert(z.begin() + i + 1, insert_z);
                // 在后面插入一个数组

                for (int j = 0; j < z[i].size(); j++) {
                    int u = z[i][j];
                    if (solver.val(u) < 0) {
                        z[i + 1].push_back(u);
                        z[i].erase(z[i].begin() + j); // 把负数从原数组中删除
                        j -= 1;
                    }
                }
                if (z[i + 1].size() != 0) { // 存在负数
                    if(z[i].size() == 0){
                        z.erase(z.begin() + i);
                        i -= 1;
                    }
                    i += 1;
                }
                else { // 否则就删除
                    z.erase(z.begin() + i + 1);
                }
            }
        }

        for (int i = 0; i < w.size(); i++) {
            if (w[i].size() != 0 && w[i].size() != 1) {
                vector<int> insert_w = {};
                w.insert(w.begin() + i + 1, insert_w);
                // 在后面插入一个数组

                for (int j = 0; j < w[i].size(); j++) {
                    int u = w[i][j];
                    if (solver.val(u) < 0) {
                        w[i + 1].push_back(u);
                        w[i].erase(w[i].begin() + j); // 把负数从原数组中删除
                        j -= 1;
                    }
                }
                if (w[i + 1].size() != 0) { // 存在负数
                    if(w[i].size() == 0){
                        w.erase(w.begin() + i);
                        i -= 1;
                    }
                    i += 1;
                }
                else { // 否则就删除
                    w.erase(w.begin() + i + 1);
                }
            }
        }
    }
    
}

bool random_2_compare(const int& a, const int& b) {
    return nodes[a]->random_2_num < nodes[b]->random_2_num;
}

void random_simulation_refine_2(CircuitData& cir_1, CircuitData& cir_2) {
    // 清除solver的random_2_num
    for (auto u : nodes) {
        u->random_2_num = 0;
    }

    for (int i = 1; i <= cir_1.inputs_n.size(); i++) {
        set_value[i] *= -1;
        set_value[i + cir_1.ports_n] *= -1;
        CaDiCaL::Solver expand_solver;
        solver_add_match(expand_solver, cir_1, cir_2);
        for (int j = 1; j <= cir_1.inputs_n.size(); j++) { // 反转输入
            expand_solver.add(set_value[j]);
            expand_solver.add(0);

            expand_solver.add(set_value[j + cir_1.ports_n]);
            expand_solver.add(0);
        }
        set_value[i] *= -1;
        set_value[i + cir_1.ports_n] *= -1;
        
        int result = expand_solver.solve();
        if (result == 10) { //  SAT
            for (int k = cir_1.inputs_n.size() + 1; k <= cir_1.inputs_n.size() + cir_1.outputs_n.size(); k++) {
                if (expand_solver.val(k) != set_value[k]) { // cir_1
                    nodes[i]->random_2_num++;
                }
                if (expand_solver.val(k + cir_1.ports_n) != set_value[k + cir_1.ports_n]) { // cir_2
                    nodes[i + cir_1.ports_n]->random_2_num++;
                }
            }
        }
        
    }

    // 划分集合
    vector<vector<int>>& x = cir_1.input_set;
    vector<vector<int>>& y = cir_2.input_set;

    for (int i = 0; i < x.size(); i++) {
        vector<int>& set = x[i];
        if (set.size() != 0 && set.size() != 1) {
            // 排序
            sort(set.begin(), set.end(), random_2_compare);
            // 分割set
            int index = 1;
            vector<int> insert_x = {};
            x.insert(x.begin() + i + (index++), insert_x); // 插入第一个元素
            x[i + index - 1].push_back(x[i][0]);
            for (int j = 1; j < x[i].size(); j++) {
                if (nodes[x[i][j]]->random_2_num == nodes[x[i][j - 1]]->random_2_num) { // 如果两个元素的random_2_num相同
                    x[i + index - 1].push_back(x[i][j]);
                }
                else {
                    vector<int> insert_x = {};
                    x.insert(x.begin() + i + (index++), insert_x); // 插入下一个元素
                    x[i + index - 1].push_back(x[i][j]);
                }
            }
            x.erase(x.begin() + i);
            i += index - 2; // 更新编号
        }
    }
    for (int i = 0; i < y.size(); i++) {
        vector<int>& set = y[i];
        if (set.size() != 0 && set.size() != 1) {
            // 排序
            sort(set.begin(), set.end(), random_2_compare);
            // 分割set
            int index = 1;
            vector<int> insert_y = {};
            y.insert(y.begin() + i + (index++), insert_y); // 插入第一个元素
            y[i + index - 1].push_back(y[i][0]);
            for (int j = 1; j < y[i].size(); j++) {
                if (nodes[y[i][j]]->random_2_num == nodes[y[i][j - 1]]->random_2_num) { // 如果两个元素的random_2_num相同
                    y[i + index - 1].push_back(y[i][j]);
                }
                else {
                    vector<int> insert_y = {};
                    y.insert(y.begin() + i + (index++), insert_y); // 插入下一个元素
                    y[i + index - 1].push_back(y[i][j]);
                }
            }
            y.erase(y.begin() + i);
            i += index - 2; // 更新编号
        }
    }
}

void random_simulation_refine_3(CircuitData& cir_1, CircuitData& cir_2) {
    // 清除solver的random_1
    for (auto u : nodes) {
        u->random_3_num = 0;
    }
    vector<vector<int>>& x = cir_1.input_set;
    vector<vector<int>>& y = cir_2.input_set;
    vector<vector<int>>& z = cir_1.output_set;
    vector<vector<int>>& w = cir_2.output_set;
    // 获取新的set_size
    int set_size = get_set_num(x);
    random_set = generateRandomOnes(set_size); // 生成一组随机数

    CaDiCaL::Solver solver;
    solver_add_match(solver, cir_1, cir_2);
    set_value.clear();
    set_value.push_back(0);  
    for (int i = 0; i < set_size; i++) {
        for (auto u : x[i]) {
            solver.add(u * random_set[i]);
            solver.add(0);
        }
    }
    for (int i = 0; i < set_size; i++) {
        for (auto u : y[i]) {
            solver.add(u * random_set[i]);
            solver.add(0);
        }
    }
    int result_ = solver.solve();
    for (int i = 1; i <= cir_1.ports_n + cir_2.ports_n; i++) {
        set_value.push_back(solver.val(i));
    }

    for (int i = 0; i < random_set.size(); i++) {
        random_set[i] = -1 * random_set[i]; // 反转
        // 添加solver
        CaDiCaL::Solver expand_solver;
        solver_add_match(expand_solver, cir_1, cir_2);
        for (int j = 0; j < set_size; j++) {
            for (auto u : x[j]) {
                expand_solver.add(u * random_set[j]);
                expand_solver.add(0);
            }
        }
        for (int j = 0; j < set_size; j++) {
            for (auto u : y[j]) {
                expand_solver.add(u * random_set[j]);
                expand_solver.add(0);
            }
        }
        random_set[i] *= -1;
        
        int result = expand_solver.solve();
        if (result == 10) { // SAT
            for (int k = cir_1.inputs_n.size() + 1; k <= cir_1.inputs_n.size() + cir_1.outputs_n.size(); k++) {
                if (expand_solver.val(k) != set_value[k]) { // cir_1
                    nodes[k]->random_3_num++;
                }
                if (expand_solver.val(k + cir_1.ports_n) != set_value[k + cir_1.ports_n]) { // cir_2
                    nodes[k + cir_1.ports_n]->random_3_num++;
                }
            }
        }
        
    }

    // 划分集合
    for (int i = 0; i < z.size(); i++) {
        vector<int>& set = z[i];
        if (set.size() != 0 && set.size() != 1) {
            // 排序
            sort(set.begin(), set.end(), random_2_compare);
            // 分割set
            int index = 1;
            vector<int> insert_z = {};
            z.insert(z.begin() + i + (index++), insert_z); // 插入第一个元素
            z[i + index - 1].push_back(z[i][0]);
            for (int j = 1; j < z[i].size(); j++) {
                if (nodes[z[i][j]]->random_3_num == nodes[z[i][j - 1]]->random_3_num) { // 如果两个元素的random_3_num相同
                    z[i + index - 1].push_back(z[i][j]);
                }
                else {
                    vector<int> insert_z = {};
                    z.insert(z.begin() + i + (index++), insert_z); // 插入下一个元素
                    z[i + index - 1].push_back(z[i][j]);
                }
            }
            z.erase(z.begin() + i);
            i += index - 2; // 更新编号
        }
    }
    for (int i = 0; i < w.size(); i++) {
        vector<int>& set = w[i];
        if (set.size() != 0 && set.size() != 1) {
            // 排序
            sort(set.begin(), set.end(), random_2_compare);
            // 分割set
            int index = 1;
            vector<int> insert_w = {};
            w.insert(w.begin() + i + (index++), insert_w); // 插入第一个元素
            w[i + index - 1].push_back(w[i][0]);
            for (int j = 1; j < w[i].size(); j++) {
                if (nodes[w[i][j]]->random_3_num == nodes[w[i][j - 1]]->random_3_num) { // 如果两个元素的random_3_num相同
                    w[i + index - 1].push_back(w[i][j]);
                }
                else {
                    vector<int> insert_w = {};
                    w.insert(w.begin() + i + (index++), insert_w); // 插入下一个元素
                    w[i + index - 1].push_back(w[i][j]);
                }
            }
            w.erase(w.begin() + i);
            i += index - 2; // 更新编号
        }
    }
}

bool vector_compare(const vector<vector<int>>& vec1, const vector<vector<int>>& vec2) {
    if (vec1.size() != vec2.size()) {
        return false;
    }
    for (int i = 0; i < vec1.size(); i++) {
        // 检查每个子 vector 的大小是否相同
        if (vec1[i].size() != vec2[i].size()) {
            return false;
        }
    }
    return true;
}

void refine_partitin(CircuitData& cir_1, CircuitData& cir_2){
    bool changed = true;    
    while (changed) {
        vector<vector<int>>& x = cir_1.input_set;
        vector<vector<int>>& y = cir_2.input_set;
        vector<vector<int>>& z = cir_1.output_set;
        vector<vector<int>>& w = cir_2.output_set;

        CircuitData old_cir_1 = cir_1;
        CircuitData old_cir_2 = cir_2;

        for (auto u : nodes) { // 恢复原本的参数
            u->random_2_num = 0;
            u->random_3_num = 0;
        }
        update_set_index(x); update_set_index(y); update_set_index(z); update_set_index(w);
        update_support_set_index(x); update_support_set_index(y); update_support_set_index(z); update_support_set_index(w);
        support_set_refine(x);
        support_set_refine(y);
        support_set_refine(z);
        support_set_refine(w);
        update_set_index(x); update_set_index(y); update_set_index(z); update_set_index(w);
        update_support_set_index(x); update_support_set_index(y); update_support_set_index(z); update_support_set_index(w);      
        
        random_simulation_refine_1(cir_1, cir_2);
        random_simulation_refine_2(cir_1, cir_2);   
        random_simulation_refine_3(cir_1, cir_2);
        
        changed = (!(vector_compare(old_cir_1.input_set, cir_1.input_set)) ||
                  !(vector_compare(old_cir_2.input_set, cir_2.input_set)) ||
                  !(vector_compare(old_cir_1.output_set, cir_1.output_set)) ||
                  !(vector_compare(old_cir_2.output_set, cir_2.output_set)));
        
        // changed = (old_cir_1.input_set != cir_1.input_set) || (old_cir_2.input_set != cir_2.input_set) || (old_cir_1.output_set != cir_1.output_set) || (old_cir_2.output_set != cir_2.output_set);
    }
}

bool match_outputs(vector<vector<int>> x, vector<vector<int>> y_temp, CircuitData& cir_1, CircuitData& cir_2) { // 是值传递
    vector<vector<int>> z = cir_1.output_set;
    vector<vector<int>> w = cir_2.output_set;
    // 构建电路图
    int result = 10;
    int O = cir_1.outputs_n.size();
    vector<int> outputs_compare = {};
    for (int i = 1; i <= O; i++) {
        outputs_compare.push_back(cir_1.ports_n + cir_2.ports_n + i);
    }

    while (result == 10) { //SAT
        // solve
        CaDiCaL::Solver expand_solver_;
        solver_add_match(expand_solver_, cir_1, cir_2);
        for (int i = 0; i < cir_1.inputs_n.size(); i++) {
            if (x[i].size() == 0 && y_temp[i].size() == 0) {
                break;
            }
            else if (x[i].size() == 1 && y_temp[i].size() == 1) {
                expand_solver_.add(x[i][0]); expand_solver_.add(-1 * y_temp[i][0]); expand_solver_.add(0);
                expand_solver_.add(-1 * x[i][0]); expand_solver_.add(y_temp[i][0]); expand_solver_.add(0);
            }
            else {
                for (int j = 0; j < x[i].size(); j++) {
                    expand_solver_.add(x[i][j]); expand_solver_.add(-1 * y_temp[i][j]); expand_solver_.add(0);
                    expand_solver_.add(-1 * x[i][j]); expand_solver_.add(y_temp[i][j]); expand_solver_.add(0);
                    if (j + 1 < x[i].size()) {
                        expand_solver_.add(x[i][j]); expand_solver_.add(-1 * x[i][j + 1]); expand_solver_.add(0);
                        expand_solver_.add(-1 * x[i][j]); expand_solver_.add(x[i][j + 1]); expand_solver_.add(0);
                    }
                }
            }
        }
        int index = 0;
        for (int i = 0; i < z.size(); i++) {
            if (z[i].size() == 0 && w[i].size() == 0) {
                break;
            }
            for (int j = 0; j < z[i].size(); j++) {
                Gate g;
                g.type = Xor;
                g.output_n = outputs_compare[index++];
                g.inputs_n.push_back(z[i][j]);
                g.inputs_n.push_back(w[i][j]);
                solver_add_gate(expand_solver_, g);
            }
        }
        for (int i = 0; i < O; i++) {
            expand_solver_.add(outputs_compare[i]);
        }
        expand_solver_.add(0);
        result = expand_solver_.solve();

        // 分割
        if (result == 10) {
            for (int i = 0; i < z.size(); i++) {
                if (z[i].size() != 0 && z[i].size() != 1) {
                    vector<int> insert_z = {};
                    z.insert(z.begin() + i + 1, insert_z);
                    // 在后面插入一个数组
                    for (int j = 0; j < z[i].size(); j++) {
                        int u = z[i][j];
                        if (expand_solver_.val(u) < 0) {
                            z[i + 1].push_back(u);
                            z[i].erase(z[i].begin() + j); // 把负数从原数组中删除
                            j -= 1;
                        }
                    }
                    if (z[i + 1].size() != 0) { // 存在负数
                        if (z[i].size() == 0) {
                            z.erase(z.begin() + i);
                            i -= 1;
                        }
                        i += 1;
                    }
                    else { // 否则就删除
                        z.erase(z.begin() + i + 1);
                    }
                }
            }

            for (int i = 0; i < w.size(); i++) {
                if (w[i].size() != 0 && w[i].size() != 1) {
                    vector<int> insert_w = {};
                    w.insert(w.begin() + i + 1, insert_w);
                    // 在后面插入一个数组
                    for (int j = 0; j < w[i].size(); j++) {
                        int u = w[i][j];
                        if (expand_solver_.val(u) < 0) {
                            w[i + 1].push_back(u);
                            w[i].erase(w[i].begin() + j); // 把负数从原数组中删除
                            j -= 1;
                        }
                    }
                    if (w[i + 1].size() != 0) { // 存在负数
                        if (w[i].size() == 0) {
                            w.erase(w.begin() + i);
                            i -= 1;
                        }
                        i += 1;
                    }
                    else { // 否则就删除
                        w.erase(w.begin() + i + 1);
                    }
                }
            }

            if (get_set_num(z) != get_set_num(w)) {
                return false;
            }
            else {
                for (int i = 0; i < get_set_num(z); i++) {
                    if (z[i].size() == 0 && w[i].size() == 0) {
                        break;
                    }
                    if (z[i].size() != w[i].size()) {
                        return false;
                    }
                    else if (z[i].size() != 0 && w[i].size() != 0) {
                        if (expand_solver_.val(z[i][0]) * expand_solver_.val(w[i][0]) < 0) {
                            return false;
                        }
                    }
                }
            }
        }
    }
    // 如果匹配成功就更改原电路的值
    // cir_1.output_set = z;
    // cir_2.output_set = w;
    return true;
}

void search_cut(CircuitData& cir_1, CircuitData& cir_2) {
    vector<vector<int>>& x = cir_1.input_set;
    vector<vector<int>>& y = cir_2.input_set;
    vector<vector<int>> x_temp = x;
    fill(p.begin(), p.end(), 0);
    bool if_match_success = true;
    for (int i = 0; i < x_temp.size(); i++) {
        if (x_temp[i].size() != 0 && x_temp[i].size() != 1) {           
            vector<int>set_x = x_temp[i];
            // 选择第一个元素x_
            int x_ = set_x[0];
            // 分割x集合
            vector<int> insert_x = {};
            x_temp.insert(x_temp.begin() + i, insert_x); // 在原本的前面插入一个集合
            // 把x_放入新插入的集合中然后把后面集合的x_删除
            x_temp[i].push_back(x_);
            x_temp[i + 1].erase(x_temp[i + 1].begin());
            if_match_success = false;
            for (int k = p[x_]; k < set_x.size(); k++) {
                vector<vector<int>> y_temp = y;
                vector<int>set_y = y_temp[i];
                // 选择一个y
                int y_ = set_y[k];
                // 分割y集合
                vector<int> insert_y = {};
                y_temp.insert(y_temp.begin() + i, insert_y);
                y_temp[i].push_back(y_);
                y_temp[i + 1].erase(y_temp[i + 1].begin() + k);

                if (match_outputs(x_temp, y_temp, cir_1, cir_2) == true) {
                    y = y_temp;
                    p[x_] = k;
                    if_match_success = true;
                    break;
                }
            }

            if (!if_match_success) { // 如果没有匹配成功，返回上一步
                // 合并x
                x_temp[i + 1].insert(x_temp[i + 1].begin(), x_temp[i][0]);
                x_temp.erase(x_temp.begin() + i);
                i -= 1;

                x_temp[i + 1].insert(x_temp[i + 1].begin(), x_temp[i][0]);
                x_temp.erase(x_temp.begin() + i);

                for(int j = i + 2; j < cir_1.inputs_n.size(); j++){
                    p[j] = 0;
                }
                y[i + 1].insert(y[i + 1].begin() + p[x_temp[i][0]], y[i][0]);
                p[x_temp[i][0]] += 1;
                y.erase(y.begin() + i);
                i -= 1;
            }
            
        }
    }

    x = x_temp; 
}

bool stop = false;

void search(int i, vector<vector<int>>& x_temp, vector<vector<int>>& y_temp, CircuitData& cir_1, CircuitData& cir_2){
    if(stop){
        return;
    }
    if(x_temp[i].size() == 0){
        stop = true;
        return;
    }
    if(x_temp[i].size() == 1){
        search(i + 1, x_temp, y_temp, cir_1, cir_2);
    }
    if(x_temp[i].size() != 0 && x_temp[i].size() != 1){
        // 选择第一个元素x_
        int x_ = x_temp[i][0];
        // 分割x集合
        vector<int> insert_x = {};
        x_temp.insert(x_temp.begin() + i, insert_x); // 在原本的前面插入一个集合
        // 把x_放入新插入的集合中然后把后面集合的x_删除
        x_temp[i].push_back(x_);
        x_temp[i + 1].erase(x_temp[i + 1].begin());

        for (int k = 0; k < x_temp[i].size(); k++) {
            if(stop){
                return;
            }
            if(!visited[y_temp[i][k]]){
                // 选择一个y
                int y_ = y_temp[i][k];
                visited[y_] = true;
                // 分割y集合
                vector<int> insert_y = {};
                y_temp.insert(y_temp.begin() + i, insert_y);
                y_temp[i].push_back(y_);
                y_temp[i + 1].erase(y_temp[i + 1].begin() + k);

                if (match_outputs(x_temp, y_temp, cir_1, cir_2) == true) {
                    search(i + 1, x_temp, y_temp, cir_1, cir_2);
                    if(stop){
                        return;
                    }
                }
                // reverse change
                y_temp[i + 1].insert(y_temp[i + 1].begin() + k, y_temp[i][0]);
                y_temp.erase(y_temp.begin() + i);
                visited[y_] = false;
            }
        }
    }
}

// 递归函数，用于生成二维数组的所有排列
void permute(vector<vector<int>>& arr, int depth, vector<vector<int>>& allPermutations, vector<int>& current) {
    if (depth == arr.size()) {
        allPermutations.push_back(current);
        return;
    }

    // 对当前深度的数组进行排列
    sort(arr[depth].begin(), arr[depth].end());
    do {
        for (int num : arr[depth]) {
            current.push_back(num);
        }
        permute(arr, depth + 1, allPermutations, current);
        for (size_t i = 0; i < arr[depth].size(); ++i) {
            current.pop_back();
        }
    } while (next_permutation(arr[depth].begin(), arr[depth].end()));
}

// 初始化生成二维数组的所有排列
vector<vector<int>> generatePermutations(vector<vector<int>>& arr) {
    vector<vector<int>> allPermutations;
    vector<int> current;
    permute(arr, 0, allPermutations, current);
    return allPermutations;
}

// 将二维数组展平成一维数组的辅助函数
vector<int> flatten(const vector<vector<int>>& arr) {
    vector<int> flat;
    for (const auto& subArray : arr) {
        flat.insert(flat.end(), subArray.begin(), subArray.end());
    }
    return flat;
}

int brute_solver_add_match(CaDiCaL::Solver& solver, CircuitData cir_1, CircuitData cir_2, vector<int> x_per, vector<int> y_per, vector<int> z_per, vector<int> w_per){
    for(auto g:cir_1.gates){
        solver_add_gate(solver, g);
    }
    for(auto g:cir_2.gates){
        solver_add_gate(solver, g);
    }

    int I = x_per.size();
    int O = z_per.size();
    vector<int> outputs_compare = {};
    for(int i = 1; i <= O; i++){
        outputs_compare.push_back(cir_1.ports_n + cir_2.ports_n + i);
    }

    for(int i = 0; i < I; i++){
        Gate g;
        g.type = Buf;
        g.output_n = x_per[i];
        g.inputs_n.push_back(y_per[i]);
        solver_add_gate(solver, g);
    }
    for(int i = 0; i < O; i++){
        Gate g;
        g.type = Xor;
        g.output_n = outputs_compare[i];
        g.inputs_n.push_back(z_per[i]);
        g.inputs_n.push_back(w_per[i]);
        solver_add_gate(solver, g);
    }
    for(int i = 0; i < O; i++){
        solver.add(outputs_compare[i]);
    }
    solver.add(0);
    int result = solver.solve();
    if (result == 20) {
        // cout << "UNSAT" << endl;
        for (int i = 0; i < I; i++) {
            cout << cir_1.int_to_string[x_per[i]] << ' ' << cir_2.int_to_string[y_per[i]] << endl;
        }
        for (int i = 0; i < O; i++) {
            cout << cir_1.int_to_string[z_per[i]] << ' ' << cir_2.int_to_string[w_per[i]] << endl;
        }
    }
    return result;
}

int main(int argc, char ** argv) {
    string input_circuit_1 = argv[1];
    string input_circuit_2 = argv[2];
    ifstream in_1(input_circuit_1);
    ifstream in_2(input_circuit_2);

    Node* node = new Node;
    nodes.push_back(node);
    set_value.push_back(0);

    auto cir_1 = CircuitData::read_input(in_1, 0);
    auto cir_2 = CircuitData::read_input(in_2, cir_1.ports_n);
    visited.resize(cir_1.ports_n + cir_2.ports_n + 1);
    p.resize(cir_1.inputs_n.size() + 1);

    cir_1.find_support_set_all();
    cir_2.find_support_set_all();

    cir_1.build_support_set();
    cir_2.build_support_set();
    cir_1.initial_support_set_index();
    cir_2.initial_support_set_index();

    refine_partitin(cir_1, cir_2);
    fill(visited.begin(), visited.end(), false);
    search(0, cir_1.input_set, cir_2.input_set, cir_1, cir_2);

    // brute
    vector<vector<int>> x = cir_1.input_set;
    vector<vector<int>> z = cir_1.output_set;
    vector<vector<int>> y = cir_2.input_set;
    vector<vector<int>> w = cir_2.output_set;

    vector<int> x_per = flatten(x);
    vector<int> z_per = flatten(z);
    vector<vector<int>> y_pers = generatePermutations(y);
    vector<vector<int>> w_pers = generatePermutations(w);

    for (const auto& y_per: y_pers) {
        for (const auto& w_per : w_pers) {
            CaDiCaL::Solver solver;
            int result = brute_solver_add_match(solver, cir_1, cir_2, x_per, y_per, z_per, w_per);
            if(result == 20){
                return 0;
            }
        }
    }

    return 0;
}