#include "thread_pool.hpp"
#include <iostream>
#include <vector>
#include <benchmark.hpp>
#include <cmath>
#include <chrono>
#include "search.hpp"
cb::ThreadPool pool[2] = {cb::ThreadPool(4), cb::ThreadPool(16)};
/*volatile int active = 0;
int recursive_function_single(int depth, int a, int b){
    if(depth == 0){
        return std::sqrt(std::sqrt(10.0 * std::sin(a + b + 0.1) * std::sin(a + b + 0.1)));
    }
    int accum = 0;
    for(int i = a;i < b;i++){
        accum += recursive_function_single(depth - 1, a + 1, b);
    }
    return accum;
}
int recursive_function(int depth, int a, int b){
    if(depth == 0){
        return std::sqrt(std::sqrt(10.0 * std::sin(a + b + 0.1) * std::sin(a + b + 0.1)));
    }
    if(depth <= 4){
        int accum = 0;
        for(int i = a;i < b;i++){
            accum += recursive_function(depth - 1, a + 1, b);
        }
        return accum;
    }
    else{
        int accum = 0;
        std::vector<std::future<int>> futures;
        for(int i = a;i < b;i++){
            if(i >= a + 4){
                accum += futures[i - a - 4].get();
            }
            futures.push_back(pool.ScheduleAndGetFuture(recursive_function, depth - 1, a + 1, b));
        }
        for(int i = futures.size() - 4;i < futures.size();i++){
            accum += futures[i].get();
        }
        return accum;
    }
}*/
int negamax_multithreaded(Position& pos, int depth, const volatile int* alpha, const volatile int* beta, search_state& state){
    auto legal = pos.generate_legal(pos.at_move);
    int maxWert = *alpha;
    int childalpha = -*beta, childbeta = -maxWert;
    if(depth <= 0){
        return evaluate(pos);
    }
    
    if(state.depth - depth < 2){
        size_t index = 0;
        stackvector<Position, 128> positions;
        std::array<std::future<int>, 4> futures;
        for(size_t i = 0;i < legal.size();i++){
            positions.push_back(pos);
            positions.back().apply_move_checked(legal[i]);
        }
        
        for(size_t i = 0;i < std::min(size_t(4), legal.size());i++){
            futures[i] = pool[state.depth - depth].ScheduleAndGetFuture(negamax_multithreaded, positions[i], depth - 1, &childalpha, &childbeta, state);
            index++;
        }
        while(index < legal.size()){
            for(size_t i = 0;i < 4;i++){
                if(futures[i].valid() && futures[i].wait_for(std::chrono::nanoseconds(0)) == std::future_status::ready){
                    if(*alpha > maxWert){
                        maxWert = *alpha;
                        childbeta = -maxWert;
                    }
                    int wert = -futures[i].get();
                    
                    if (wert > maxWert) {
                        maxWert = wert;
                        if (depth == state.depth){
                            state.bestmove = legal[i];
                        }
                        childbeta = -maxWert;
                    }
                    if (maxWert >= *beta){
                        childbeta = -1000000;
                        break;
                    }
                    futures[i] = pool[state.depth - depth].ScheduleAndGetFuture(negamax_multithreaded, positions[index], depth - 1, &childalpha, &childbeta, state);
                    index++;
                    if(index >= legal.size())break;
                }
            }
            //if(futures[0].valid()){
            //    std::cout << futures[0].get() << std::endl;
            //}
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        for(size_t i = 0;i < std::min(size_t(4), legal.size());i++){
            int wert = -futures[i].get();
            if (wert > maxWert) {
                maxWert = wert;
                if (depth == state.depth){
                    state.bestmove = legal[i];
                }
                if (maxWert >= *beta){
                    break;
                }
            }
        }
        return maxWert;
    }
    else{
        //std::cout << "invoked " << std::endl;
        for (size_t i = 0;i < legal.size();i++) {
            Position pclone(pos);
            pclone.apply_move_checked(legal[i]);
            int wert = -negamax_multithreaded(pclone, depth - 1, &childalpha, &childbeta, state);
            if (wert > maxWert) {
                maxWert = wert;
                childbeta = -maxWert;
                if (depth == state.depth){
                    state.bestmove = legal[i];
                }
                if (maxWert >= *beta){
                    return maxWert;
                }
            }
        }
        //std::cout << "returned " << std::endl;
    }
    return maxWert;
}
/*int func2(){
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    return _bm_nanoTime() >> 32;
}
int func(){
    std::future<int> fut = pool.ScheduleAndGetFuture(func2);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return _bm_nanoTime() >> 32;
}

int main0(){
    std::future<int> fut = pool.ScheduleAndGetFuture(func);
    return 0;
    //std::cout << fut.get() << "\n";
}*/
int main() {
    Bitboards::init();
    Position p;//("rnb1kbnr/ppp1pppp/3q4/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 3");
    search_state state;
    state.count = 0;
    state.depth = 7;
    int salpha = -100000, sbeta = 100000;
    auto t1 = _bm_nanoTime();
    negamax_multithreaded(p, state.depth, &salpha, &sbeta, state);
    auto t2 = _bm_nanoTime();
    std::cout << state.bestmove.to_short_notation() << "\n";
    std::cout << (t2 - t1) / 1000 / 1000.0 << " ms" << std::endl;
    std::cout << state.count << "\n";
    return 0;
}