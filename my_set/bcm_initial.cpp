#include <cmath>
#include <vector>
#include <cstdio>
#include <fstream>
#include <istream>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <algorithm>
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
};

struct CircuitData {
    vector<string> ports;
    vector<string> inputs;
    vector<string> outputs;
    vector<string> wires;
    vector<Gate>   gates;
    static CircuitData read_input(istream & in);
    void print(ostream & out);
};

CircuitData CircuitData::read_input(istream & in) {
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
    while(in >> token, token != ";") {
        if (token == ",") continue;
        inputs.push_back(token);
    }
    //    output   o1 , o2 , ...
    in >> token;
    vector<string> outputs;
    while(in >> token, token != ";") {
        if (token == ",") continue;
        outputs.push_back(token);
    }
    //    wire     w1 , w2 , ...
    //    or maybe no wire
    in >> token;
    vector<string> wires;
    if(token == "wire") {
        while(in >> token, token != ";") {
            if (token == ",") continue;
            wires.push_back(token);
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
        while(in >> token, token != ")") {
            if (token == ",") continue;
            inputs.push_back(token);
        }
        //     ;
        in >> token;
        auto gate = Gate {type, name, inputs, output};
        gates.push_back(gate);
        in >> token;
    }
    return CircuitData {ports, inputs, outputs, wires, gates};
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

int main(int argc, char ** argv) {
    string input_circuit_1 = argv[1];
    // string input_circuit_2 = argv[2];
    ifstream in(input_circuit_1);
    auto cir = CircuitData::read_input(in);
    cir.print(cout);
    CaDiCaL::Solver solver;
    solver.add(1); solver.add(2); solver.add(0);
    solver.add(1); solver.add(3); solver.add(0);
    solver.add(-2); solver.add(-3); solver.add(0);
    solver.add(-1); solver.add(-2); solver.add(0);
    int result = solver.solve();
    if(result == 0) {
        cout << "UNKNOWN" << endl;
    } 
    else if (result == 10) {
        cout << "SAT" << endl;
        for(int i = 1; i<= 3; i++) {
            cout << solver.val(i) << " ";
        }
    }
    else if (result == 20) {
        cout << "UNSAT" << endl;
    }
    return 0;
}
