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

template<Key K,typename V>
struct node{
    std::weak_ptr<node<K,V>> left;
    std::shared_ptr<node<K,V>> right;
    KV<K,V> data;
};

template<Key K,typename V>
class mSkipListView;

template<Key K,typename V>
class mSkipListModel{
private:
    std::shared_ptr<node<K,V>> first_;
    std::shared_ptr<node<K,V>> last_;
private:
    std::vector<std::weak_ptr<mSkipListView<K,V>>> views_;//使用该数据的视图
public:
    std::shared_ptr<mSkipListView> getView(int gap){
        std::shared_ptr<mSkipListView<K,V>> view = std::make_shared<mSkipListView<K,V>>(&first_,gap,inform);
        views_.push_back(view);
        return view;
    };
    void inform(node<K,V>* changedNode){
        for (auto it = views_.begin(); i != views_.end(); i++)
        {
            if((*it)->expired()){
                it = views_.erase(it)
                if(it==views_.end()) return;
            }
            (*it)->buildAndIndex(changedNode);
        }
    };
};

template<Key K,typename V>
struct v_node{
    std::weak_ptr<v_node<K,V>> left;
    std::vector<v_node<K,V>*> leftIndex;

    std::shared_ptr<v_node<K,V>> right;
    std::vector<v_node<K,V>*> rightIndex;

    std::shared_ptr<node<K,V>>* pnode=nullptr;
    std::shared_ptr<node<K,V>>& node(){ return (*pnode); };
    KV& data(){ (*node)->data; };
};

template<Key K,typename V>
class mSkipListView{
private:
    std::shared_ptr<v_node<K,V>> first_;
    std::shared_ptr<v_node<K,V>> last_;
    int gap;//两端具有下一层索引的节点中间有几个节点需要建立新的索引
    int leftAndMinGap(){ return gap%2==0?gap/2:gap/2+1; };//若达到新建缩引条件，则从左边节点到新的需要提升索引的节点需要右移几次
    int deep=0;//第0层表示原始数据，还未建立索引
    void (*inform)(ndoe* changedNode);
public:
    mSkipListView(std::shared_ptr<node<K,V>>* pfirst,int gap,void (*inform)(node<K,V>* changedNode)):
        gap(gap),inform(inform){
        first_->pnode=pfirst;
        
        last_=first_;
        while(true){
            if(last_->node()->right){
                initConnect(last_,new v_node<K,V>);
                last_=last_->right;
            }else(
                break;
            )
        }
    };
    ~mSkipListView()=default;
    mSkipListView(mSkipListView<K,V>& other)=delete;
    mSkipListView(mSkipListView<K,V>&& other)=delete;
    mSkipListView& operator=(const mSkipListView<K,V>& other)=delete;
    mSkipListView& operator=(mSkipListView<K,V>&& other)=delete;
private:
    void initConnect(std::shared_ptr<v_node<K,V>>& left,v_node<K,V> right){
        left->right=right;
        left->right->left=left;
    };
    void connect(v_node<K,V>* left,v_node<K,V>* right,int deep){
        
    };
    void buildAndIndex(node<K,V>* changedNode){

    };
public:
    const V& get(K key);
    void put(K key,V value);
    void del(K key);
};

#endif