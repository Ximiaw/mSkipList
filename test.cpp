
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <chrono>
#include <random>
#include "mSkipList.hpp"

using namespace std;
using namespace msl;
using namespace std::chrono;

int main() {
    auto rd=random_device();
    auto m=mt19937(rd());
    size_t random_max=500000;
    auto r=uniform_int_distribution<>(1,random_max);

    size_t max=1000000;
    cout<<"随机插查"<<max<<"条1~"<<random_max<<endl;

    vector<size_t> v;
    set<size_t> s;
    for (size_t i = 0; i < max; i++)
    {
        size_t si = r(m);
        v.push_back(si);
        s.insert(si);
    }
    
    auto m_map=map<size_t,size_t>{};
    auto s_m=high_resolution_clock::now();
    for (size_t i = 0; i < v.size(); i++)
    {
        m_map.insert({v[i],v[i]});
    }
    auto e_m=high_resolution_clock::now();
    auto el_m=duration_cast<milliseconds>(e_m-s_m);
    cout<<"map插入:"<<el_m.count()<<"ms"<<endl;

    m_map=map<size_t,size_t>{};
    s_m=high_resolution_clock::now();
    for (size_t i = 0; i < v.size(); i++)
    {
        m_map[v[i]];
    }
    e_m=high_resolution_clock::now();
    el_m=duration_cast<milliseconds>(e_m-s_m);
    cout<<"map查找:"<<el_m.count()<<"ms"<<endl;

    s_m=high_resolution_clock::now();
    for (auto it=s.begin(); it!=s.end(); it++)
    {
        m_map.erase(*it);
    }
    e_m=high_resolution_clock::now();
    el_m=duration_cast<milliseconds>(e_m-s_m);
    cout<<"map删除:"<<el_m.count()<<"ms"<<endl;

    auto m_sl=mSkipList<0,size_t,size_t>(4096,3,-1,0,0);
    auto s_m_sl=high_resolution_clock::now();
    for (size_t i = 0; i < v.size(); i++)
    {
        m_sl.insert(v[i],v[i]);
    }
    auto e_m_sl=high_resolution_clock::now();
    auto el_m_sl=duration_cast<milliseconds>(e_m_sl-s_m_sl);
    cout<<"mSkipList插入:"<<el_m_sl.count()<<"ms"<<endl;

    s_m_sl=high_resolution_clock::now();
    for (size_t i = 0; i < v.size(); i++)
    {
        m_sl.get<size_t>(v[i],0);
    }
    e_m_sl=high_resolution_clock::now();
    el_m_sl=duration_cast<milliseconds>(e_m_sl-s_m_sl);
    cout<<"mSkipList查找:"<<el_m_sl.count()<<"ms"<<endl;
    
    s_m_sl=high_resolution_clock::now();
    for (auto it=s.begin(); it!=s.end(); it++)
    {
        m_sl.erase(*it);
    }
    e_m_sl=high_resolution_clock::now();
    el_m_sl=duration_cast<milliseconds>(e_m_sl-s_m_sl);
    cout<<"mSkipList删除:"<<el_m_sl.count()<<"ms"<<endl;

    return 0;
}