#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<queue>
#include<algorithm>
#include<unordered_map>

using namespace std;

enum Type {
	INPUT, AND, INV
};

enum Pattern {
	NOT, NAND2, NOR2, AOI21, AOI22, NOT_NAND2 // 自己合成的与门，cost = 2
};

struct Node {
	Type type;
	string name;
	vector<int>node_from = {};
	vector<int>node_to = {};
	int in_deg = 0;
	int pattern = -1; // 匹配的模块
	vector<vector<int>> child_nodes = {}; // 匹配成功后的子节点数组，child_nodes[i]表示匹配模块i的子节点序号
	void Child_generate(int i); // 生成子节点的函数
};

struct Library {
	Pattern pattern;
	bool symmetric; // 是否对称
	unordered_map<int, Node*>lib_nodes = {}; // 包含的节点
	Library(const Pattern& p);
};

Library::Library(const Pattern& p) {
	pattern = p;
	symmetric = true;
	switch (p) {
	case NOT: {
		Node* n1 = new Node;
		n1->type = INV; n1->node_from.push_back(2);
		lib_nodes[1] = n1;
		Node* n2 = new Node;
		n2->type = INPUT;
		lib_nodes[2] = n2;
	}
		break;
	case NAND2: {
		Node* n1 = new Node;
		n1->type = INV; n1->node_from.push_back(2);
		lib_nodes[1] = n1;
		Node* n2 = new Node;
		n2->type = AND; n2->node_from.push_back(3); n2->node_from.push_back(4);
		lib_nodes[2] = n2;
		Node* n3 = new Node;
		n3->type = INPUT;
		lib_nodes[3] = n3;
		Node* n4 = new Node;
		n4->type = INPUT;
		lib_nodes[4] = n4;
	}
		break;
	case NOR2: {
		Node* n1 = new Node;
		n1->type = AND; n1->node_from.push_back(2); n1->node_from.push_back(3);
		lib_nodes[1] = n1;
		Node* n2 = new Node;
		n2->type = INV; n2->node_from.push_back(4);
		lib_nodes[2] = n2;
		Node* n3 = new Node;
		n3->type = INV; n3->node_from.push_back(5);
		lib_nodes[3] = n3;
		Node* n4 = new Node;
		n4->type = INPUT;
		lib_nodes[4] = n4;
		Node* n5 = new Node;
		n5->type = INPUT;
		lib_nodes[5] = n5;
	}
		break;
	case AOI21: {
		symmetric = false;
		Node* n1 = new Node;
		n1->type = AND; n1->node_from.push_back(2); n1->node_from.push_back(3);
		lib_nodes[1] = n1;
		Node* n2 = new Node;
		n2->type = INV; n2->node_from.push_back(4);
		lib_nodes[2] = n2;
		Node* n3 = new Node;
		n3->type = INV; n3->node_from.push_back(5);
		lib_nodes[3] = n3;
		Node* n4 = new Node;
		n4->type = AND; n4->node_from.push_back(6); n4->node_from.push_back(7);
		lib_nodes[4] = n4;
		Node* n5 = new Node;
		n5->type = INPUT;
		lib_nodes[5] = n5;
		Node* n6 = new Node;
		n6->type = INPUT;
		lib_nodes[6] = n6;
		Node* n7 = new Node;
		n7->type = INPUT;
		lib_nodes[7] = n7;
	}
		break;
	case AOI22: {
		Node* n1 = new Node;
		n1->type = AND; n1->node_from.push_back(2); n1->node_from.push_back(3);
		lib_nodes[1] = n1;
		Node* n2 = new Node;
		n2->type = INV; n2->node_from.push_back(4);
		lib_nodes[2] = n2;
		Node* n3 = new Node;
		n3->type = INV; n3->node_from.push_back(5);
		lib_nodes[3] = n3;
		Node* n4 = new Node;
		n4->type = AND; n4->node_from.push_back(6); n4->node_from.push_back(7);
		lib_nodes[4] = n4;
		Node* n5 = new Node;
		n5->type = AND; n5->node_from.push_back(8); n5->node_from.push_back(9);
		lib_nodes[5] = n5;
		Node* n6 = new Node;
		n6->type = INPUT;
		lib_nodes[6] = n6;
		Node* n7 = new Node;
		n7->type = INPUT;
		lib_nodes[7] = n7;
		Node* n8 = new Node;
		n8->type = INPUT;
		lib_nodes[8] = n8;
		Node* n9 = new Node;
		n9->type = INPUT;
		lib_nodes[9] = n9;
	}
		break;
	case NOT_NAND2: {
		Node* n1 = new Node;
		n1->type = AND; n1->node_from.push_back(2); n1->node_from.push_back(3);
		lib_nodes[1] = n1;
		Node* n2 = new Node;
		n2->type = INPUT;
		lib_nodes[2] = n2;
		Node* n3 = new Node;
		n3->type = INPUT;
		lib_nodes[3] = n3;
	}
		break;
	default:
		break;
	}
}

