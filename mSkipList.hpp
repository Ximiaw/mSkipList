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

    KV<K,V>& data() { return kv; };
private:
    KV<K,V> kv;
};

template<Key K,typename V>
struct node<K,V,true>{
    node<K,V,true>* left=nullptr;
    std::vector<node<K,V,true>*> leftIndex;

    node<K,V,true>* right=nullptr;
    std::vector<node<K,V,true>*> rightIndex;

    KV<K,V>& data() { return (*pnode)->kv; };
    
    node<K,V>** pnode=nullptr;//该指针只允许模型类在其独有逻辑允许使用
};

template<Key K,typename V>
using v_node=node<K,V,true>;

//通知进行什么操作
enum class OPERATE{
    ADD,
    DEL
};

//表示位置，在通知数据模型是要用到
enum class LOCATION{
    LEFT,
    MIDDLE,
    RIGHT
};

template<Key K,typename V,bool isV=false,typename Derived=nullptr_t>
class mSkipList;

/*
这里是视图
*/
template<Key K,typename V,typename Derived>
class mSkipList<K,V,true,Derived>{
protected:
    v_node<K,V>* first_=nullptr;
    v_node<K,V>* last_=nullptr;
protected:
    auto& first() { 
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            return static_cast<Derived*>(this)->first_;
        }else{
            return first_;
        }
    };
    auto& last() { 
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            return static_cast<Derived*>(this)->last_;
        }else{
            return last_;
        }
    };
    auto newNode() { 
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            return new node<K,V>;
        }else{
            return new v_node<K,V>;
        }
    };
protected:
    auto leftConnectRight(void* pleft,void* pright){
        auto left=reinterpret_cast<decltype(first())>(pleft);//通过first确定调用着是否为子类（Derived不为nullptr_t），可以减少行数避免if constexpr
        auto right=reinterpret_cast<decltype(first())>(pright);

    };
    //pnode为节点指针，类型为node<K,V>或v_node<K,V>
    auto leftConnectRight(void* pnode){
        auto node=reinterpret_cast<decltype(first())>(pnode);

    };
    auto leftInsert(void* pnode,const KV<K,V>& kv){
        auto node=reinterpret_cast<decltype(first())>(pnode);

    };
    auto rightInsert(void* pnode,const KV<K,V>& kv){
        auto node=reinterpret_cast<decltype(first())>(pnode);

    };
public:
    void task(node<K,V>* node,OPERATE operate){

    };
protected:
    int gap=3;//两端具有下一层索引的节点中间有几个节点需要建立新的索引
    int leftAndMidGap(){ return gap%2==0?gap/2:gap/2+1; };//若达到新建缩引条件，则从左边节点到新的需要提升索引的节点需要右移几次
    int deep=0;//表第deep+1层索引
public:
    mSkipList(int gap):gap(gap){};
    virtual ~mSkipList(){
        while (first()!=last())
        {
            first() = first()->right;
            delete first()->left;
        }
        delete first();
        first()=nullptr;
        last()=nullptr;
    };
    mSkipList(const mSkipList<K,V,true>&)=delete;
    mSkipList(mSkipList<K,V,true>&&)=delete;
    mSkipList<K,V,true>& operator=(const mSkipList<K,V,true>&)=delete;
    mSkipList<K,V,true>& operator=(mSkipList<K,V,true>&&)=delete;
};

template<Key K,typename V>
using mSkipList_view=mSkipList<K,V,true>;

/*
虽然可以视图和模型分离，但node的统一接口，使得模型自身带有一个视图，并且在这里的node会省下不少空间
多视图会导致节点变动时需要长时间重整各个视图的索引，因此不推荐多视图（当然如果完成会尝试改成多线程，使得视图不多的情况下仅需等待最长的视图更新）
允许改变间隙，但改变后直到下一次插入/删除可能改变附近的索引重建并不保证完全重建，如果需要请显示调用
*/
template<Key K,typename V>
class mSkipList<K,V>:public mSkipList<K,V,true,mSkipList<K,V>>{
protected:
    friend class mSkipList<K,V,true,mSkipList<K,V>>;
    node<K,V>* first_=nullptr;
    node<K,V>* last_=nullptr;
protected:
    std::vector<std::shared_ptr<mSkipList_view<K,V>>> views_;//使用该数据的视图

//这里的移动和拷贝应该允许，但为方便先全部删除
public:
    mSkipList(int gap=3):mSkipList<K,V,true,mSkipList<K,V>>(gap){};
    ~mSkipList() override{
        views_.clear();
        if(!this->first()) return;
        while (this->first()!=this->last())
        {
            this->first() = this->first()->right;
            delete this->first()->left;
        }
        delete this->first();
        this->first()=nullptr;
        this->last()=nullptr;
    };
    mSkipList(const mSkipList<K,V>&)=delete;
    mSkipList(mSkipList<K,V>&&)=delete;
    mSkipList<K,V>& operator=(const mSkipList<K,V>&)=delete;
    mSkipList<K,V>& operator=(mSkipList<K,V>&&)=delete;

public:
    std::weak_ptr<mSkipList_view<K,V>> getView(int gap){
        views_.push_back(std::shared_ptr<mSkipList_view<K,V>>{new mSkipList_view<K,V>{gap}});
        return views_.back();
    };
    void delView(std::weak_ptr<mSkipList_view<K,V>>& view){
        auto shared=view.lock();
        if(!shared) return;
        std::erase_if(views_,[&shared](std::shared_ptr<mSkipList_view<K,V>>& view){ return shared==view; });
    };

    //通知各个视图处理节点变化后的操作
    //ADD 原始数据先添加，而后视图更新
    //DEL 视图先更新，而后原始数据删除
    void inform(node<K,V>* node,OPERATE operate){
        this->task(node,operate);
        for(auto& view:views_){
            view->task(node,operate);
        }
    };

    //在node的location方向，添加/删除/修改一个KV为kv的新节点
    //如果location为middle，则指node本身，如果同时为ADD则是修改该节点的将KV值
    void task(v_node<K,V>* v_node,LOCATION location,OPERATE operate,KV<K,V> kv){
        auto node=*v_node->pnode;
        task(node,location,operate,kv);
    };
    void task(node<K,V>* node,LOCATION location,OPERATE operate,KV<K,V> kv){
        if(location==LOCATION::MIDDLE&&operate==OPERATE::ADD){
            node->data().value=kv.value;
        }else if(location==LOCATION::MIDDLE&&operate==OPERATE::DEL){
            inform(node,operate);
            this->leftConnectRight(node);
            delete node;
        }else if(location==LOCATION::LEFT&&operate==OPERATE::ADD){
            auto newNode = this->leftInsert(node,kv);
            inform(newNode,operate);
        }else if(location==LOCATION::RIGHT&&operate==OPERATE::ADD){
            auto newNode = this->rightInsert(node,kv);
            inform(newNode,operate);
        }
    };
};

#endif