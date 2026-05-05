#ifndef MSKIPLIST
#define MSKIPLIST

#include<vector>
#include<array>
#include<memory>
#include<concepts>
#include<tuple>

template<typename T>
concept Key=std::semiregular<T>&&std::totally_ordered<T>;//视图类里面用，判断所选key是否满足需求

template<typename... T_D>
class Data{
private:
    std::tuple<T_D...> data_;
    void* ptrs_[sizeof...(T_D)];
    template<int... Is>
    void init(std::index_sequence<Is...>){
        ((ptrs_[Is] = static_cast<void*>(&std::get<Is>(data_))), ...);
    };
public:
    explicit Data(T_D... args):data_(std::forward<T_D>(args)...){
        init(std::index_sequence_for<T_D...>{});
    };
    std::tuple<T_D...>& data(){ 
        return data_;
    };
    template<typename T>
    T& ref(int i) {
        assert(i >= 0 && i < sizeof...(T_D) && "Index out of bounds");
        auto* ptr = static_cast<T*>(ptrs_[i]);
        assert(ptr != nullptr && "Null pointer");
        return *ptr; 
    };
    template<size_t i>
    auto& get(){
        static_assert(i < sizeof...(T_D), "Index out of range");
        return std::get<i>(data_);
    };
};

template<typename T>
using nodeIndexList=std::vector<T*>;

template<typename... T_D>
struct V_Node;

template<typename... T_D>
struct Node{
    nodeIndexList<Node<T_D...>> leftIndex;
    nodeIndexList<Node<T_D...>> rightIndex;
    nodeIndexList<V_Node<T_D...>> v_node;//重建索引时，获取视图，减少一次查询
    Data<T_D...> data_;
    Data<T_D...>& data(){ return data_; };//禁止修改当前主键，如果改到其他主键则通知视图删除节点，然后重新插入
};

template<typename... T_D>
struct V_Node{
    nodeIndexList<V_Node<T_D...>> leftIndex;
    nodeIndexList<V_Node<T_D...>> rightIndex;
    Node<T_D...>* node=nullptr;
    Data<T_D...>& data(){ return node->data(); };//可能报错，但是如果对应数据节点不存在则该视图不应该存在
};

template<typename T,typename... T_D>
concept NodeBase=requires(T* node)
{
    {node->leftIndex}->std::same_as<nodeIndexList<T>>;
    {node->rightIndex}->std::same_as<nodeIndexList<T>>;
    {node->data()}->std::same_as<Data<T_D...>&>;
};

template<typename T,int keyIndex,typename... T_D> requires NodeBase<T,T_D...>
class Algorithm{
public:
    T* first=nullptr;//头尾添加两个哨兵节点，通过指针判断，使得算法简化为只需处理中间节点
    T* last=nullptr;
    int gap=3;//任意层两个相邻的有着下一层节点的中间夹着gap个节点需要建立索引
    int max_deep=-1;
private:
    int step_size(){
        return gap%2==0?gap/2:gap/2+1;
    };
    bool a_is_greater_than_b(T* a,T* b){
        if(a==first||b==last) return false;
        if(a==last||b==first) return true;
        return std::get<keyIndex>(a->data())>std::get<keyIndex>(b->data());
    };
    bool a_is_greater_than_b(T* a,std::tuple<T_D...>& ta,T* b,std::tuple_element_t<keyIndex,std::tuple<T_D...>>& tb){
        if(a==first||b==last) return false;
        if(a==last||b==first) return true;
        return std::get<keyIndex>(ta)>tb;
    };

    bool a_is_less_than_b(T* a,T* b){
        if(a==last||b==first) return false;
        if(a==first||b==last) return true;
        return std::get<keyIndex>(a->data())<std::get<keyIndex>(b->data());
    };
    bool a_is_less_than_b(T* a,std::tuple<T_D...>& ta,T* b,std::tuple_element_t<keyIndex,std::tuple<T_D...>>& tb){
        if(a==last||b==first) return false;
        if(a==first||b==last) return true;
        return std::get<keyIndex>(ta)<tb;
    };