// 提前构造library数组
Library libs[6] = { Library(NOT), Library(NAND2), Library(NOR2), Library(AOI21), Library(AOI22), Library(NOT_NAND2) };
unordered_map<int, Node*>nodes = {};
vector<Node*> outputs = {};
vector<int>output_idxes = {};
vector<int>table = {};
vector<bool>visited = {};
queue<int> topo_nodes = {};
int p_cost[6] = { 1,1,1,1,1,2 }; // 记录每个门的cost数组
int cost = 0;

void Node::Child_generate(int i) {
	switch (i) {
	case NOT: {
		child_nodes[i].push_back(node_from[0]);
	}
		break;
	case NAND2: {
		int n2 = node_from[0];
		child_nodes[i].push_back(nodes[n2]->node_from[0]);
		child_nodes[i].push_back(nodes[n2]->node_from[1]);
	}
		break;
	case NOR2: {
		int n2 = node_from[0];
		int n3 = node_from[1];
		child_nodes[i].push_back(nodes[n2]->node_from[0]);
		child_nodes[i].push_back(nodes[n3]->node_from[0]);
	}
		break;
	case AOI21: {
		int n2 = node_from[0];
		int n3 = node_from[1];
		if (nodes[nodes[n2]->node_from[0]]->type == AND) {
			int n4 = nodes[n2]->node_from[0];
			child_nodes[i].push_back(nodes[n4]->node_from[0]);
			child_nodes[i].push_back(nodes[n4]->node_from[1]);
			child_nodes[i].push_back(nodes[n3]->node_from[0]);
		}
		else {
			int n4 = nodes[n3]->node_from[0];
			child_nodes[i].push_back(nodes[n4]->node_from[0]);
			child_nodes[i].push_back(nodes[n4]->node_from[1]);
			child_nodes[i].push_back(nodes[n2]->node_from[0]);
		}
	}
		break;
	case AOI22: {
		int n2 = node_from[0];
		int n3 = node_from[1];
		int n4 = nodes[n2]->node_from[0];
		int n5 = nodes[n3]->node_from[0];
		child_nodes[i].push_back(nodes[n4]->node_from[0]);
		child_nodes[i].push_back(nodes[n4]->node_from[1]);
		child_nodes[i].push_back(nodes[n5]->node_from[0]);
		child_nodes[i].push_back(nodes[n5]->node_from[1]);
	}
		break;
	case NOT_NAND2: {
		child_nodes[i].push_back(node_from[0]);
		child_nodes[i].push_back(node_from[1]);
	}
		break;
	default:
		break;
	}
}

