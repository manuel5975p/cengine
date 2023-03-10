#include "thread_pool.hpp"
#include <iostream>
#include <vector>
#include <benchmark.hpp>
#include <cmath>
#include <chrono>
#include <memory>
#include <iterator>
#include <set>
#include <mutex>
#include "search.hpp"
//cb::ThreadPool pool(24);
std::mutex abmutex;

struct position_tree_node{
    Position p;
    int alpha;
    int beta;
    int depth;
    position_tree_node* parent;
    std::atomic<size_t> evaluated_count;
    bool pruned;
    complete_move move_justbefore;
    position_tree_node(Position _p, int a, int b, int d) : p(_p), alpha(a), beta(b), depth(d), parent(nullptr), evaluated_count(0), pruned(false){}
    std::vector<std::unique_ptr<position_tree_node>> children;
    template<bool locked = true>
    void raise_alpha_from_below(int na){
        //std::cout << "Depth " << depth << " alpha " << na << std::endl;
        if constexpr(locked)
            std::lock_guard<std::mutex> lock(abmutex);
        evaluated_count++;
        if(na > alpha){
            alpha = na;
            for(auto& c : children){
                c->lower_beta<false>(-alpha);
            }
            if(alpha >= beta){
                parent->raise_alpha_from_below<false>(-alpha);
                mark_pruned();
            }
        }
        if(evaluated_count.operator std::size_t() >= children.size() && parent){
            parent->raise_alpha_from_below<false>(-alpha);
        }
    }
    template<bool locked = true>
    void raise_alpha(int na){
        if constexpr(locked)
            std::lock_guard<std::mutex> lock(abmutex);
        if(na > alpha){
            alpha = na;
            for(auto& c : children){
                c->lower_beta<false>(-alpha);
            }
            if(alpha >= beta){
                //parent->raise_alpha_from_below<false>(-alpha);
                //mark_pruned();
            }
        }
    }
    template<bool locked = true>
    void lower_beta(int nb){
        if constexpr(locked)
            std::lock_guard<std::mutex> lock(abmutex);
        if(nb < beta){
            beta = nb;
            for(auto& c : children){
                c->raise_alpha<false>(-beta);
            }
            if(alpha >= beta){
                //parent->raise_alpha_from_below<false>(-alpha);
                //mark_pruned();
            }
        }
    }
    private:
    void mark_pruned(){
        pruned = true;
        for(auto& child : children){
            child->mark_pruned();
        }
    }
};

int negamax_multithreaded(Position& pos, int depth, int alpha, int beta, search_state& state){
    
}
std::unique_ptr<position_tree_node> build_tree(Position p, int depth, int alpha, int beta, std::vector<position_tree_node*>& collection){
    if(depth == 0){
        auto up = std::make_unique<position_tree_node>(p, alpha, beta, depth);
        collection.push_back(up.get());
        return up;
    }
    auto legal = p.generate_legal(p.at_move);
    std::vector<std::pair<complete_move, int>> guesses(legal.size());
    for(size_t i = 0;i < legal.size();i++){
        Position pclone(p);
        pclone.apply_move_checked(legal[i]);
        //guesses[i] = {legal[i], negamax_serial(pclone, 1, alpha, beta)};
    }
    std::sort(guesses.begin(), guesses.end(), [](const std::pair<complete_move, int>& a, const std::pair<complete_move, int>& b){
        return a.second > b.second;
    });
    for(size_t i = 0;i < guesses.size();i++){
        //std::cout << legal[i].to_string() << "\n";
        legal[i] = guesses[i].first;
        
    }
    std::unique_ptr<position_tree_node> this_node = std::make_unique<position_tree_node>(p, alpha, beta, depth);
    this_node->children.reserve(legal.size());
    for(const auto& move : legal){
        Position pclone(p);
        bool moved = pclone.apply_move_checked(move);
        assert(moved);
        //std::cout << move.to_short_notation() << "\n";
        std::unique_ptr<position_tree_node> subtree = build_tree(pclone, depth - 1, alpha, beta, collection);
        subtree->parent = this_node.get();
        subtree->move_justbefore = move;
        this_node->children.push_back(std::move(subtree));
    }
    //collection.push_back(this_node.get());
    return this_node;
}
void negamax_node(position_tree_node* node){
    if(node->pruned){
        //node->parent->raise_alpha_from_below(-node->alpha);
        return;
    }
    const Position& pos = node->p;
    auto legal = pos.generate_legal(pos.at_move);
    int depth = 4;
    for (size_t i = 0;i < legal.size();i++) {
        Position pclone(pos);
        pclone.apply_move_checked(legal[i]);
        if(node->alpha >= node->beta)break;
        //int wert = -negamax_serial(pclone, depth - 1, -node->beta, -node->alpha);
        int wert = 0;
        if(node->pruned)return;
        if (wert > node->alpha) {
            //node->alpha = wert;
            node->raise_alpha(wert);
            if (node->alpha >= node->beta){
                break;
            }
        }
    }
    if(node->pruned)return;
    //std::cout << pos.to_string() << std::endl;
    node->parent->raise_alpha_from_below(-node->alpha);
}

