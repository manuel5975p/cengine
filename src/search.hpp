#ifndef SEARCH_HPP
#define SEARCH_HPP
#include "position.hpp"
#include "evaluation.hpp"
#include <vector>
#include <tuple>
#include <cstdint>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <sys/mman.h>
extern bool interrupt_token;
template<typename T>
constexpr T empty_value(0);
template<typename K, typename V>
struct pairmemmap{
    std::pair<K, V>* kvs;
    std::size_t n_elems;
    size_t m_size;
    constexpr static unsigned probing_attempts = 4;
    pairmemmap() : pairmemmap(1 << 21){}
    pairmemmap(std::size_t size) : kvs(nullptr), n_elems(0){
        m_size = size;
        assert((m_size & (m_size - 1)) == 0 && "Size must be a power of two");
        kvs = (std::pair<K, V>*)mmap(nullptr, m_size * sizeof(std::pair<K, V>), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        clear();
    }
    struct iterator{
        pairmemmap* ptr;
        size_t true_index;
        std::pair<K, V>* operator->()const noexcept{
            return ptr->kvs + true_index;
        }
        std::pair<K, V>* operator*()const noexcept{
            return *(this->operator->());
        }
        bool operator==(const iterator&) const = default;
        bool operator!=(const iterator&) const = default;
    };
    iterator find(const K& key){
        size_t hash = std::hash<K>{}(key);
        size_t index = hash & (m_size - 1);
        size_t attempts = 0;
        while(attempts++ < probing_attempts){
            if(kvs[index].first == key){
                return iterator{this, index};
            }
            index = (index + 1) & (m_size - 1);
        }
        return end();
    }
    void insert(const std::pair<K, V>& kv){
        //if(n_elems > (m_size / 4)){
        //    resize(m_size * 2);
        //}
        size_t hash = std::hash<K>{}(kv.first);
        size_t index = hash & (m_size - 1);
        size_t attempts = 0;
        
        while(kvs[index].first != kv.first && kvs[index].first != empty_value<K> && attempts++ < probing_attempts){
            index = (index + 1) & (m_size - 1);
        }
        if(kvs[index].first != kv.first && kvs[index].first != empty_value<K>){
            std::cerr << "massive collision\n";
        }
        n_elems += kvs[index].first != empty_value<K>;
        kvs[index] = kv;
    }
    void resize(size_t newsize){
        assert((newsize & (newsize - 1)) == 0 && "Size must be a power of two");
        std::pair<K, V>* newkvs = (std::pair<K, V>*)mmap(nullptr, newsize * sizeof(std::pair<K, V>), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        for(size_t i = 0;i < newsize;i++){
            newkvs[i].first = empty_value<K>;
        }
        
        std::swap(kvs, newkvs);
        std::swap(m_size, newsize);
        n_elems = 0;

        for(size_t i = 0;i < newsize;i++){
            if(newkvs[i].first != empty_value<K>){
                insert(newkvs[i]);
            }
        }
        munmap(newkvs, newsize * sizeof(std::pair<K, V>));
    }
    void reserve(size_t n){
        resize(n);
    }
    size_t size()const noexcept{
        return n_elems;
    }
    pairmemmap operator=(const pairmemmap& o) = delete;
    pairmemmap (const pairmemmap& o) = delete;
    pairmemmap operator=(pairmemmap&& o){
        kvs = o.kvs;
        m_size = o.m_size;
        n_elems = o.n_elems;
        o.kvs = nullptr;
    }
    pairmemmap (pairmemmap&& o){
        kvs = o.kvs;
        m_size = o.m_size;
        n_elems = o.n_elems;
        o.kvs = nullptr;
    }
    ~pairmemmap(){
        if(kvs)
            munmap(kvs, m_size * sizeof(K));
        kvs = nullptr;
    }
    iterator end(){
        return iterator{this, ~size_t(0)};
    }
    
    void clear(){
        for(size_t i = 0;i < m_size;i++){
            kvs[i].first = empty_value<K>;
        }
    }
};
struct /*__attribute__((aligned(1), packed))*/ hash_entry{
    int depth;
    int value;
    shortmove bestmove;
    Color at_move;
    bool was_cut_off;
};
struct search_state{
    int depth;
    std::unordered_map<hash_int, std::tuple<int, int, short>> map;
    complete_move bestmove;
    uint64_t count;
    search_state() : map(1 << 20), bestmove(Square(0),Square(0), W_KING, NO_PIECE), count(0){

    }
};
struct turbo_search_state{
    int depth;
    //std::unordered_map<hash_int, std::tuple<int, int, short>> map; //Depth, value, bestmove
    pairmemmap<hash_int, hash_entry> map;
    turbomove bestmove;
    uint64_t count;
    turbo_search_state() : map(1 << 28), count(0){}
};
static int negamax(Position& pos, int depth, int alpha, int beta, search_state& state){
    state.count++;
    
    if(pos.spec_mem.since_capture >= 50){std::cout << "abort eval" << std::endl;return 0;}
    if(depth <= -30){
        return evaluate(pos);
    }
    
    stackvector<complete_move, 256> Zugliste = pos.generate_legal(pos.at_move);
    bool hash_hit_but_shallower = false;
    if(state.map.contains(pos.quickhash())){
        auto it = state.map.find(pos.quickhash());
        if(std::get<0>(it->second) >= depth){
            //std::cout << "hashhit " << depth << std::endl;
            //std::cout << "immediately out: " << std::get<1>(it->second) << ", depth: " << depth << ", computed depth: " << std::get<0>(it->second) << std::endl;
            return std::get<1>(it->second);
        }
        else{
            hash_hit_but_shallower = true;
        }
    }
    if(Zugliste.size() == 0){
        if(pos.check(pos.at_move)){
            return -123456;
        }
        else
            return 0;
    }

    if (depth <= 0){
        bool atcheck = pos.check(pos.at_move);
        if(!atcheck)
        for(auto it = Zugliste.begin();it < Zugliste.end();){
            state.count++;
	    	if(it->captured_piece == NO_PIECE){ //TODO: en passant
                Position pclone(pos);
                bool ch = pclone.apply_move_checked(*it);
                if(!ch)std::terminate();
                if(!pclone.check(pclone.at_move)){
	    		    std::swap(*it, *(Zugliste.end() - 1));
	    		    Zugliste.pop_back();
                }
                else{
                    //std::cout << it->to_string() << "\n";
                    //std::cout << pclone.to_string() << "\n\n";
                    it++;
                }
	    	}
	    	else{
	    		it++;
            }
	    }
        if(Zugliste.size() > 0){
            //if(!atcheck){
                int stand_pat = evaluate(pos);
                if(stand_pat >= beta){
                    return stand_pat;
                }
                if(alpha < stand_pat){
                    alpha = stand_pat;
                }
            //}
            int maxWert = alpha;
            for (auto& move : Zugliste) {
                Position pclone(pos);
                bool ch = pclone.apply_move_checked(move);
                if(!ch)std::terminate();
                int wert = -negamax(pclone, depth - 1, -beta, -maxWert, state);
                //pos.revert_move_checked(move);
                if(depth == state.depth){
                    //std::cout << move.to_string() << ": " << wert << "\n"; 
                }
                if (wert > maxWert) {
                    maxWert = wert;
                    if (maxWert >= beta){
                        state.map[pos.quickhash()] = std::make_tuple(depth, maxWert, short(move.from) << 8 | short(move.to));
                        return maxWert;
                    }
                }
            }
            return maxWert;
        }
        else
            return evaluate(pos);
    }
    else if(depth == 2){
        int score = negamax(pos, 0, alpha, beta, state);
        if(score < -500 || score > beta){
            return score;
        }
    }
    int maxWert = alpha;
    std::vector<std::pair<complete_move, int>> guesses(Zugliste.size());
    short previous_best_move = -1;
    if(hash_hit_but_shallower)
        previous_best_move = std::get<2>(state.map.find(pos.quickhash())->second);
    for(size_t i = 0;i < Zugliste.size();i++){
        Position pclone(pos);
        bool ch = pclone.apply_move_checked(Zugliste[i]);
        int wert;
        auto nit = state.map.find(pclone.quickhash());
            if((nit) != state.map.end()){
                wert = std::get<1>(nit->second);
            }
        else
            wert = -evaluate(pclone);
        guesses[i] = std::make_pair(Zugliste[i], wert);
        if(hash_hit_but_shallower){
            if(Zugliste[i].from == previous_best_move >> 8 && Zugliste[i].to == (previous_best_move & 255)){
                //std::cout << "katoff" << std::endl;
                guesses[i].second += 1000;
            }
        }
    }
    std::sort(guesses.begin(), guesses.end(), [](const std::pair<complete_move, int>& a1, const std::pair<complete_move, int>& a2){
        return a1.second > a2.second;
    });
    if (depth == state.depth){
        state.bestmove = guesses[0].first;
    }
    size_t index(0);
    complete_move local_best_move;
    for (auto& [move, wart] : guesses) {
        ++index;
        Position pclone(pos);
        bool ch = pclone.apply_move_checked(move);
        if(!ch)std::terminate();
        int wert = -negamax(pclone, depth - 1, -beta, -maxWert, state);
        //pos.revert_move_checked(move);
        if(depth == state.depth){
            //std::cout << move.to_string() << ": " << wert << "\n"; 
        }
        if (wert > maxWert) {
            local_best_move = move;
            maxWert = wert;
            if (depth == state.depth){
                state.bestmove = move;
            }
            if (maxWert >= beta){
                //std::cout << "katoff " << index << " / " << guesses.size() << std::endl;
                state.map[pos.quickhash()] = std::make_tuple(depth, maxWert, short(move.from) << 8 | short(move.to));
                return maxWert;
            }
        }
    }
    if(maxWert > alpha){
        state.map[pos.quickhash()] = std::make_tuple(depth, maxWert, short(local_best_move.from) << 8 | short(local_best_move.to));
    }
    return maxWert;
}
int negamax_multithreaded(Position& pos, int depth, int alpha, int beta, search_state& state);
int negamax_serial(Position& pos, int depth, int alpha, int beta, turbo_search_state& state);
int qsearch(Position& pos, int depth, int alpha, int beta, turbo_search_state& state);
uint64_t perft(Position& p, int depth);
inline int qsearch_naive(Position& pos, int depth, turbo_search_state& state){
    state.count++;
    int maxWert = evaluate(pos);
    constexpr int value[] = {1,3,3,5,8};
    auto loud = pos.generate_loud(pos.at_move);
    /*sort_based_on(loud.begin(), loud.end(),[&](const turbomove& tm){
        if(tm.bb2 && tm.index2 < 12){
            return value[tm.index2 % 6] - value[tm.index1 % 6];
        }
        return 0;
    });*/
    for(auto& mv : loud){
        special_members backup = pos.spec_mem;
        pos.apply_move(mv);
        int wert = -qsearch_naive(pos, depth - 1, state);
        pos.undo_move(mv);
        pos.spec_mem = backup;
        if(wert > maxWert)
            maxWert = wert;
    }
    return maxWert;
}
inline int negamax_naive(Position& pos, int depth, turbo_search_state& state){
    state.count++;
    if(depth == 0)return evaluate(pos);
    int maxWert = -10000000;
    auto legal = pos.generate_new(pos.at_move);
    for (size_t i = 0;i < legal.size();i++) {
        special_members backup = pos.spec_mem;
        pos.apply_move(legal[i]);
        int wert = -negamax_naive(pos, depth - 1, state);
        pos.undo_move(legal[i]);
        pos.spec_mem = backup;
        if (wert > maxWert) {
            maxWert = wert;
            if(depth == state.depth){
                state.bestmove = legal[i];
            }
        }
    }
    return maxWert;
}
#endif