bool match(int r, Library lib, int p) {
	if (lib.lib_nodes[p]->type == INPUT) { // 种类为INPUT
		return true;
	}
	if (lib.lib_nodes[p]->type == nodes[r]->type) {
		if (p == 1 || p != 1 && nodes[r]->node_to.size() == 1) {
			if (nodes[r]->type == AND) {
				if (lib.symmetric) {
					return ((match(nodes[r]->node_from[0], lib, lib.lib_nodes[p]->node_from[0]))
						&& (match(nodes[r]->node_from[1], lib, lib.lib_nodes[p]->node_from[1])));
				}
				else {
					return ((match(nodes[r]->node_from[0], lib, lib.lib_nodes[p]->node_from[0]))
						&& (match(nodes[r]->node_from[1], lib, lib.lib_nodes[p]->node_from[1])))
						|| ((match(nodes[r]->node_from[0], lib, lib.lib_nodes[p]->node_from[1]))
							&& (match(nodes[r]->node_from[1], lib, lib.lib_nodes[p]->node_from[0])));
				}
			}
			else if (nodes[r]->type == INV) {
				return (match(nodes[r]->node_from[0], lib, lib.lib_nodes[p]->node_from[0]));
			}
		}
	}
	return false;
}

void treecover() {
	for (auto& node : nodes) {
		node.second->child_nodes.resize(6);
		for (auto in : node.second->node_from) {
			nodes[in]->node_to.push_back(node.first);
		}
	} // 形成每个节点的node_to

	for (auto& node : nodes) {
		if (node.second->node_to.size() == 1) {
			int to = node.second->node_to[0];
			nodes[to]->in_deg += 1;
		}
	} // 拆分不同的树

	for (auto& node : nodes) {
		if (node.second->type == INPUT || node.second->in_deg == 0) {
			topo_nodes.push(node.first);
		}
	}

	while (!topo_nodes.empty()) {
		int now = topo_nodes.front();
		topo_nodes.pop();
		if (nodes[now]->node_to.size() == 1) {
			int to = nodes[now]->node_to[0];
			nodes[to]->in_deg--;
			if (nodes[to]->in_deg == 0) {
				topo_nodes.push(to);
			}
		}
		if (nodes[now]->type == INPUT) {
			table[now] = 0;
			continue;
		}

		for (int i = 0; i <= 5; i++) {
			if (match(now, libs[i], 1)) {
				nodes[now]->Child_generate(i);
				int mincost = p_cost[i];
				for (auto& x : nodes[now]->child_nodes[i]) {
					if (nodes[x]->node_to.size() == 1) {
						mincost += table[x];
					}
				}
				if (mincost < table[now]) {
					table[now] = mincost;
					nodes[now]->pattern = i;
				}
			}
		}
	} // 按照拓扑排序的顺序计算拆分后每个节点的mincost，并形成匹配的模块
}

void pattern_output(ofstream& blif_file, int r) {
	blif_file << ".names ";
	for (auto x : nodes[r]->child_nodes[nodes[r]->pattern]) {
		blif_file << nodes[x]->name << " ";
	}
	blif_file << nodes[r]->name << endl;
	if (r == 1392) {
		blif_file << endl;
	}
	switch (nodes[r]->pattern) {
	case NOT: {
		blif_file << "0 1" << endl;
	}
		break;
	case NAND2: {
		blif_file << "0- 1" << endl;
		blif_file << "-0 1" << endl;
	}
		break;
	case NOR2: {
		blif_file << "00 1" << endl;
	}
		break;
	case AOI21: {
		blif_file << "0-0 1" << endl;
		blif_file << "-00 1" << endl;
	}
		break;
	case AOI22: {
		blif_file << "0-0- 1" << endl;
		blif_file << "-00- 1" << endl;
		blif_file << "0--0 1" << endl;
		blif_file << "-0-0 1" << endl;
	}
		break;
	case NOT_NAND2: {
		blif_file << "11 1" << endl;
	}
		break;
	default:
		break;
	}
}

int node_output(ofstream& blif_file, int r) {
	int cost_temp = 0;
	if (visited[r])return 0;
	visited[r] = 1;
	if (nodes[r]->pattern != -1) {
		cost_temp += p_cost[nodes[r]->pattern];
		pattern_output(blif_file, r);
		for (auto& node : nodes[r]->child_nodes[nodes[r]->pattern]) {
			cost_temp += node_output(blif_file, node);
		}
	}
	return cost_temp;
} // 逐一输出模块

