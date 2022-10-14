#include "search.hpp"
int qsearch(Position& pos, int depth, int alpha, int beta, turbo_search_state& state){
    auto iter = state.map.find(pos.quickhash());
    //if(iter != state.map.end()){
    //    return std::get<1>(iter->second);
    //}
    state.count++;
    int stand_pat = evaluate(pos);
    if( stand_pat >= beta || depth < -8){
        //state.map[pos.quickhash()] = std::make_tuple(depth, stand_pat, 0);
        return stand_pat;
    }
    if( alpha < stand_pat )
        alpha = stand_pat;
    constexpr int value[] = {1,3,3,5,8};
    auto loud = pos.generate_loud(pos.at_move);
    sort_based_on(loud.begin(), loud.end(),[&](const turbomove& tm){
        if(tm.bb2 && tm.index2 < 12){
            return 1 + value[tm.index2 - (tm.index2 >= 6 ? 6 : 0)] - value[tm.index1 - (tm.index1 >= 6 ? 6 : 0)];
        }
        return 0;
    });
    for(auto& mv : loud){
        special_members backup = pos.spec_mem;
        pos.apply_move(mv);
        int wert = -qsearch(pos, depth - 1, -beta, -alpha, state);
        pos.undo_move(mv);
        pos.spec_mem = backup;
        if(wert >= beta){
            //state.map[pos.quickhash()] = std::make_tuple(depth, wert, 0);
            return wert;

        }
        if(wert > alpha)
            alpha = wert;
    }
    //state.map[pos.quickhash()] = std::make_tuple(depth, alpha, 0);
    return alpha;
}
int negamax_serial(Position& pos, int depth, int alpha, int beta, turbo_search_state& state){
    int maxWert = alpha;
    
    stackvector<turbomove, 256> legal;
    //auto mapit = state.map.find(pos.quickhash());
    short last_cutoff_move = 0;
    //if(mapit != state.map.end()){
    //    const auto& [entry_depth, entry_value, entry_shortmove] = mapit->second;
    //    if(entry_depth >= depth){
    //        return entry_value;
    //    }
    //    last_cutoff_move = std::get<2>(mapit->second);
    //}
    state.count++;
    if(depth <= 0){
        /*legal = pos.generate_new(pos.at_move);
        if(legal.size() == 0){
            if(pos.check(pos.at_move)){
                return -1234567;
            }
            return 0;
        }*/
        return qsearch(pos, depth, alpha, beta, state);
    }
    
    if(depth > 0){
        legal = pos.generate_new(pos.at_move);
        if(legal.size() == 0){
            if(pos.check(pos.at_move)){
                return -1234567;
            }
            return 0;
        }
    }
    if(depth >= 3){
        sort_based_on(legal.begin(), legal.end(), [&pos, beta, alpha, maxWert, depth, &state, last_cutoff_move](const turbomove& tm){
            //if(last_cutoff_move){
            //    Square starting_square = Square(last_cutoff_move & 63);
            //    Square  ending_square =  Square((last_cutoff_move >> 8) & 63);
            //    if(tm.bb1 & (1ull << starting_square)){
            //        if(tm.bb1 & (1ull << ending_square)){
            //            return 10000;
            //        }
            //    }
            //}
            int eval;
            special_members backup = pos.spec_mem;
            pos.apply_move(tm);
            //auto cit = state.map.find(pos.quickhash());
            //if(cit != state.map.end()){
            //    eval = -std::get<1>(cit->second);
            //}
            //else
                eval = -evaluate(pos);
            pos.undo_move(tm);
            pos.spec_mem = backup;
            return eval;
        });
    }
    else if(depth >= 1){
        
        sort_based_on(legal.begin(), legal.end(), [](const turbomove& tm){
            constexpr int value[] = {1,3,3,5,8};
            if(tm.bb2 && tm.index2 < 12){
                return 3 + value[tm.index2 % 6] - value[tm.index1 % 6];
            }
            return 0;
        });
    }
    size_t i = 0;
    turbomove cbest = legal[0];
    for (i = 0;i < legal.size();i++) {
        special_members backup = pos.spec_mem;
        pos.apply_move(legal[i]);
        int wert = -negamax_serial(pos, depth - 1, -beta, -maxWert, state);
        pos.undo_move(legal[i]);
        pos.spec_mem = backup;
        if (wert > maxWert) {
            cbest = legal[i];
            maxWert = wert;
            if(depth == state.depth){
                state.bestmove = legal[i];
            }
            if (maxWert >= beta){
                break;
            }
        }
    }
    Square from = lsb(cbest.bb1);
    Square to   = lsb(_blsr_u64(cbest.bb1));
    if((cbest.flags >> 1) & 1){
        std::swap(from, to);
    }
    //state.map[pos.quickhash()] = std::make_tuple(depth, maxWert, short(int(int(from) << 8) | int(to)));
    return maxWert;
}
template<bool print>
uint64_t perft(Position& p, int depth){
    
    stackvector<turbomove, 256> moves;
    if(p.at_move == WHITE){
        moves = p.generate_new<WHITE>();
    }
    if(p.at_move == BLACK){
        moves = p.generate_new<BLACK>();
    }
    if(depth == 1){
        if constexpr(print){
            for(auto& mv : moves)
                std::cout << mv.to_short_notation() << ": " << 1 << std::endl;
        }
        return moves.size();
    }
    uint64_t accum = 0;
    if constexpr(print){
        //#pragma omp parallel for schedule(guided)
        for(auto& mv : moves){
            special_members backup = p.spec_mem;
            Position pclone(p);
            pclone.apply_move(mv);
            uint64_t ps = perft<false>(pclone, depth - 1);
            accum += ps;
            std::cout << mv.to_short_notation() << ": " << ps << std::endl;
        }
    }
    else{
        for(auto& mv : moves){
            special_members backup = p.spec_mem;
            Position backupo(p);
            p.apply_move(mv);
            uint64_t ps = perft<false>(p, depth - 1);
            p.undo_move(mv);
            p.spec_mem = backup;
            accum += ps;
        }
    }
    return accum;
}
uint64_t perft(Position& p, int depth){
    return perft<true>(p, depth);
}