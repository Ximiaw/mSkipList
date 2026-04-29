#include"mSkipList.hpp"
#include<iostream>
#include<random>

int main(){
    mSkipList<int,int> sm;
    auto view = sm.getView(3);
    sm.delView(view);
}
