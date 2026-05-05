#include"mSkipList.hpp"
#include<iostream>
#include<string>
#include<random>
#include<chrono>

int main(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 10000000000);
    
    mSkipList<0,long long,std::string> msl{0,""};

    auto start_msl=std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 1000000; i++)
    {
        msl.insert(dis(gen),std::to_string(i));
    }
    auto end_msl=std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_msl - start_msl);
    
    auto start_get=std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < 1000000; i++)
    {
        msl.get<long long>(dis(gen));
    }
    auto end_get=std::chrono::high_resolution_clock::now();
    auto duration_ns_get = std::chrono::duration_cast<std::chrono::nanoseconds>(end_msl - start_msl);

    auto node=msl.fir();
    int old=0;
    bool a=false;
    while (node->rightIndex.size()>0)
    {
        a=a||old>node->data().ref<long long>(0);
        std::cout<<node->data().ref<long long>(0)<<"\t"<<node->data().ref<std::string>(1)<<std::endl;
        old=node->data().ref<long long>(0);
        node=node->rightIndex[0];
    }
    a=a||a<node->data().ref<long long>(0);
    std::cout<<node->data().ref<long long>(0)<<"\t"<<node->data().ref<std::string>(1)<<std::endl;

    auto str=a?"乱序":"顺序";
    std::cout<<str<<std::endl;


    std::cout << "插入耗时: " << duration_ns.count() << " 纳秒" << std::endl;
    std::cout << "查找耗时: " << duration_ns.count() << " 纳秒" << std::endl;
    return 0;
}
