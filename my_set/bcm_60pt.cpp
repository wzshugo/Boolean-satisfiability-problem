#include <cmath>
#include <vector>
#include <cstdio>
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

GateType gate_from_str(const string & s) {
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
    switch(g) {
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
    vector<string> inputs;
    string output;

    vector<int> inputs_n;
    int output_n;
};

struct CircuitData {
    vector<string> ports;
    vector<string> inputs;
    vector<string> outputs;
    vector<string> wires;
    vector<Gate>   gates;

    unordered_map<string, int> string_to_int;
    unordered_map<int, string> int_to_string;
    vector<int> inputs_n;
    vector<int> outputs_n;

    int ports_n = 0;

    static CircuitData read_input(istream & in, int cir_n);
    void print(ostream & out);
};

CircuitData CircuitData::read_input(istream & in, int cir_n) {
    unordered_map<string, int> string_to_int;
    unordered_map<int, string> int_to_string;
    int index = 1;
    string token;
    //     module   top      (
    in >> token >> token >> token;
    vector<string> ports;
    while(in >> token, token != ")") {
        if (token == ",") continue;
        ports.push_back(token);
    }
    //      ;
    in >> token;
    //    input    a1 , a2 , ...
    in >> token;
    vector<string> inputs;
    vector<int> inputs_n;
    while(in >> token, token != ";") {
        if (token == ",") continue;
        inputs.push_back(token);
        inputs_n.push_back(cir_n + (index++));
        string_to_int[token] = cir_n + index - 1;
        int_to_string[cir_n + index - 1] = token;
    }
    //    output   o1 , o2 , ...
    in >> token;
    vector<string> outputs;
    vector<int> outputs_n;
    while(in >> token, token != ";") {
        if (token == ",") continue;
        outputs.push_back(token);
        outputs_n.push_back(cir_n + (index++));
        string_to_int[token] = cir_n + index - 1;
        int_to_string[cir_n + index - 1] = token;
    }
    //    wire     w1 , w2 , ...
    //    or maybe no wire
    in >> token;
    vector<string> wires;
    if(token == "wire") {
        while(in >> token, token != ";") {
            if (token == ",") continue;
            wires.push_back(token);
            string_to_int[token] = cir_n + (index++);
            int_to_string[cir_n + index - 1] = token;
        }
        in >> token;
    }
    vector<Gate> gates;
    while(token != "endmodule") {
        auto type = gate_from_str(token);
        string name;
        string output;
        //    g1        (        wi        ,
        in >> name >> token >> output >> token;
        vector<string> inputs;
        vector<int> inputs_n;
        int output_n = string_to_int[output];
        while(in >> token, token != ")") {
            if (token == ",") continue;
            inputs.push_back(token);
            inputs_n.push_back(string_to_int[token]);
        }
        //     ;
        in >> token;
        auto gate = Gate {type, name, inputs, output, inputs_n, output_n};
        gates.push_back(gate);
        in >> token;
    }
    int ports_n = inputs.size() + outputs.size() + wires.size();
    return CircuitData {ports, inputs, outputs, wires, gates, string_to_int, int_to_string, inputs_n, outputs_n, ports_n};
}

void CircuitData::print(ostream & out) {
    out << "module top ( ";
    auto printCommaInterleave = [&out](const vector<string> & v) {
        for(size_t i = 0; i < v.size(); i++) {
            if(i) out << " , ";
            out << v[i];
        }
    };
    printCommaInterleave(ports);
    out << " ) ;\n";
    out << "  input ";
    printCommaInterleave(inputs);
    out << " ;\n";
    out << "  output ";
    printCommaInterleave(outputs);
    out << " ;\n";
    if(wires.size()) {
        out << "  wire ";
        printCommaInterleave(wires);
        out << " ;\n";
    }
    for(auto g: gates) {
        out << "  " << gate_to_str(g.type) << " ( ";
        out << g.output << " , ";
        printCommaInterleave(g.inputs);
        out << " ) ;\n";
    }
    out << "endmodule\n";
}

void solver_add_gate(CaDiCaL::Solver& solver, const Gate& g){
    switch(g.type){
        case Not:{
            solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(0);
        }    
            break;
        case Buf:{
            solver.add(-1*g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
            solver.add(g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(0);
        }
            break;
        case Nand:{
            solver.add(-1*g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(-1*g.inputs_n[1]); solver.add(0);
            solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
            solver.add(g.output_n); solver.add(g.inputs_n[1]); solver.add(0);
        }
            break;
        case And:{
            solver.add(g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(-1*g.inputs_n[1]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(g.inputs_n[0]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(g.inputs_n[1]); solver.add(0);
        }
            break;
        case Nor:{
            solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(-1*g.inputs_n[1]); solver.add(0);
        }
            break;
        case Or:{
            solver.add(-1*g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
            solver.add(g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(0);
            solver.add(g.output_n); solver.add(-1*g.inputs_n[1]); solver.add(0);
        }
            break;
        case Xnor:{
            solver.add(-1*g.output_n); solver.add(g.inputs_n[0]); solver.add(-1*g.inputs_n[1]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
            solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
            solver.add(g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(-1*g.inputs_n[1]); solver.add(0);
        }
            break;
        case Xor:{
            solver.add(g.output_n); solver.add(g.inputs_n[0]); solver.add(-1*g.inputs_n[1]); solver.add(0);
            solver.add(g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(g.inputs_n[0]); solver.add(g.inputs_n[1]); solver.add(0);
            solver.add(-1*g.output_n); solver.add(-1*g.inputs_n[0]); solver.add(-1*g.inputs_n[1]); solver.add(0);
        }
            break;
        default:
            break;
    };
    return ;
}

int solver_add_match(CaDiCaL::Solver& solver, CircuitData cir_1, CircuitData cir_2){
    for(auto g:cir_1.gates){
        solver_add_gate(solver, g);
    }
    for(auto g:cir_2.gates){
        solver_add_gate(solver, g);
    }

    int I = cir_1.inputs.size();
    int O = cir_1.outputs.size();
    vector<int> outputs_compare = {};
    for(int i = 1; i <= O; i++){
        outputs_compare.push_back(cir_1.ports_n + cir_2.ports_n + i);
    }

    for(int i = 0; i < I; i++){
        Gate g;
        g.type = Buf;
        g.output_n = cir_1.inputs_n[i];
        g.inputs_n.push_back(cir_2.inputs_n[i]);
        solver_add_gate(solver, g);
    }
    for(int i = 0; i < O; i++){
        Gate g;
        g.type = Xor;
        g.output_n = outputs_compare[i];
        g.inputs_n.push_back(cir_1.outputs_n[i]);
        g.inputs_n.push_back(cir_2.outputs_n[i]);
        solver_add_gate(solver, g);
    }
    for(int i = 0; i < O; i++){
        solver.add(outputs_compare[i]);
    }
    solver.add(0);
    int result = solver.solve();
    if (result == 20) {
        // cout << "UNSAT" << endl;
        for(int i = 0; i < I; i++){
            cout << cir_1.int_to_string[cir_1.inputs_n[i]] << ' ' << cir_2.int_to_string[cir_2.inputs_n[i]] << endl;
        }
        for(int i = 0; i < O; i++){
            cout << cir_1.int_to_string[cir_1.outputs_n[i]] << ' ' << cir_2.int_to_string[cir_2.outputs_n[i]] << endl;
        }
    }
    /*else if(result == 0) {
        cout << "UNKNOWN" << endl;
    } 
    else if (result == 10) {
        cout << "SAT" << endl;
        for(int i = 1; i <= cir_1.ports_n + cir_2.ports_n + O; i++) {
            cout << solver.val(i) << " ";
        }
        cout << endl;
    }
    */
    return result;
}

int main(int argc, char ** argv) {
    string input_circuit_1 = argv[1];
    string input_circuit_2 = argv[2];
    ifstream in_1(input_circuit_1);
    ifstream in_2(input_circuit_2);
    auto cir_1 = CircuitData::read_input(in_1, 0);
    auto cir_2 = CircuitData::read_input(in_2, cir_1.ports_n);
    // cir_1.print(cout);
    // cir_2.print(cout);
    
    CaDiCaL::Solver solver;
    int result = solver_add_match(solver, cir_1, cir_2);
    if(result == 20){
        return 0;
    }
    while (next_permutation(cir_2.outputs_n.begin(), cir_2.outputs_n.end())) {
        CaDiCaL::Solver solver;
        int result = solver_add_match(solver, cir_1, cir_2);
        if(result == 20){
            return 0;
        }  
    }

    while (next_permutation(cir_2.inputs_n.begin(), cir_2.inputs_n.end())) {
        CaDiCaL::Solver solver;
        int result = solver_add_match(solver, cir_1, cir_2);
        if(result == 20){
            return 0;
        }
        while (next_permutation(cir_2.outputs_n.begin(), cir_2.outputs_n.end())) {
                CaDiCaL::Solver solver;
                int result = solver_add_match(solver, cir_1, cir_2);
                if(result == 20){
                    return 0;
                }    
        }
    }
    
    return 0;
}