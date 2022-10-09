#include "thread_pool.hpp"
#include <iostream>
#include <vector>
#include <benchmark.hpp>
#include <cmath>
#include <chrono>
#include <memory>
#include <mutex>
#include "search.hpp"
cb::ThreadPool pool(24);
std::mutex abmutex;
struct position_tree_node{
    Position p;
    int alpha;
    int beta;
    int depth;
    position_tree_node* parent;
    std::atomic<size_t> evaluated_count;
    bool pruned;
    position_tree_node(Position _p, int a, int b, int d) : p(_p), alpha(a), beta(b), depth(d), parent(nullptr), evaluated_count(0), pruned(false){}
    std::vector<std::unique_ptr<position_tree_node>> children;
    template<bool locked = true>
    void raise_alpha_from_below(int na){
        if constexpr(locked)
            std::lock_guard<std::mutex> lock(abmutex);
        evaluated_count++;
        if(na > alpha){
            alpha = na;
            for(auto& c : children){
                c->lower_beta<false>(-alpha);
            }
            if(alpha >= beta){
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
                mark_pruned();
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
                mark_pruned();
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
int negamax_serial(Position& pos, int depth, int alpha, int beta){
    int maxWert = alpha;
    auto legal = pos.generate_legal(pos.at_move);
    if(depth == 0){
        return evaluate(pos);
    }
    for (size_t i = 0;i < legal.size();i++) {
        Position pclone(pos);
        pclone.apply_move_checked(legal[i]);
        int wert = -negamax_serial(pclone, depth - 1, -beta, -maxWert);
        if (wert > maxWert) {
            maxWert = wert;
            if (maxWert >= beta){
                return maxWert;
            }
        }
    }
    return maxWert;
}
int negamax_multithreaded(Position& pos, int depth, int alpha, int beta, search_state& state){
    
}
std::unique_ptr<position_tree_node> build_tree(Position p, int depth, int alpha, int beta, std::vector<position_tree_node*>& collection){
    if(depth == 0){
        auto up = std::make_unique<position_tree_node>(p, alpha, beta, depth);
        collection.push_back(up.get());
        return up;
    }
    auto legal = p.generate_legal(p.at_move);
    std::unique_ptr<position_tree_node> this_node = std::make_unique<position_tree_node>(p, alpha, beta, depth);
    this_node->children.reserve(legal.size());
    for(const auto& move : legal){
        Position pclone(p);
        pclone.apply_move_checked(move);
        std::unique_ptr<position_tree_node> subtree = build_tree(pclone, depth - 1, alpha, beta, collection);
        subtree->parent = this_node.get();
        this_node->children.push_back(std::move(subtree));
    }
    //collection.push_back(this_node.get());
    return this_node;
}
void negamax_node(position_tree_node* node){
    const Position& pos = node->p;
    auto legal = pos.generate_legal(pos.at_move);
    int depth = 4;
    for (size_t i = 0;i < legal.size();i++) {
        Position pclone(pos);
        pclone.apply_move_checked(legal[i]);
        int wert = -negamax_serial(pclone, depth - 1, -node->beta, -node->alpha);
        if (wert > node->alpha) {
            node->raise_alpha(wert);
            if (node->alpha >= node->beta){
                break;
            }
        }
    }
    node->parent->raise_alpha_from_below(-node->alpha);
}
int main() {
    Bitboards::init();
    Position p("rnb1kbnr/pppp1ppp/4p3/8/4P3/3P4/PPP2PPP/RNB1KBNR w KQkq - 1 3");
    search_state state;
    state.count = 0;
    state.depth = 2;
    int salpha = -100000, sbeta = 100000;
    auto t1 = _bm_nanoTime();
    int realval = negamax_serial(p, 6, salpha, sbeta);
    auto t2 = _bm_nanoTime();
    std::cout << "Akchual value: " << realval << std::endl;
    std::cout << "Time required: " << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    
    std::vector<position_tree_node*> leaves;
    auto up = build_tree(p, state.depth, salpha, sbeta, leaves);
    std::cout << up->alpha << "\n";
    t1 = _bm_nanoTime();
    std::vector<std::pair<position_tree_node*, int>> guesses(leaves.size());
    #pragma omp parallel for schedule(guided)
    for(size_t i = 0;i < leaves.size();i++){
        guesses[i] = {leaves[i], negamax_serial(leaves[i]->p, 2, salpha, sbeta)};
    }
    std::sort(guesses.begin(), guesses.end(), [](const std::pair<position_tree_node*, int>& a, const std::pair<position_tree_node*, int>& b){
        return a.second > b.second;
    });
    for(size_t i = 0;i < leaves.size();i++){
        leaves[i] = guesses[i].first;
    }
    #pragma omp parallel for schedule(guided)
    for(auto& leaf : leaves){
        //std::cout << leaf->p.to_string() << "\n";
        if(!leaf->pruned)
            negamax_node(leaf);
    }
    t2 = _bm_nanoTime();

    std::cout << up->alpha << "\n";
    std::cout << up->evaluated_count << " evaluated, ";
    std::cout << "Time required: " << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    std::cout << up->children.size() << " children" << std::endl;
    
    
    //std::cout << state.bestmove.to_short_notation() << "\n";
    //std::cout << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    //std::cout << state.count << "\n";
    return 0;
}