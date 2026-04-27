#include"mSkipList.hpp"
#include<iostream>
#include<random>
int main(){
    mSkipList<int,int> sl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1,10000);

    for (int i = 0; i < 100000; i++)
    {
        int ran = dist(gen);
        sl.put(ran,ran);
    }
    
    bool is=false;
    int a=0;
    for (auto it = sl.begin(); it != sl.end(); it++)
    {
        is=is||!(a<=it->key);
        a=it->key;
        std::cout<<it->key<<"\t"<<it->value<<std::endl;
    }

    if(is)
        std::cout<<"顺序有误"<<std::endl;
    else
        std::cout<<"顺序无误"<<std::endl;

    std::cout<<sl.length()<<"\t"<<sl.deep()<<std::endl;
    // for (auto it = sl.end(); it != sl.begin(); it--)
    // {
    //     std::cout<<it->key<<"\t"<<it->value<<std::endl;
    // }
}