void Output(ofstream& blif_file, int I) {
	blif_file << ".model top" << endl;
	blif_file << ".inputs ";
	for (int i = 1; i <= I; i++) {
		blif_file << nodes[2 * i]->name << " ";
	}
	blif_file << endl;

	blif_file << ".outputs ";
	for (const auto& output_node : outputs) {
		blif_file << output_node->name << " ";
	}
	blif_file << endl;

	int cost = 0;
	for (auto out_idx : output_idxes)
	{
		if (out_idx == 0 || out_idx == 1 || nodes[out_idx]->type == INPUT) {
			continue;
		}
		cost += node_output(blif_file, out_idx);
	}

	for (auto& output_node : outputs) {
		if (output_node->node_from[0] != 0 && output_node->node_from[0] != 1) {
			blif_file << ".names " << nodes[output_node->node_from[0]]->name << " " << output_node->name << endl;
			blif_file << "1 1" << endl;
		}
		else {
			blif_file << ".names " << output_node->name << endl;
			blif_file << output_node->node_from[0] << endl;
		}
	}

	blif_file << ".end" << endl;
	cout << "cost of the mapping is " << cost << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: ./techmap <input_path> <output_path>" << endl;
        return 1;
    }

    string input_path = argv[1];
    string output_path = argv[2];
    ifstream aag_file(input_path);
    ofstream blif_file(output_path);

	string header = {};
	aag_file >> header;

	int M = 0, I = 0, L = 0, O = 0, A = 0;
	aag_file >> M >> I >> L >> O >> A;

	table.resize(2 * M + 2);
	visited.resize(2 * M + 2);
	fill(table.begin(), table.end(), 0x3f3f3f3f);
	fill(visited.begin(), visited.end(), 0);

	int input_idx = 0;
	for (int i = 1; i <= I; i++) {
		aag_file >> input_idx;
		Node* input = new Node;
		input->type = INPUT;
		nodes[2 * i] = input;
	}

	int output_idx = 0;
	for (int i = 0; i < O; i++) {
		aag_file >> output_idx;
		output_idxes.push_back(output_idx);

		if (output_idx % 2 == 1) { // 如果有非节点，非节点为输出节点
			Node* inv = new Node;
			inv->type = INV;
			inv->node_from.push_back(output_idx - 1);
			nodes[output_idx] = inv;
		}
	}

	int a = 0, b = 0, c = 0;
	for (int i = I + 1; i <= M; i++) {
		aag_file >> a >> b >> c;
		if (b > a || c > a) {
			blif_file << "error" << endl;
		}
		Node* And = new Node;
		And->type = AND;
		nodes[2 * i] = And;
		nodes[2 * i]->node_from.push_back(b);
		if (b % 2 == 1 && nodes.find(b) == nodes.end()) {
			// 如果有非节点且还未加入nodes中
			Node* inv = new Node;
			inv->type = INV;
			inv->node_from.push_back(b - 1);
			nodes[b] = inv;
		}

		nodes[2 * i]->node_from.push_back(c);
		if (c % 2 == 1 && nodes.find(c) == nodes.end()) {
			Node* inv = new Node;
			inv->type = INV;
			inv->node_from.push_back(c - 1);
			nodes[c] = inv;
		}
	}

	string name = {}, tmp_str = {};
	for (int i = 1; i <= I; i++) {
		aag_file >> tmp_str >> name;
		nodes[2 * i]->name = name;
	}

	for (auto i : output_idxes) {
		aag_file >> tmp_str >> name;
		Node* output = new Node;
		output->name = name;
		output->node_from.push_back(i);
		outputs.push_back(output);
	}

	for (auto& node : nodes) {
		if (node.second->name.empty()) {
			node.second->name = "n" + to_string(node.first);
			// 节点name为其在nodes中的序号
		}
	}

	treecover();
	Output(blif_file, I);

	return 0;
}