#ifndef MSKIPLIST
#define MSKIPLIST

//#include<iostream>//test

//#include<cassert>
//#include<iterator>
#include<memory>
#include<vector>
#include<concepts>
#include<stdexcept>
//#include<type_traits>

template<typename T>
concept K = std::semiregular<T>
    && std::totally_ordered<T>;

template<K K,typename V>
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

template<K K,typename V,bool isV=false>
struct node;

template<K K,typename V>
struct node<K,V,false>{
    node<K,V>* left=nullptr;
    std::vector<node<K,V>*> leftIndex;
    
    node<K,V>* right=nullptr;
    std::vector<node<K,V>*> rightIndex;

    KV<K,V>& data() { return kv; };
private:
    KV<K,V> kv;
};

template<K K,typename V>
struct node<K,V,true>{
    node<K,V,true>* left=nullptr;
    std::vector<node<K,V,true>*> leftIndex;

    node<K,V,true>* right=nullptr;
    std::vector<node<K,V,true>*> rightIndex;

    KV<K,V>& data() { return (*pnode)->kv; };
    
    node<K,V>** pnode=nullptr;//该指针只允许模型类在其独有逻辑允许使用，或视图类的条件编译使用
};

template<K K,typename V>
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

template<K K,typename V,bool isV=false,typename Derived=nullptr_t>
class mSkipList;

