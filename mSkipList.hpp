#ifndef MSKIPLIST
#define MSKIPLIST

//#include<iostream>//test

//#include<cassert>
//#include<iterator>
#include<memory>
#include<vector>
#include<concepts>
//#include<type_traits>

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
    
    node<K,V>** pnode=nullptr;//该指针只允许模型类在其独有逻辑允许使用，或视图类的条件编译使用
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
    int gap=3;//两端具有下一层索引的节点中间有几个节点需要建立新的索引
    int leftToMidGap(){ return gap%2==0?gap/2:gap/2+1; };//若达到新建缩引条件，则从左边节点到新的需要提升索引的节点需要右移几次
    //int deep=0;//表第deep+1层索引，没什么用可以通过first的rightIndex读到，不使用这个类变量还可以少维护一个东西
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
    auto newNode() { //decltype只有他能提供指针语义，上面两个是引用
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            return new node<K,V>;
        }else{
            return new v_node<K,V>;
        }
    };
    void loadData(void* new_ptr,void* pkv){
        decltype(newNode()) node = reinterpret_cast<decltype(newNode())>(new_ptr);
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            auto kv = reinterpret_cast<KV<K,V>*>(pkv);
            node->data()=*kv;
        }else{
            //v_node的pnode是node<K,V>**
            auto kv = reinterpret_cast<decltype(first()->pnode)>(pkv);
            node->pnode=kv;
        }
    };
protected:
    //pleft为插入处左边第一个有着下一层索引的节点
    auto moveRight(void* pleft,int deep){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        for(int i=0;i<leftToMidGap();++i){
            if(!left||left->rightIndex.size()<deep+1) return nullptr;
            left=left->rightIndex[deep];
        }
        return left;
    }
    //pleft和pright是有着下一层节点的指针，他们在这一层（下一层）是相邻的
    //pmiddle是需要插入到下一层索引的节点，middleDeep为pmiddle的索引最高层，middleDeep+1为pmiddle的左右索引的size
    bool indexInsert(void* pleft,void* pmiddle,void* pright,int middleDeep){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        auto middle=reinterpret_cast<decltype(newNode())>(pmiddle);
        auto right=reinterpret_cast<decltype(newNode())>(pright);
        if(!left||!middle||!right
            ||left->rightIndex.size()<=middleDeep+1||right->leftIndex.size()<=middleDeep+1
            ||left->rightIndex[middleDeep+1]!=right||right->leftIndex[middleDeep+1]!=left
            ||!(middle->rightIndex.size()==middleDeep+1)||!(middle->leftIndex.size()==middleDeep+1))
            return false;
        left->rightIndex[middleDeep+1]=middle;
        middle->leftIndex.push_back(left);
        right->leftIndex[middleDeep+1]=middle;
        middle->rightIndex.push_back(right);
        return true;
    }
    //关于索引连接，任意一层的任意两个节点，其指向两者中间方向的索引数组必然均有当前节点高度或均没有当前节点高度，不存在一边有一边没有的情况
    //在现有的节点上建立新连接，如果中间有节点不会处理
    bool connectNewIndex(void* pleft,void* pright,int deep){//deep为所要操控的层数0开始，比如在第0层修改索引则deep为0
        auto left=reinterpret_cast<decltype(newNode())>(pleft);//通过first确定调用着是否为子类（Derived不为nullptr_t），可以减少行数避免if constexpr
        auto right=reinterpret_cast<decltype(newNode())>(pright);
        if(!left||!right||left->rightIndex.size()>=deep+1||right->left.size()>=deep+1) return false;
        left->rightIndex.push_back(right);
        right->leftIndex.push_back(left);
        return true;
    };
    //在现有的节点上修改连接，如果中间有节点不会处理
    bool connectIndex(void* pleft,void* pright,int deep){//deep为所要操控的层数0开始，比如在第0层修改索引则deep为0
        auto left=reinterpret_cast<decltype(newNode())>(pleft);//通过newNode确定调用着是否为子类（Derived不为nullptr_t），可以减少行数避免if constexpr
        auto right=reinterpret_cast<decltype(newNode())>(pright);//值的注意的是只有newNode是节点的指针，first和last是指针的引用
        if(!left||!right||left->rightIndex.size()<deep+1||right->left.size()<deep+1) return false;        
        left->rightIndex[deep]=right;
        right->leftIndex[deep]=left;
        return true;
    };
    //连接任意节点，但是不会处理中间节点
    bool connectNode(void* pleft,void* pright){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        auto right=reinterpret_cast<decltype(newNode())>(pright);
        if(!left||!right||!left->right||!right->left) return false;    
        left->right=right;
        right->left=left;
        return true;
    };
    //pnode为中间节点的指针，类型为node<K,V>或v_node<K,V>
    bool delNode(void* pnode){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!node->left||!node->right) return false;
        node->left->right=node->right;
        node->right->left=node->left;
        delete node;
        return true;
    };
    auto firstLeftInsert(void* pkv){
        auto new_ptr = newNode();
        loadData(new_ptr,pkv);
        if(first()){
            if(!connectNode(new_ptr,first())){
                delete new_ptr;
                return nullptr;
            };
            first() = new_ptr;
            return new_ptr;
        }else{
            first() = new_ptr;
            last() = new_ptr;
            return new_ptr;
        }
    };
    auto lastRightInsert(void* pkv){
        auto new_ptr = newNode();
        loadData(new_ptr,pkv);
        if(last()){
            if(!connectNode(last(),new_ptr)){
                delete new_ptr;
                return nullptr;
            }
            last() = new_ptr;
            return new_ptr;
        }else{
            first() = new_ptr;
            last() = new_ptr;
            return new_ptr;  
        }
    };
    //kv可能是KV*或者node<K,V>**
    //指针长度为代在一个计算机内固定长度，无论几重指针，也就是kv为node<K,V>**时，可以认为kv是node<K,V>*的指针
    auto leftInsert(void* pnode,void* pkv){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!node->left) return nullptr;
        decltype(newNode()) ptr = node->left;
        auto new_ptr = newNode();
        loadData(new_ptr,pkv);
        if(connectNode(ptr,new_ptr)&&connectNode(new_ptr,node)) return new_ptr;
        return nullptr;
    };
    auto rightInsert(void* pnode,void* pkv){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!node->right) return nullptr;
        decltype(newNode()) ptr = node->right;
        auto new_ptr = newNode();
        loadData(new_ptr,pkv);
        if(connectNode(node,new_ptr)&&connectNode(new_ptr,ptr)) return new_ptr;
        return nullptr;
    };

public:
    void task(node<K,V>* node,OPERATE operate){
        if(operate==OPERATE::ADD){
            //node是新添加的节点，需要给他建立索引

        }else if(operate==OPERATE::DEL){
            //node是将删除的节点，清理他的索引

        }
    };
public:
    mSkipList(int gap):gap(gap){};
    virtual ~mSkipList(){
        if(!first()) return;
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
        this->task(node,operate);//自身类型和其他视图类型不一样，这导致需要单独调用
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
            this->delNode(node);
        }else if(location==LOCATION::LEFT&&operate==OPERATE::ADD){
            auto newNode = this->leftInsert(node,&kv);
            inform(newNode,operate);
        }else if(location==LOCATION::RIGHT&&operate==OPERATE::ADD){
            auto newNode = this->rightInsert(node,&kv);
            inform(newNode,operate);
        }
    };
};

#endif