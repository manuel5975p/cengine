#include "search.hpp"
bool interrupt_token;
int qsearch(Position& pos, int depth, int alpha, int beta, turbo_search_state& state){
    //auto iter = state.map.find(pos.quickhash());
    //if(iter != state.map.end()){
    //    return std::get<1>(iter->second);
    //}
    state.count++;
    int stand_pat = evaluate(pos);
    if( stand_pat >= beta || depth < -8){
        //std::cout << depth << "\n";
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
std::pair<int, turbomove> negamax_serial_impl(Position& pos, int depth, int alpha, int beta, turbo_search_state& state, const std::unordered_set<turbomove>& upper_killerset);
int negamax_serial(Position& pos, int depth, int alpha, int beta, turbo_search_state& state){
    std::unordered_set<turbomove> empty;
    std::pair<int, turbomove> vmpair = negamax_serial_impl(pos, depth, alpha, beta, state, empty);
    if(vmpair.second.index1 != 12){
        state.bestmove = vmpair.second;
    }
    return vmpair.first;
}
std::pair<int, turbomove> negamax_serial_impl(Position& pos, int depth, int alpha, int beta, turbo_search_state& state, const std::unordered_set<turbomove>& upper_killerset){
    int maxWert = alpha;
    stackvector<turbomove, 256> legal;
    shortmove last_cutoff_move;
    int what_would_have_been = -777;
    if(depth > 0){
        auto mapit = state.map.find(pos.quickhash());
        if(mapit != state.map.end()){
            const auto& entry = mapit->second;
            if(entry.depth >= depth){
                if(!entry.was_cut_off){
                    std::optional<turbomove> opt = pos.create_move(entry.bestmove);
                    assert(opt.operator bool());
                    //return {entry.value, opt.value()};
                }
                else if(entry.value >= beta){
                    std::optional<turbomove> opt = pos.create_move(entry.bestmove);
                    assert(opt.operator bool());
                    //return {entry.value, opt.value()};
                }
            }
            
            last_cutoff_move = mapit->second.bestmove;
            if(false){// && last_cutoff_move){
                auto opt = pos.create_move(last_cutoff_move);
                if(!opt){
                    last_cutoff_move.set_null();
                }
                else{
                    //std::cerr << "asdasa\n";
                    turbomove lctm = opt.value();
                    special_members backup = pos.spec_mem;
                    pos.apply_move(lctm);
                    std::pair<int, turbomove> vmpair = negamax_serial_impl(pos, depth - 1, -beta, -maxWert, state, std::unordered_set<turbomove>());
                    pos.undo_move(lctm);
                    pos.spec_mem = backup;
                    /*if(-vmpair.first >= beta){
                        return vmpair{};
                    }*/
                    if(-vmpair.first > maxWert){
                        maxWert = vmpair.first;
                    }
                }   
            }
        }
    }
    state.count++;
    if(depth <= 0){
        /*legal = pos.generate_new(pos.at_move);
        if(legal.size() == 0){
            if(pos.check(pos.at_move)){
                return -1234567;
            }
            return 0;
        }*/
        return {qsearch(pos, depth, alpha, beta, state), turbomove{12,0,0,0,0}};
    }
    
    if(depth > 0){
        legal = pos.generate_new(pos.at_move);
        if(legal.size() == 0){
            if(pos.check(pos.at_move)){
                return {-1234567, turbomove{12,0,0,0,0}};
            }
            return {0, turbomove{12,0,0,0,0}};
        }
    }
    Square starting_square = Square(last_cutoff_move.from());
    Square  ending_square =  Square(last_cutoff_move.to());
    Bitboard cutoffcheckmask = (1ull << starting_square) | (1ull << ending_square);
    if(depth >= 3){
        
        sort_based_on(legal.begin(), legal.end(), [&](const turbomove& tm){
            if(!last_cutoff_move.is_null()){
                Bitboard matsch = tm.bb1 & cutoffcheckmask;
                Bitboard popbit = matsch & (matsch - 1);
                if(popbit){
                    return 10000;
                }
            }
            auto kit = upper_killerset.find(tm);
            if(kit != upper_killerset.end()){
                return 5000;
            }
            
            int eval;
            special_members backup = pos.spec_mem;
            pos.apply_move(tm);
            
            auto cit = state.map.find(pos.quickhash());
            if(cit != state.map.end()){
                eval = -cit->second.value;
            }
            else{
                eval = -qsearch(pos, -7, -beta, -alpha, state);
            }   
            eval += tm.bb2 ? 200 : 0;
            pos.undo_move(tm);
            pos.spec_mem = backup;
            return eval;
        });
    }
    else if(depth >= 1){
        
        sort_based_on(legal.begin(), legal.end(), [&](const turbomove& tm) -> int{
            if(!last_cutoff_move.is_null()){
                Bitboard matsch = (tm.bb1 & cutoffcheckmask);
                Bitboard popbit = (matsch & (matsch - 1)   );
                if(popbit){
                    return 10000;
                }
            }
            auto kit = upper_killerset.find(tm);
            if(kit != upper_killerset.end()){
                return 5000;
            }
            constexpr int value[] = {1,3,3,5,8};
            if(tm.bb2 && tm.index2 < 12){
                return 3 + value[tm.index2 - (tm.index2 >= 6 ? 6 : 0)] - value[tm.index1 - (tm.index1 >= 6 ? 6 : 0)];
            }
            return 0;
        });
    }
	size_t i = 0;
    turbomove cbest = legal[0];
    std::unordered_set<turbomove> killerset;
    killerset.reserve(legal.size());
    
    bool was_alpha_raised = false;
    bool bsearchPv = true;
    bool was_cutoff = false;
	if(false && depth >= 6 && alpha > -4000){
		for (i = 0;i < legal.size();i++) {
        if(interrupt_token)return {0, turbomove{12,0,0,0,0}};
        special_members backup = pos.spec_mem;
        pos.apply_move(legal[i]);
        std::pair<int, turbomove> vmpair;
        vmpair = negamax_serial_impl(pos, depth - 4, -beta, -maxWert + 1000, state, killerset);
        int wert = -vmpair.first;
        pos.undo_move(legal[i]);
        pos.spec_mem = backup;
		if(wert <= alpha - 100){
			std::cout << "klönked\n";
			std::swap(legal[i], legal[legal.size() - 1]);
			legal.pop_back();
		}
		else{
			i++;
		}
    }
	}
   

    for (i = 0;i < legal.size();i++) {
        if(interrupt_token)return {0, turbomove{12,0,0,0,0}};
        special_members backup = pos.spec_mem;
        pos.apply_move(legal[i]);
        std::pair<int, turbomove> vmpair;
        if(bsearchPv){
            if(depth == state.depth - 1 && legal[i].to_short_notation() == "c6f3" && depth == 7){
                std::cout.flush();
            }
            vmpair = negamax_serial_impl(pos, depth - 1, -beta, -maxWert, state, killerset);
        }
        else{
            vmpair = negamax_serial_impl(pos, depth - 1, -maxWert - 1, -maxWert, state, killerset);
            if(-vmpair.first > maxWert && -vmpair.first < beta)
                vmpair = negamax_serial_impl(pos, depth - 1, -beta, -maxWert, state, killerset);
        }
        
        if(vmpair.second.bb1)
            killerset.insert(vmpair.second);
        int wert = -vmpair.first;
        pos.undo_move(legal[i]);
        pos.spec_mem = backup;
        if(depth == state.depth){
            std::cout << legal[i].to_short_notation() << ": " << -vmpair.first << std::endl;
        }
        //bsearchPv = false;
        if (wert > maxWert) {
            was_alpha_raised = true;
            cbest = legal[i];
            maxWert = wert;
            if(depth == state.depth){
                state.bestmove = legal[i];
            }
            if (maxWert >= beta){
                was_cutoff = true;
                break;
            }
        }
    }
    /*if(depth == 1){
        if(last_cutoff_move && (last_cutoff_move == short(int(int(from) << 8) | int(to)))){
            //if(i == legal.size())std::cout << depth << "<\n";
            std::cout << "Match: cutoff " << i << " / " << legal.size() << "\n";
            print(cutoffcheckmask);
            std::cout << "===================\n";
            print(cbest.bb1);
            std::cout << "Real Cutoff caused by " << cbest.to_short_notation() << "\n";
        }
    
        else{
            std::cout << "Tcham\n";
        }
    }*/
    //bool already_in = false;
    //if(state.map.find(pos.quickhash()) != state.map.end()){
    //    already_in = state.map.find(pos.quickhash())->second.depth < depth;
    //    if(state.map.find(pos.quickhash())->second.depth == depth){
    //        assert(state.map.find(pos.quickhash())->second.value == maxWert);
    //    }
    //}
    //if(!already_in)
    if(was_alpha_raised)
        state.map.insert(std::make_pair(pos.quickhash(), hash_entry{depth, maxWert, cbest.to_shortmove(), pos.at_move, was_cutoff}));
    return {maxWert, cbest};
}
template<bool print>
uint64_t perft(Position& p, int depth){
    generator<turbomove> moves;
    if(p.at_move == WHITE){
        moves = p.generate_coroutine<WHITE>();
    }
    if(p.at_move == BLACK){
        moves = p.generate_coroutine<BLACK>();
    }
    if(depth == 1){
        size_t s = 0;
        while(moves.next()){
            s++;
            if constexpr(print)
                std::cout << moves.getValue().to_short_notation() << ": " << 1 << std::endl;
        }
        return s;
    }
    uint64_t accum = 0;
    if constexpr(print){
        //#pragma omp parallel for schedule(guided)
        while(moves.next()){
            special_members backup = p.spec_mem;
            Position pclone(p);
            pclone.apply_move(moves.getValue());
            uint64_t ps = perft<false>(pclone, depth - 1);
            accum += ps;
            std::cout << moves.getValue().to_short_notation() << ": " << ps << std::endl;
        }
    }
    else{
        while(moves.next()){
            special_members backup = p.spec_mem;
            Position backupo(p);
            p.apply_move(moves.getValue());
            uint64_t ps = perft<false>(p, depth - 1);
            p.undo_move(moves.getValue());
            p.spec_mem = backup;
            accum += ps;
        }
    }
    return accum;
}
uint64_t perft(Position& p, int depth){
    return perft<true>(p, depth);
}