int maiin(){
    Bitboards::init();
    Position shitass("rnb1kb1r/1p1p1pp1/pqp5/4p2p/4P1K1/2N5/PPP2PPP/R1BQ1BNR w kq h6 0 1");
    std::cout << shitass.spec_mem.ep << "\n";
    auto shitloud = shitass.generate_new<WHITE>();
    for(auto& mv : shitloud){
        std::cout << mv.to_short_notation() << "\n";
    }
    //return 0;
    Position p;//("rnb1kbnr/pp1p1ppp/1qp5/4p3/4P3/8/PPP1KPPP/RNBQ1BNR b kq - 1 2");
    turbo_search_state rs;
    rs.count = 0;
    rs.depth = 2;
    int salpha = -100000, sbeta = 100000;
    int64_t turboval = negamax_serial(p, rs.depth, salpha, sbeta, rs);
    rs.count = 0;
    rs.depth = 4;
    turboval = negamax_serial(p, rs.depth, salpha, sbeta, rs);
    rs.count = 0;
    rs.depth = 6;
    turboval = negamax_serial(p, rs.depth, salpha, sbeta, rs);
    rs.count = 0;
    rs.depth = 8;
    turboval = negamax_serial(p, rs.depth, salpha, sbeta, rs);
    std::cout << "turboval: " << turboval << std::endl;
    std::cout << rs.bestmove.to_short_notation() << "\n";
    std::cout << rs.count << " nodes\n";
    return 0;
}
int main2() {
    Bitboards::init();
    Position p("3r1rk1/pp4b1/2p4p/2q3p1/4B1bN/2P5/PPQ5/2K1R2R w - - 0 1");
    //std::cout << p.fen() << "\n";
    //std::cout << perft(p, 6) << "\n";
    //return 0;
    ////print(LineBetween[0][36]);
    //auto pmoves = p.generate_loud<WHITE>();
    //for(auto& move : pmoves){
    //    std::cout << move.to_short_notation() << "\n";
    //}
    //return 0;
    search_state state;
    state.count = 0;
    state.depth = 4;
    int salpha = -100000, sbeta = 100000;
    turbo_search_state rs;
    rs.count = 0;
    rs.depth = 6;
    bool flag = true;
    std::thread fr([&rs, &flag]{
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        while(flag){
            std::cout << std::to_string(rs.count) + "\n";
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    auto t1 = _bm_nanoTime();
    int turboval = negamax_serial(p, rs.depth, salpha, sbeta, rs);
    int convval = negamax(p, rs.depth, salpha, sbeta, state);
    auto t2 = _bm_nanoTime();
    flag = false;
    std::cout << "convval: " << convval << std::endl;
    std::cout << "turboval: " << turboval << std::endl;
    std::cout << "Time required: " << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    std::cout << rs.bestmove.to_short_notation() << "\n";
    std::cout << rs.count << " nodes\n";
    fr.join();
    return 0;
    std::vector<position_tree_node*> leaves;
    auto up = build_tree(p, state.depth, salpha, sbeta, leaves);
    //xoshiro_256 gen;
    //std::shuffle(leaves.begin(), leaves.end(), gen);
    //std::cout << up->alpha << "\n";
    t1 = _bm_nanoTime();
    //std::vector<std::pair<position_tree_node*, int>> guesses(leaves.size());
    //#pragma omp parallel for schedule(guided)
    //for(size_t i = 0;i < leaves.size();i++){
    //    guesses[i] = {leaves[i], negamax_serial(leaves[i]->p, 2, salpha, sbeta)};
    //}
    //std::sort(guesses.begin(), guesses.end(), [](const std::pair<position_tree_node*, int>& a, const std::pair<position_tree_node*, int>& b){
    //    return a.second > b.second;
    //});
    //for(size_t i = 0;i < leaves.size();i++){
    //    leaves[i] = guesses[i].first;
    //}
    #pragma omp parallel for schedule(guided)
    for(auto& leaf : leaves){
        //std::cout << leaf->p.to_string() << "\n";
        negamax_node(leaf);
    }
    t2 = _bm_nanoTime();
    size_t maxindex = -1;
    int malpha = -100000;
    for(size_t i = 0;i < up->children.size();i++){
        if(-up->children[i]->alpha > malpha){
            malpha = -up->children[i]->alpha;
            maxindex = i;
            //break;
        }
    }
    std::cout << up->children[maxindex]->move_justbefore.to_short_notation() << "\n";
    std::cout << up->alpha << "\n";
    std::cout << up->evaluated_count << " evaluated, ";
    std::cout << "Time required: " << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    std::cout << up->children.size() << " children" << std::endl;
    
    
    //std::cout << state.bestmove.to_short_notation() << "\n";
    //std::cout << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    //std::cout << state.count << "\n";
    return 0;
}
