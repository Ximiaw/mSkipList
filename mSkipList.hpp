#ifndef MSKIPLIST
#define MSKIPLIST

#include<iostream>//test

#include<cassert>
#include<iterator>
#include<memory>
#include<vector>
#include<concepts>
#include<type_traits>

template<typename K>
concept Key = std::semiregular<K>
    && std::totally_ordered<K>;

template<Key K,typename V>
struct KV{
    K key;
    V value;

    bool operator==(const KV& other) const { return key == other.key; }
    bool operator>(const KV& other) const { return key > other.key; }
    bool operator<(const KV& other) const { return key < other.key; }
};

template<Key K,typename V,bool isV=false>
struct node;

template<Key K,typename V>
struct node<K,V,false>{
    node<K,V>* left=nullptr;
    std::vector<node<K,V>*> leftIndex;
    
    node<K,V>* right=nullptr;
    std::vector<node<K,V>*> rightIndex;

    KV<K,V> kv;
    KV<K,V>& data() { return kv; };
};

template<Key K,typename V>
struct node<K,V,true>{
    node<K,V,true>* left=nullptr;
    std::vector<node<K,V,true>*> leftIndex;

    node<K,V,true>* right=nullptr;
    std::vector<node<K,V,true>*> rightIndex;

    node<K,V>** pnode=nullptr;
    KV<K,V>& data() { return (*pnode)->kv; };
};

template<Key K,typename V>
using v_node=node<K,V,true>;

//通知进行什么操作
enum class OPERATE{
    ADD,//原始数据先添加，而后视图更新
    DEL//视图先更新，而后原始数据删除
};

template<Key K,typename V,bool isV=false>
class mSkipList;

template<Key K,typename V>
class mSkipList<K,V,true>{
protected:
    v_node<K,V>* first_=nullptr;
    v_node<K,V>* last_=nullptr;

    int gap=3;//两端具有下一层索引的节点中间有几个节点需要建立新的索引
    int leftAndMidGap(){ return gap%2==0?gap/2:gap/2+1; };//若达到新建缩引条件，则从左边节点到新的需要提升索引的节点需要右移几次
    int deep=0;//表第deep+1层索引

};

template<Key K,typename V>
using mSkipList_view=mSkipList<K,V,true>;

/*
虽然可以视图和模型分离，但node的统一接口，使得模型自身带有一个视图，并且在这里的node会省下不少空间
多视图会导致节点变动时需要长时间重整各个视图的索引，因此不推荐多视图（当然如果完成会尝试改成多线程，使得视图不多的情况下仅需等待最长的视图更新）
允许改变间隙，但改变后直到下一次插入/删除可能改变附近的索引重建并不保证完全重建，如果需要请显示调用
*/
template<Key K,typename V>
class mSkipList<K,V,false>:public mSkipList_view<K,V>{
protected:
    node<K,V>* first_=nullptr;
    node<K,V>* last_=nullptr;

protected:
    std::vector<std::weak_ptr<mSkipList_view<K,V>>> views_;//使用该数据的视图

public:
    std::shared_ptr<mSkipList_view<K,V>> getView(int gap){
    };
    void inform(node<K,V>* node,OPERATE operate){
    };
};

#endif