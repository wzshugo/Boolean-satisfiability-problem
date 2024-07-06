#include <cryptominisat5/cryptominisat.h>
#include <vector>
using std::vector, std::cout, std::cin, std::endl;
using namespace CMSat;

int main()
{
    SATSolver solver;
    vector<Lit> clause;
    int n;
    cin >> n;
    int n_vars = n*n;
    solver.new_vars(n*n);

    // 每一行至少有一个queen
    for(int i = 0; i < n; i++){
        clause.clear();
        for(int j = 0; j < n; j++){
            clause.push_back(Lit(i*n + j, false));
        }
        solver.add_clause(clause);
    }

    // 每一行最多有一个queen
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            for(int k = j + 1; k < n; k++){
                clause.clear();
                clause.push_back(Lit(i*n + j, true));
                clause.push_back(Lit(i*n + k, true));
                solver.add_clause(clause);
            }
        }
    }

    // 每一列只能有一个queen
    for(int j = 0; j < n; j++){
        for(int i = 0; i < n; i++){
            for(int k = i + 1; k < n; k++){
                clause.clear();
                clause.push_back(Lit(i*n + j, true));
                clause.push_back(Lit(k*n + j, true));
                solver.add_clause(clause);
            }
        }
    }

    // 每一个对角线只能有一个queen
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            for(int k = 1; k < n; k++){
                if(i + k < n && j + k < n){
                    clause.clear();
                    clause.push_back(Lit(i*n + j, true));
                    clause.push_back(Lit((i + k)*n + j + k, true));
                    solver.add_clause(clause);
                }
                if(i + k < n && j - k >= 0 ){
                    clause.clear();
                    clause.push_back(Lit(i*n + j, true));
                    clause.push_back(Lit((i + k)*n + j - k, true));
                    solver.add_clause(clause);
                }
            }
        }
    }

    lbool ret = solver.solve();
    //puts(ret == l_True ? "s SATISFIABLE" : "s UNSATISFIABLE");
    if (ret == l_True) {
        const auto& model = solver.get_model();
        for(int i = 0; i < n; i++){
            for(int j = 0; j < n; j++){
                if(model[i*n + j] == l_True){
                    cout << '#';
                }
                else{
                    cout << ".";
                }
            }
            cout << endl;
        }
    }
    return 0;
}
