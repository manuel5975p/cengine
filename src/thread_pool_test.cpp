#include "thread_pool.hpp"
#include <iostream>
#include <vector>
#include <benchmark.hpp>
#include <cmath>
#include <chrono>
#include <memory>
#include <set>
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
    std::vector<std::pair<complete_move, int>> guesses(legal.size());
    for(size_t i = 0;i < legal.size();i++){
        Position pclone(p);
        pclone.apply_move_checked(legal[i]);
        guesses[i] = {legal[i], negamax_serial(pclone, 1, alpha, beta)};
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
    int depth = 3;
    for (size_t i = 0;i < legal.size();i++) {
        Position pclone(pos);
        pclone.apply_move_checked(legal[i]);
        if(node->alpha >= node->beta)break;
        int wert = -negamax_serial(pclone, depth - 1, -node->beta, -node->alpha);
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
uint64_t perft(Position& p, int depth, bool verbose = false){
    stackvector<turbomove, 256> moves;
    if(p.at_move == WHITE){
        moves = p.generate_new<WHITE>();
    }
    if(p.at_move == BLACK){
        moves = p.generate_new<BLACK>();
    }
    //auto leg = p.generate_legal(p.at_move);
    /*if(leg.size() != moves.size()){
        
        //std::cout << p.to_string() << "\n";
        //std::cout << p.fen() << "\n";
        //std::set<std::string> missing;
        //for(auto& turbo : leg){
        //    missing.insert(turbo.to_short_notation());
        //}
        //for(auto& turbo : moves){
        //    missing.erase(turbo.to_short_notation());
        //}
        //std::cout << *missing.begin() << "\n";
    }*/
    if(depth == 1){
    //    if(verbose){
    //        for(auto& mv : moves){
    //            //std::cout << mv.to_short_notation() << "\n";
    //        }
    //    }
        return moves.size();
    }
    
    uint64_t accum = 0;
    for(auto& mv : moves){
        special_members backup = p.spec_mem;
        bool lverbose = verbose;
        //if(mv.to_short_notation() == "f3e5"){
        //    print(pclone.get<B_PAWN>());
        //}
        p.apply_move(mv);
        //std::cout << mv.to_short_notation() << "\n";
        //if(mv.to_short_notation() == "a5b6"){
            //lverbose = true;
            //std::cout << mv.index1 << ' ' << mv.index2 << std::endl;
            //std::cout << bb_to_string(mv.bb1) << bb_to_string(mv.bb2) << std::endl;
            //std::cout << pclone.spec_mem.ep << "\n";
            //std::cout << pclone.fen() << "\n";
        //}
        uint64_t ps = perft(p, depth - 1, lverbose);
        p.undo_move(mv);
        p.spec_mem = backup;
        accum += ps;
        //if(depth == 6 || verbose){
        //    std::cout << mv.to_short_notation() << ": " << ps << std::endl;
        //}
    }
    return accum;
}
int main() {
    Bitboards::init();
    Position p;//("rnbqkbnr/p1pppppp/1p6/8/8/P3P3/1PPP1PPP/RNBQK2R w KQkq - 0 1");
    std::cout << p.fen() << "\n";
    std::cout << perft(p, 6) << "\n";
    return 0;
    //print(LineBetween[0][36]);
    auto pmoves = p.generate_new<BLACK>();
    for(auto& move : pmoves){
        std::cout << move.to_short_notation() << "\n";
    }
    return 0;
    search_state state;
    state.count = 0;
    state.depth = 2;
    int salpha = -100000, sbeta = 100000;
    auto t1 = _bm_nanoTime();
    int realval = negamax_serial(p, 5, salpha, sbeta);
    auto t2 = _bm_nanoTime();
    std::cout << "Akchual value: " << realval << std::endl;
    std::cout << "Time required: " << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    
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