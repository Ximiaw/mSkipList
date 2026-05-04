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
private:
    int step_size(){
        return gap%2==0?gap/2:gap/2+1;
    };
    bool a_is_greater_than_b(T* a,T* b){
        if(a==first||b==last) return false;
        if(a==last||b==first) return true;
        return std::get<keyIndex>(a->data())>std::get<keyIndex>(b->data());
    };
    bool a_is_greater_than_b(T* a,std::tuple<T_D...>& ta,T* b,std::tuple<T_D...>& tb){
        if(a==first||b==last) return false;
        if(a==last||b==first) return true;
        return std::get<keyIndex>(ta)>std::get<keyIndex>(tb);
    };

    bool a_is_less_than_b(T* a,T* b){
        if(a==last||b==first) return false;
        if(a==first||b==last) return true;
        return std::get<keyIndex>(a->data())<std::get<keyIndex>(b->data());
    };
    bool a_is_less_than_b(T* a,std::tuple<T_D...>& ta,T* b,std::tuple<T_D...>& tb){
        if(a==last||b==first) return false;
        if(a==first||b==last) return true;
        return std::get<keyIndex>(ta)<std::get<keyIndex>(tb);
    };

    bool a_is_equal_to_b(T* a,T* b){
        if(a==last||b==first) return false;
        return std::get<keyIndex>(a->data())==std::get<keyIndex>(b->data());
    };
    bool a_is_equal_to_b(T* a,std::tuple<T_D...>& ta,T* b,std::tuple<T_D...>& tb){
        if(a==last||b==first) return false;
        return std::get<keyIndex>(ta)==std::get<keyIndex>(tb);
    };
private:
    T* build_and_index(T* node,int deep){

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
    T* find(std::tuple_element_t<keyIndex,std::tuple<T_D...> key>){
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

#endif // MSKIPLIST