#include <Eigen/Dense>
#include <filesystem>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstddef>
#include <avx.hpp>
using std::size_t;
//constexpr std::array<size_t, 4> sizes = {3,4,4,1};
constexpr std::array<size_t, 6> sizes = {832,256,256,256,256,1};
namespace fs = std::filesystem;
void readfiletom(const std::string& str, Eigen::MatrixXf& mat){
    std::ifstream istr(str);
    for(size_t i = 0;i < mat.rows();i++){
        for(size_t j = 0;j < mat.cols();j++){
            float read;
            istr >> read;
            mat(i,j) = read;
        }
    }
    mat.transposeInPlace();
}
float relu(float x){
    if(x > 0)return x;
    return 0;
}
struct neural_net{
    std::vector<Eigen::MatrixXf> weights;
    neural_net(const std::string& p){
        std::vector<std::string> weightfiles;
        for(auto& el : fs::directory_iterator(p)){
            //std::cout << el.path().string() << "<\n";
            if(el.path().string().find("weights") != std::string::npos){
                weightfiles.push_back(el.path().string());
                
            }
        }
        std::sort(weightfiles.begin(), weightfiles.end());
        for(size_t i = 0;i < weightfiles.size();i++){
            std::cout << weightfiles[i] << "\n";
        }
        weights.push_back(Eigen::MatrixXf(sizes[0], sizes[1]));
        readfiletom(weightfiles[0], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[1], 1));
        readfiletom(weightfiles[1], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[1], sizes[2]));
        readfiletom(weightfiles[2], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[2], 1));
        readfiletom(weightfiles[3], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[2], sizes[3]));
        readfiletom(weightfiles[4], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[3], 1));
        readfiletom(weightfiles[5], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[3], sizes[4]));
        readfiletom(weightfiles[6], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[4], 1));
        readfiletom(weightfiles[7], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[4], sizes[5]));
        readfiletom(weightfiles[8], weights.back());
        weights.push_back(Eigen::MatrixXf(sizes[5], 1));
        readfiletom(weightfiles[9], weights.back());
    }
    Eigen::MatrixXf operator()(Eigen::MatrixXf input){
        for(size_t i = 0;i < weights.size();i += 2){
            //std::cout << weights[i].rows() << ", " << weights[i].cols() << "\n";
            //std::cout << input.rows() << "\n\n";
            Eigen::MatrixXf input2 = weights[i] * input;
            input = input2;
            //std::cout << input.rows() << "\n";
            //std::cout << weights[i + 1].cols() << "\n";
            size_t elcount = input.cols() * input.rows();
            
            for(size_t ih = 0;ih < input.cols();ih++){
                input.col(ih) += weights[i + 1].transpose();
            }
            if(i + 2 < weights.size()){
            	for(size_t c = 0;c < elcount;c += 1){
                    input.data()[c] = std::tanh(input.data()[c]);
                }
                /*for(size_t c = 0;c < elcount - (elcount % 8);c += 8){
                    vec8f loaded(input.data() + c);
                    loaded = loaded.relu();
                    loaded.store(input.data() + c);
                }

                for(size_t k = elcount - (elcount % 8);k < elcount;k++){
                    input.data()[k] = relu(input.data()[k]);
                }*/
            }
            //std::cout << input << "\n";
            //std::terminate();
        }
        //assert(input.rows() == 1);
        return input;
    }
};
