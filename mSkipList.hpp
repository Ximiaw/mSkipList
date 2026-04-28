#ifndef MSKIPLIST
#define MSKIPLIST

#include<iostream>//test

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
    bool operator<(const KV& other) const { return key < other.key; }//是多余了，但是不写对称有点难受
};

template<Key K,typename V>
class mSkipList{
public:
    struct node{
        std::weak_ptr<node> left;
        std::shared_ptr<node> right;
        KV<K,V> data;
    }
private:
    
};


#endif