    bool a_is_equal_to_b(T* a,T* b){
        if(a==last||b==first) return false;
        return std::get<keyIndex>(a->data())==std::get<keyIndex>(b->data());
    };
    bool a_is_equal_to_b(T* a,std::tuple<T_D...>& ta,T* b,std::tuple_element_t<keyIndex,std::tuple<T_D...>>& tb){
        if(a==last||b==first) return false;
        return std::get<keyIndex>(ta)==tb;
    };
private:
    void top_build_and_index(){
        if(max_deep==first->rightIndex.size()-1) return;
        int deep=first->rightIndex.size()-1;
        int count=0;
        int fre=0;
        T* left=nullptr;
        T* right=nullptr;
        T* ptr=nullptr;
        while (true)
        {
            ptr=first;
            count=traverse_to_indexed_child(left,right,ptr,deep);
            if(count<gap+2) break;
            count-=3;//去除首尾和尾节点相邻的节点
            fre=count/step_size();
            for(int i=0;i<fre;++i){
                ptr=move_right(left,deep);
                if(!ptr) break;
                connect_node_push(left,ptr);
                left=ptr;
            }
            connect_node_push(ptr,last);//首尾节点有着所有层的索引
            ++deep;
        }
    };
    void bubble_build_and_index(T* node,int deep){
        while(true){
            node=build_and_index(node,deep);
            if(!node) return;
            ++deep;
        }
    };
    T* build_and_index(T* node,int deep){
        if(!node) return nullptr;
        T* left=nullptr;
        T* right=nullptr;
        int count=traverse_to_indexed_child(left,right,node,deep);
        if(count<gap+2) return nullptr;
        count-=3;
        int fre=count/step_size();
        for(int i=0;i<fre;++i){
            node=move_right(left,deep);
            if(!node) return nullptr;
            if(!insert_index(left,right,node,deep)) return nullptr;
            left=node;
        }
        return node;//因为count-3所以该节点为最接近right的新的提升索引的节点，如果递归或者while控制好deep可以一直向上构建
    };
    bool insert_index(T* left,T* right,T* node,int deep){
        if(!left||!right||!node
            ||left->rightIndex.size()<=deep+1||right->leftIndex.size()<=deep+1
            ||!(left->rightIndex[deep+1]==right&&right->leftIndex[deep+1]==left)
            ||node->leftIndex.size()!=deep+1||node->rightIndex.size()!=deep+1)
            return false;
        left->rightIndex[deep+1]=node;
        node->leftIndex.push_back(left);
        right->leftIndex[deep+1]=node;
        node->rightIndex.push_back(right);
        return true;
    };
    int traverse_to_indexed_child(T*& left,T*& right,T* node,int deep){
        int count=1;
        T* ptr_left=node;
        T* ptr_right=node;
        while(true){
            if(!ptr_left||!ptr_right) return 0;
            if(ptr_left->rightIndex.size()<=deep+1
                &&ptr_left->leftIndex.size()==deep+1){
                ptr_left=ptr_left->leftIndex[deep];
                ++count;
            }
            if(ptr_right->leftIndex.size()<=deep+1
                &&ptr_right->rightIndex.size()==deep+1){
                ptr_right=ptr_right->rightIndex[deep];
                ++count;
            }
            if((ptr_left->leftIndex.size()<deep+1||ptr_left->rightIndex.size()>deep+1)
                &&(ptr_right->rightIndex.size()<deep+1||ptr_right->leftIndex.size()>deep+1))
                break;
        }
        left=ptr_left;
        right=ptr_right;
        return count;
    };
    T* move_right(T* node,int deep){
        for(int i=0;i<step_size();++i){
            if(!node||node->rightIndex.size()<deep+1) return nullptr;
            node=node->rightIndex[deep];
        }
        return node;
    };
    void clear_index(T* node){//清理0外的索引
        if(!node||node->leftIndex.size()<=1||node->rightIndex.size()<=1) return;
        node->leftIndex.resize(1);
        node->rightIndex.resize(1);
    };
    void clear_index_deep(T* node,int deep){//清理deep外的索引
        if(!node||deep<0||node->leftIndex.size()<=deep+1||node->rightIndex.size()<=deep+1) return;
        node->leftIndex.resize(deep+1);
        node->rightIndex.resize(deep+1);
    };
    //不保证中间节点如何
    bool connect_node(T* left,T* right,int deep){
        if(!left||!right
            ||left->rightIndex.size()<deep+1
            ||right->leftIndex.size()<deep+1)
            return false;
        left->rightIndex[deep]=right;
        right->leftIndex[deep]=left;
        return true;
    };
    bool connect_node_push(T* left,T* right){
        if(!left||!right
            ||!(left->rightIndex.size()==right->leftIndex.size()))
            return false;
        left->rightIndex.push_back(right);
        right->leftIndex.push_back(left);
        return true;
    };
public:
    //节点的新建和连接需要在外部类实现，这里只该现管理索引，即该类不管理第0层索引
    void insert_build_and_index(T* node,int deep){
        bubble_build_and_index(node,deep);
        top_build_and_index();
    };
    void anew_build_and_index(){
        T* ptr=first;
        while (true)
        {
            if(ptr==last) break;
            clear_index(ptr);
            ptr=ptr->rightIndex[0];
        }
        clear_index(ptr);
        top_build_and_index();
    };
    void delete_build_and_index(std::tuple_element_t<keyIndex,std::tuple<T_D...>>& key){
        T* ptr=find(key);
        if(!ptr||!a_is_equal_to_b(ptr,ptr->data().data(),nullptr,key)) return;
        T* left=nullptr;
        T* right=nullptr;
        int count=traverse_to_indexed_child(left,right,ptr,0);
        left=ptr;
        do{
            int deep=ptr->leftIndex.size()-1;
            for(int i=1;i<=deep;++i){
                connect_node(ptr->leftIndex[i],ptr->rightIndex[i],i);
            }
            clear_index(ptr);
            ptr=right;
        }while(count==3&&right!=last&&ptr!=right);//如果等于3意味着ptr的左右两边有着下一层索引，需要合并
        ptr=left;
        //ptr后续删除后可能导致能构建新索引
        //因此这里临时摘去ptr重新构建索引
        //而任意两有着下级索引的节点一定不会挨着
        //所以这里用ptr的左边或者右边节点构建即可
        left=ptr->leftIndex[0];
        right=ptr->rightIndex[0];
        connect_node(left,right,0);
        insert_build_and_index(left);
        connect_node(left,ptr,0);
        connect_node(ptr,right,0);
    };
    void delete_build_and_index(T* ptr){
        if(!ptr||ptr==first||ptr==last) return;
        T* left=nullptr;
        T* right=nullptr;
        int count=traverse_to_indexed_child(left,right,ptr,0);
        left=ptr;
        do{
            int deep=ptr->leftIndex.size()-1;
            for(int i=1;i<=deep;++i){
                connect_node(ptr->leftIndex[i],ptr->rightIndex[i],i);
            }
            clear_index(ptr);
            ptr=right;
        }while(count==3&&right!=last&&ptr!=right);//如果等于3意味着ptr的左右两边有着下一层索引，而ptr将要删除，这两个节点可能相邻，清除其中一边
        ptr=left;
        left=ptr->leftIndex[0];
        right=ptr->rightIndex[0];
        connect_node(left,right,0);
        insert_build_and_index(left);
        connect_node(left,ptr,0);
        connect_node(ptr,right,0);
    };
    T* find(std::tuple_element_t<keyIndex,std::tuple<T_D...>>& key){
        int deep=first->rightIndex.size()-1;
        T* ptr=first;
        while(true){
            if(a_is_equal_to_b(ptr,ptr->data().data(),nullptr,key))
                return ptr;
            if(ptr->rightIndex.size()<deep+1){
                if(deep==0) return ptr;
                --deep;
                continue;
            }
            if(a_is_greater_than_b(ptr->rightIndex[deep],ptr->rightIndex[deep]->data().data(),nullptr,key)){
                if(deep==0) return ptr;
                --deep;
                continue;
            }
            if(a_is_less_than_b(ptr->rightIndex[deep],ptr->rightIndex[deep]->data().data(),nullptr,key)){
                ptr=ptr->rightIndex[deep];
            }
        }
    };
};