/*
这里是视图
*/
template<K K,typename V,typename Derived>
class mSkipList<K,V,true,Derived>{
protected:
    v_node<K,V>* first_=nullptr;
    v_node<K,V>* last_=nullptr;
    long long length=0;
    int maxDeep_=-2;//最高的层数
    int gap=3;//两端具有下一层索引的节点中间有几个节点需要建立新的索引
    int leftToMidGap(){ return gap%2==0?gap/2:gap/2+1; };//若达到新建缩引条件，则从左边节点到新的需要提升索引的节点需要右移几次
    mSkipList<K,V>* base=nullptr;
    //int deep=0;//表第deep+1层索引，没什么用可以通过first的rightIndex读到，不使用这个类变量还可以少维护一个东西
protected:
    auto& first() { 
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,decltype(*base)>){
            return base->first_;
        }else{
            return first_;
        }
    };
    auto& last() { 
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,decltype(*base)>){
            return base->last_;
        }else{
            return last_;
        }
    };
    auto newNode() { //decltype只有他能提供指针语义，上面两个是引用
        if constexpr(std::is_base_of_v<mSkipList<K,V,true,Derived>,decltype(*base)>){
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
    //todo 继续写工具函数
    //穿透的由底向上的索引构建
    //某一区间的索引检查并构建合法索引
    //头插/尾插对索引的特殊情况处理
    //头/尾插删的函数

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
    };
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
    };
    //返回null*/node<K,V>*/v_node<K,V>*
    //位置如果查到则返回给节点指针，如果在first前面则返回first，否则返回间隙左边节点
    auto find(const K& key) -> decltype(newNode()){
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
    };
    //只清理索引，其他不保证
    void clearIndex(void* pnode){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        node->leftIndex.clear();
        node->rightIndex.clear();
    };
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
    };
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
    };
    //pleft为插入处左边第一个有着下一层索引的节点，因为首节点必定拥有所有层索引，所以moveLeft不再写
    auto moveRight(void* pleft,int deep) -> decltype(newNode()){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        for(int i=0;i<leftToMidGap();++i){
            if(!left||left->rightIndex.size()<deep+1) return nullptr;
            left=left->rightIndex[deep];
        }
        return left;
    };
    auto moveRightNode(void* pleft) -> decltype(newNode()){
        auto left=reinterpret_cast<decltype(newNode())>(pleft);
        for(int i=0;i<leftToMidGap();++i){
            if(!left||!left->right) return nullptr;
            left=left->right;
        }
        return left;
    };
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
    };
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
    };
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
    auto firstLeftInsert(void* pkv) -> decltype(newNode()){
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
    auto lastRightInsert(void* pkv) -> decltype(newNode()){
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
    auto leftInsert(void* pnode,void* pkv) -> decltype(newNode()){
        auto node=reinterpret_cast<decltype(newNode())>(pnode);
        if(!node||!node->left) return nullptr;
        decltype(newNode()) ptr = node->left;
        auto new_ptr = newNode();
        loadData(new_ptr,pkv);
        if(connectNode(ptr,new_ptr)&&connectNode(new_ptr,node)) return new_ptr;
        delete new_ptr;
        return nullptr;
    };
    auto rightInsert(void* pnode,void* pkv) -> decltype(newNode()){
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
    bool deleteNodeAndIndex(node<K,V>* node){
        //todo
        return true;
    };
    //插入节点后处理索引，node是新添加的节点的指针
    bool insertNodeAndIndex(node<K,V>* node){
        //todo
        return true;
    };
    //在最顶层向上构建索引，无论是顶层索引还是从原始数据开始
    bool topBuildAndIndex(){
        if(maxDeep_==first()->rightIndex.size()-1) return false;
        //first为nullptr
        if(!first()) return true;
        int count=0;
        decltype(newNode()) pleft=nullptr;
        decltype(newNode()) pright=nullptr;
        decltype(newNode()) ptr=nullptr;
        while (true)
        {
            if(maxDeep_==first()->rightIndex.size()-1) return false;
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
    };

public:
    const V& get(const K& key){
        auto node = find(key);
        if(node&&node->data()==key) return node->data().value;
        throw std::runtime_error("key not found.");
    };
    const V& get(const K&& key){
        auto node = find(key);
        if(node&&node->data()==key) return node->data().value;
        throw std::runtime_error("key not found.");
    };
    void put(K& key,V& value){
        if(first()->data()>key)
            base->task(first(),LOCATION::LEFT,OPERATE::ADD,KV{key,value});
        else if(first()->data()==key)
            base->task(first(),LOCATION::MIDDLE,OPERATE::ADD,KV{key,value});
        else if(last()->data()==key)
            base->task(last(),LOCATION::MIDDLE,OPERATE::ADD,KV{key,value});
        else if(last()->data()<key)
            base->task(last(),LOCATION::RIGHT,OPERATE::ADD,KV{key,value});
        else
            base->task(find(key),LOCATION::RIGHT,OPERATE::ADD,KV{key,value});
    };
    void put(K&& key,V&& value){
        if(first()->data()>key)
            base->task(first(),LOCATION::LEFT,OPERATE::ADD,KV{key,value});
        else if(first()->data()==key)
            base->task(first(),LOCATION::MIDDLE,OPERATE::ADD,KV{key,value});
        else if(last()->data()==key)
            base->task(last(),LOCATION::MIDDLE,OPERATE::ADD,KV{key,value});
        else if(last()->data()<key)
            base->task(last(),LOCATION::RIGHT,OPERATE::ADD,KV{key,value});
        else
            base->task(find(key),LOCATION::RIGHT,OPERATE::ADD,KV{key,value});
    };
    void del(const K& key){
        auto node = find(key);
        if(node&&node->data()==key)
            base->task(node,LOCATION::MIDDLE,OPERATE::DEL,KV{key});
    }
    bool exists(const K& key){
        auto node = find(key);
        if(node&&node->data()==key) return true;
        return false;
    };
    bool exists(const K&& key){
        auto node = find(key);
        if(node&&node->data()==key) return true;
        return false;
    };
    const long long size(){
        return length;
    };
    const int deep(){
        return first()->rightIndex.size();
    };
    const int maxDeep(){
        return maxDeep_;
    };
    void setMaxDeep(int max){
        maxDeep_=max-1;
        decltype(newNode()) ptr=first();
        if(!ptr) return;
        while (true)
        {
            if(!ptr->right) break;
            if(ptr->leftIndex.size()>maxDeep_+1) ptr->leftIndex.resize(maxDeep_+1);
            if(ptr->rightIndex.size()>maxDeep_+1) ptr->rightIndex.resize(maxDeep_+1);
            ptr=ptr->right;
        }
        if(ptr->leftIndex.size()>maxDeep_+1) ptr->leftIndex.resize(maxDeep_+1);
        if(ptr->rightIndex.size()>maxDeep_+1) ptr->rightIndex.resize(maxDeep_+1);
    };
    void viewTask(node<K,V>* node,OPERATE operate){
        if(operate==OPERATE::ADD){
            //node是新添加的节点，需要给他建立索引
            insertNodeAndIndex(node);
            ++length;
        }else if(operate==OPERATE::DEL){
            //node是将删除的节点，清理他的索引
            deleteNodeAndIndex(node);
            --length;
        }
    };
public:
    mSkipList(int gap,mSkipList<K,V>* base):gap(gap),base(base){};
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
public:
    //todo 迭代器的失效约定
    class iterator{
    private:
        void* first_;
        void* last_;
        void* ptr_;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = KV<K,V>;
        using difference_type = std::ptrdiff_t;
        using pointer = KV<K,V>*;
        using reference = KV<K,V>&;
        iterator()=delete;
        explicit iterator(void* start,void* last):first_(start),last_(last),ptr_(start){};
        explicit iterator(void* start,void* last,void* ptr):first_(start),last_(last),ptr_(ptr){};
        reference operator*() const{
            auto ptr=reinterpret_cast<decltype(newNode())>(ptr_);
            return ptr->data();
        };
        pointer operator->() const{
            auto ptr=reinterpret_cast<decltype(newNode())>(ptr_);
            return &ptr->data();
        };
        iterator& operator++(){ 
            auto ptr=reinterpret_cast<decltype(newNode())>(ptr_);
            if(ptr&&ptr->right) ptr=ptr->right; 
            else ptr=nullptr;
            ptr_=ptr;
            return *this;
        };
        iterator operator++(int){ 
            iterator old{first_,last_,ptr_};
            auto ptr=reinterpret_cast<decltype(newNode())>(ptr_); 
            if(ptr&&ptr->right) ptr=ptr->right;
            else ptr=nullptr;
            ptr_=ptr;
            return old; 
        };
        iterator& operator--(){
            auto ptr=reinterpret_cast<decltype(newNode())>(ptr_);
            if(ptr&&ptr->left) ptr=ptr->left; 
            else ptr=nullptr; 
            ptr_=ptr;
            return *this; 
        };
        iterator operator--(int){
            iterator old{first_,last_,ptr_};
            auto ptr=reinterpret_cast<decltype(newNode())>(ptr_);
            if(ptr&&ptr->left) ptr=ptr->left; 
            else ptr=nullptr; 
            ptr_=ptr;
            return old; 
        };
        bool operator==(const iterator& other) const{ 
            auto a=reinterpret_cast<decltype(newNode())>(ptr_);
            auto b=reinterpret_cast<decltype(newNode())>(other.ptr_);
            return a->data()==b->data(); 
        };
        bool operator!=(const iterator& other) const{
            auto a=reinterpret_cast<decltype(newNode())>(ptr_);
            auto b=reinterpret_cast<decltype(newNode())>(other.ptr_);
            return a->data()!=b->data();
        };
    };
    iterator begin(){
        return iterator{first(),last(),first()};
    };
    iterator end(){
        return iterator{first(),last(),nullptr};
    };
    iterator rbegin(){
        return iterator{first(),last(),last()};
    };
    iterator rend(){
        return iterator{first(),last(),nullptr};
    };
};

template<K K,typename V>
using mSkipList_view=mSkipList<K,V,true,nullptr_t>;

/*
虽然可以视图和模型分离，但node的统一接口，使得模型自身带有一个视图，并且在这里的node会省下不少空间
多视图会导致节点变动时需要长时间重整各个视图的索引，因此不推荐多视图（当然如果完成会尝试改成多线程，使得视图不多的情况下仅需等待最长的视图更新）
允许改变间隙，但改变后直到下一次插入/删除可能改变附近的索引重建并不保证完全重建，如果需要请显示调用
*/
template<K K,typename V>
class mSkipList<K,V>:public mSkipList<K,V,true,mSkipList<K,V>>{
protected:
    friend class mSkipList<K,V,true,mSkipList<K,V>>;
    node<K,V>* first_=nullptr;
    node<K,V>* last_=nullptr;
protected:
    std::vector<std::shared_ptr<mSkipList_view<K,V>>> views_;//使用该数据的视图

//这里的移动和拷贝应该允许，但为方便先全部删除
public:
    mSkipList(int gap=3):mSkipList<K,V,true,mSkipList<K,V>>(gap,this){};
    virtual ~mSkipList() {
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
        views_.push_back(std::shared_ptr<mSkipList_view<K,V>>{new mSkipList_view<K,V>{gap,this}});
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
        this->viewTask(node,operate);//自身类型和其他视图类型不一样，这导致需要单独调用
        for(auto& view:views_){
            view->viewTask(node,operate);
        }
    };

    //在node的location方向，添加/删除/修改一个KV为kv的新节点
    //如果location为middle，则指node本身，如果同时为ADD则是修改该节点的将KV值
    //多视图导致的开销，使得在有外部视图时，需要find两次，多了一次查询
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