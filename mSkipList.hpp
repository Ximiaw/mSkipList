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
    
    bool operator==(const K& other) const { return key == other; }
    bool operator>(const K& other) const { return key > other; }
    bool operator<(const K& other) const { return key < other; }
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
    int maxDeep=-2;//最高的层数
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
    //只构建当前层的索引，pnode是新增加的节点，deep为要构建的层数（righIndex[deep]）
    bool buildAndIndex(void* pnode,int deep){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        decltype(newNode()) left=nullptr;
        decltype(newNode()) right=nullptr;
        int nodeCount = traverseToIndexedChild(&left,&right,node,deep);
        if(nodeCount<gap+2) return false;
        nodeCount-=3;//去除头尾节点，和尾节点的左边节点，避免新建索引和right相邻
        int fre = nodeCount/leftToMidGap();
        decltype(newNode()) ptr=nullptr;
        for(int i=0;i<fre;++i){
            ptr = moveRight(left,deep);
            if(!ptr) return false;//计算好的循环，如果不够就是错误，但还好索引没断开
            if(!indexInsert(left,ptr,right,deep)) return false;//这个函数对层数非常敏感，但是经过traverseToIndexedChild后中间能遍历到的都是最高只有当前节点的
            left=ptr;
        }
        return true;
    }
    bool buildAndIndexNode(void* pnode){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        decltype(newNode()) left=nullptr;
        decltype(newNode()) right=nullptr;
        int nodeCount = traverseToIndexedChildNode(&left,&right,node);
        if(nodeCount<gap+2) return false;
        nodeCount-=3;//去除头尾节点，和尾节点的左边节点，避免新建索引和right相邻
        int fre = nodeCount/leftToMidGap();
        decltype(newNode()) ptr=nullptr;
        for(int i=0;i<fre;++i){
            ptr = moveRightNode(left);
            if(!ptr) return false;//计算好的循环，如果不够就是错误，但还好索引没断开
            if(!indexInsertNode(left,ptr,right)) return false;//这个函数对层数比较敏感，但是经过traverseToIndexedChild后中间能遍历到的都是最高只有当前节点的
            left=ptr;
        }
        return true;
    }
    //返回null*/node<K,V>*/v_node<K,V>*
    //位置如果查到则返回给节点指针，如果在first前面则返回first，否则返回间隙左边节点
    auto find(const K& key){
        if(!first()) return nullptr;
        if(first()->data()==key||first()->data()>key) return first();
        if(last()->data()==key||last()->data()<key) return last();
        int deep=first()->rightIndex.size()-1;
        decltype(newNode()) ptr=first();
        while (true)
        {
            if(deep==-1) break;
            if(ptr->rightIndex.size()<deep+1){
                --deep;
                continue;
            }
            if(ptr->data()==key) return ptr;
            if(ptr->rightIndex[deep]->data()<key){
                ptr=ptr->rightIndex[deep];
                continue;
            }else{
                --deep;
            }
        }
        while (true)
        {
            if(ptr->data()==key) return ptr;
            if(!ptr->right) return ptr;
            if(ptr->right->data()>key) return ptr;
            ptr=ptr->right;
        }
    }
    //只清理索引，其他不保证
    void clearIndex(void* pnode){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        node->leftIndex.clear();
        node->rightIndex.clear();
    }
    //这里pleft和pright是指针，这个两个形参需要传入二级指针，寻找pnode左右最近的有着下一层索引的节点
    //pnode是即将提升索引的节点，deep是pnode所在的层高，deep+1为pnode的左右索引的size
    //返回值是包含pnode的相邻两个有着下一层索引的节点中间的节点数
    int traverseToIndexedChild(void* ppleft,void* ppright,void* pnode,int deep){
        auto left=reinterpret_cast<decltype(&newNode())>(ppleft);
        auto right=reinterpret_cast<decltype(&newNode())>(ppright);
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!left||!right
            ||node->leftIndex.size()>deep+1
            ||node->rightIndex.size()>deep+1)
            return 0;
        decltype(newNode()) ptr_left=node;
        decltype(newNode()) ptr_right=node;
        int count=1;//下方while不会包含node本身这个计数
        while (true)
        {
            if(ptr_left->leftIndex.size()==deep+1){
                ptr_left=ptr_left->leftIndex[deep];
                ++count;
            }else if(ptr_left->leftIndex.size()<deep+1){
                return count;
            }
            if(ptr_right->rightIndex.size()==deep+1){
                ptr_right=ptr_right->rightIndex[deep];
                ++count;
            }else if(ptr_right->rightIndex.size()<deep+1){
                return count;
            }
            if(ptr_left->rightIndex.size()>deep+1&&ptr_right->leftIndex.size()>deep+1) break;
        }
        (*left)=ptr_left;
        (*right)=ptr_right;
        return count;
    }
    int traverseToIndexedChildNode(void* ppleft,void* ppright,void* pnode){
        auto left=reinterpret_cast<decltype(&newNode())>(ppleft);
        auto right=reinterpret_cast<decltype(&newNode())>(ppright);
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!left||!right
            ||!node->left
            ||!node->right)
            return 0;
        decltype(newNode()) ptr_left=node;
        decltype(newNode()) ptr_right=node;
        int count=1;//下方while不会包含node本身这个计数
        while (true)
        {
            if(ptr_left->leftIndex.size()==0){
                ptr_left=ptr_left->left;
                ++count;
            }else if(!ptr_left->left){
                return count;
            }
            if(ptr_right->rightIndex.size()==0){
                ptr_right=ptr_right->right;
                ++count;
            }else if(!ptr_right->right){
                return count;
            }
            if(ptr_left->rightIndex.size()>0&&ptr_right->leftIndex.size()>0) break;
        }
        (*left)=ptr_left;
        (*right)=ptr_right;
        return count;
    }
    //pleft为插入处左边第一个有着下一层索引的节点，因为首节点必定拥有所有层索引，所以moveLeft不再写
    auto moveRight(void* pleft,int deep){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        for(int i=0;i<leftToMidGap();++i){
            if(!left||left->rightIndex.size()<deep+1) return nullptr;
            left=left->rightIndex[deep];
        }
        return left;
    }
    auto moveRightNode(void* pleft){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        for(int i=0;i<leftToMidGap();++i){
            if(!left||!left->right) return nullptr;
            left=left->right;
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
    bool indexInsertNode(void* pleft,void* pmiddle,void* pright){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        auto middle=reinterpret_cast<decltype(newNode())>(pmiddle);
        auto right=reinterpret_cast<decltype(newNode())>(pright);
        if(!left||!middle||!right
            ||left->rightIndex.size()==0||right->leftIndex.size()==0
            ||middle->rightIndex.size()!=0||middle->leftIndex.size()!=0)
            return false;
        left->rightIndex[0]=middle;
        middle->leftIndex.push_back(left);
        right->leftIndex[0]=middle;
        middle->rightIndex.push_back(right);
        return true;
    }
    //关于索引连接，任意一层的任意两个节点，其指向两者中间方向的索引数组必然均有当前节点高度或均没有当前节点高度，不存在一边有一边没有的情况
    //在现有的节点上建立新连接，但不会处理中间节点和原始节点
    bool connectNewIndex(void* pleft,void* pright,int deep){//deep为所操控将节点的层数
        auto left=reinterpret_cast<decltype(newNode())>(pleft);//通过first确定调用着是否为子类（Derived不为nullptr_t），可以减少行数避免if constexpr
        auto right=reinterpret_cast<decltype(newNode())>(pright);
        if(!left||!right
            ||!(left->rightIndex.size()==deep+1&&right->leftIndex.size()==deep+1))
            return false;
        left->rightIndex.push_back(right);
        right->leftIndex.push_back(left);
        return true;
    };
    //在现有的节点上修改连接，但不会处理中间节点和原始节点
    bool connectIndex(void* pleft,void* pright,int deep){//deep为所要操控的层数0开始，比如在第0层修改索引则deep为0
        auto left=reinterpret_cast<decltype(newNode())>(pleft);//通过newNode确定调用着是否为子类（Derived不为nullptr_t），可以减少行数避免if constexpr
        auto right=reinterpret_cast<decltype(newNode())>(pright);//值的注意的是只有newNode是节点的指针，first和last是指针的引用
        if(!left||!right||left->rightIndex.size()<deep+1||right->leftIndex.size()<deep+1) return false;        
        left->rightIndex[deep]=right;
        right->leftIndex[deep]=left;
        return true;
    };
    //连接任意节点，但是不会处理中间节点和索引
    bool connectNode(void* pleft,void* pright){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        auto right=reinterpret_cast<decltype(newNode())>(pright);
        if(!left||!right) return false;
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
        delete new_ptr;
        return nullptr;
    };
    auto rightInsert(void* pnode,void* pkv){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!node->right) return nullptr;
        decltype(newNode()) ptr = node->right;
        auto new_ptr = newNode();
        loadData(new_ptr,pkv);
        if(connectNode(node,new_ptr)&&connectNode(new_ptr,ptr)) return new_ptr;
        delete new_ptr;
        return nullptr;
    };
protected:
    //插入节点后处理索引，node是新添加的节点的指针，调用方需要保证没有其他节点和其重复
    bool lInsertBuildAndIndex(node<K,V>* node){
        decltype(newNode()) ptr=nullptr;
        decltype(newNode()) left=nullptr;
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            if(!last()){
                if(lastRightInsert(&node->data())) return true;
                else return false;
            }
            ptr=lastRightInsert(&node->data());
        }else{
            //这里没办法拿到二级指针，只能通过CRTP拿
            decltype(&newNode()) pptr=&static_cast<Derived*>(this)->last();
            if(!last()){
                if(lastRightInsert(pptr)) return true;
                else return false;
            }
            ptr=lastRightInsert(pptr);
        }
        left=ptr->left;
        return mInsertBuildAndIndex(node->left);//检测左边边旧的last节点是否达到需要构建索引的程度
    }
    bool fInsertBuildAndIndex(node<K,V>* node){
        decltype(newNode()) ptr=nullptr;
        decltype(newNode()) right=nullptr;
        int deep=first()->rightIndex.size()-1;
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            if(!first()){
                if(firstLeftInsert(&node->data())) return true;
                else return false;
            }
            ptr=firstLeftInsert(&node->data());
        }else{
            //这里没办法拿到二级指针，只能通过CRTP拿
            decltype(&newNode()) pptr=&static_cast<Derived*>(this)->first();
            if(!first()){
                if(firstLeftInsert(pptr)) return true;
                else return false;
            }
            ptr = firstLeftInsert(pptr);
        }
        right=ptr->right;
        for(int i=0;i<=deep;++i){
            if(!connectNewIndex(ptr,right,i)) return false;
            connectNode(ptr,right->rightIndex[i]);
        }
        clearIndex(right);
        return mInsertBuildAndIndex(node->right);//检测右边旧的first节点是否达到需要构建索引的程度
    }
    //插入节点后处理索引，node是新添加的节点的指针
    bool mInsertBuildAndIndex(node<K,V>* node){
        if(!node||!node->left||!node->right) return false;
        decltype(newNode()) left=nullptr;
        decltype(newNode()) right=nullptr;
        decltype(newNode()) ptr=nullptr;
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,Derived>){
            left=ptr=node;
            if(left->data()==node->data()){
                left->data().value=node->data().value;
                return true;
            }
        }else{
            left=find(node->data());
            if(left->data()==node->data()){
                left->data().value=node->data().value;
                return true;
            }
            if(!(left->data()<node->data())) return false;
            //关于&node->left->right
            //视图里面需要插入node<K,V>**，而数据层节点在数据模型处已经插入并连接
            //因此这里node的左边有着node<K,V>*
            //为了拿到node的可信任二级指针，因此拿左节点的右指针（是指向node的node<K,V>*)
            ptr=rightInsert(left,&node->left->right);
        }
        auto base=ptr;//记录以方便后续向上层建立索引

        //从最低点查询是否建立节点，如果满足条件则建立合适节点的第零层索引
        int count=traverseToIndexedChildNode(&left,&right,ptr);
        if(count<gap+2) return false;
        count-=3;
        int fre=count/leftToMidGap();
        for(int i=0;i<fre;++i){
            ptr=moveRightNode(left);
            if(!ptr) return false;
            if(!indexInsertNode(left,ptr,right)) return false;
            left=ptr;
        }
        //下面会一直建立新的索引，不跑最上层是因为将那些工作留给topBuildAndIndex以简化逻辑
        ptr=base;
        for (int deep = 0; deep < first()->rightIndex.size()-1; deep++)
        {
            count=traverseToIndexedChild(&left,&right,ptr,deep);
            if(count<gap+2) break;
            count-=3;
            int fre=count/leftToMidGap();
            for(int i=0;i<fre;++i){
                ptr=moveRight(left);
                if(!ptr) return false;
                if(!indexInsert(left,ptr,right,deep)) return false;
                left=ptr;
            }
            ptr=base;
        }
        return topBuildAndIndex();
    }
    //在最顶层向上构建索引，无论是顶层索引还是从原始数据开始
    bool topBuildAndIndex(){
        if(maxDeep==first()->rightIndex.size()-1) return false;
        //first为nullptr
        if(!first()) return true;
        int count=0;
        decltype(newNode()) pleft=nullptr;
        decltype(newNode()) pright=nullptr;
        decltype(newNode()) ptr=nullptr;
        while (true)
        {
            if(maxDeep==first()->rightIndex.size()-1) return false;
            int deep=first()->rightIndex.size()-1;
            if(deep==-1){
                //没有上层索引
                count=traverseToIndexedChildNode(&pleft,&pright,first());
                if(count<gap+2) break;
                count-=1;//这里-1是去除首节点，因为是原始数据因此末尾可以建立索引
                int fre=count/leftToMidGap();
                for(int i=0;i<fre;++i){
                    ptr=moveRightNode(pleft);
                    if(!ptr) return false;
                    if(!connectNewIndex(pleft,ptr,-1)) return false;
                    pleft=ptr;
                }
            }else{
                //至少有着一层索引
                count=traverseToIndexedChild(&pleft,&pright,first(),deep);
                if(count<gap+2) break;
                count-=1;//这里-1是去除首节点，因为是原始数据因此末尾可以建立索引
                int fre=count/leftToMidGap();
                for(int i=0;i<fre;++i){
                    ptr=moveRight(pleft);
                    if(!ptr) return false;
                    if(!connectNewIndex(pleft,ptr,deep)) return false;
                    pleft=ptr;
                }
            }
        }
        return true;
    }

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