template<typename T,typename... T_D> requires NodeBase<T,T_D...>
class Allocator{
private:
    std::allocator<T> allocator;
    using traits = std::allocator_traits<decltype(allocator)>;
    std::vector<T*> allocator_ptrs;
    std::vector<T*> free_list;
    size_t allocator_index=0;
    int allocate_size=1024;
    T* first=nullptr;
public:
    Allocator(int allocate_size,T* first):allocate_size(allocate_size),first(first){};
    ~Allocator(){
        T* ptr=first;//first落后，ptr指向前方
        while(ptr->rightIndex.size()>0){
            ptr=ptr->rightIndex[0];
            traits::destroy(allocator,first);
            first=ptr;
        }
        traits::destroy(allocator,ptr);
        for(int i=0;i<allocator_ptrs.size();++i){
            allocator.deallocate(allocator_ptrs[i],allocate_size);
        }
        allocator_ptrs.clear();
        free_list.clear();
    };
    void del_node(T* node){
        free_list.push_back(node);
        traits::destroy(allocator,node);
    };
    T* get_node(T_D... td){
        if(free_list.size()>0){
            T* node=free_list.back();
            free_list.pop_back();
            return traits::construct(allocator,node,td);
        }
        ++allocator_index;
        if(allocate_size<allocator_index){
            allocator_index=1;
            allocator_ptrs.push_back(allocator.allocate(allocate_size));
        }
        return traits::construct(allocator,allocator_ptrs.back()[allocator_index-1],td);
    };
};

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

template<int keyIndex,typename... T_D>
class mSkipList;

template<int keyIndex,typename... T_D>
class mSkipList_view{
private:
    V_Node<T_D...>* first=nullptr;
    V_Node<T_D...>* last=nullptr;
    Algorithm<V_Node<T_D...>,keyIndex,T_D...> algorithm;
    mSkipList<keyIndex,T_D...>* base=nullptr;
    const int key_index=keyIndex;
    
};

template<int keyIndex,typename... T_D>
class mSkipList{
private:
    Node<T_D...>* first=nullptr;
    Node<T_D...>* last=nullptr;

    Algorithm<Node<T_D...>,keyIndex,T_D...> algorithm;
    std::vector<std::shared_ptr<mSkipList_view<keyIndex,T_D...>>> views;
    const int key_index=keyIndex;

    Allocator<Node<T_D...>,T_D...> allocator;
public:
    void insert(T_D... td){
        
    };
public:
    mSkipList(int node_count=1024){
        
    };
private:
};

#endif // MSKIPLIST