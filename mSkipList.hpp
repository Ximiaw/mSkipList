#ifndef MSKIPLIST
#define MSKIPLIST

#include<vector>
#include<array>
#include<memory>
#include<concepts>
#include<tuple>
#include<cassert>

template<typename T>
concept Key=std::semiregular<T>&&std::totally_ordered<T>;//视图类里面用，判断所选key是否满足需求

template<typename... T_D>
class Data{
private:
    std::tuple<T_D...> data_;
    void* ptrs_[sizeof...(T_D)];
    template<std::size_t... Is>
    constexpr void init(std::index_sequence<Is...>){
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
    Node(T_D... args):data_(std::forward<T_D>(args)...){};
};

template<typename... T_D>
struct V_Node{
    nodeIndexList<V_Node<T_D...>> leftIndex;
    nodeIndexList<V_Node<T_D...>> rightIndex;
    Node<T_D...>* node=nullptr;
    Data<T_D...>& data(){ return node->data(); };//可能报错，但是如果对应数据节点不存在则该视图不应该存在
    V_Node(Node<T_D...>* node):node(node){};
};

template<typename T,typename... T_D>
concept NodeBase=requires(T* node)
{
    {node->leftIndex}->std::same_as<nodeIndexList<T>&>;
    {node->rightIndex}->std::same_as<nodeIndexList<T>&>;
    {node->data()}->std::same_as<Data<T_D...>&>;
};

template<typename T,int keyIndex,typename... T_D>
    requires NodeBase<T,T_D...>
class Algorithm{
public:
    T* first=nullptr;//头尾添加两个哨兵节点，通过指针判断，使得算法简化为只需处理中间节点
    T* last=nullptr;
    int max_deep=-1;
    int gap=4;//任意层两个相邻的有着下一层节点的中间夹着gap个节点需要建立索引
    int step_size(){
        return gap%2==0?gap/2:gap/2+1;
    };
public:
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
    void sink_clear_index(){
        int deep=first->rightIndex.size()-1;
        T* ptr=first;
        int old_count=0;
        int new_count=0;
        while(true){
            new_count=right_to_indexed_child(ptr,deep);
            if(new_count>=gap+2&&old_count<gap+2) break;
            old_count=new_count;
            --deep;
        }
        clear_index_deep(deep);
    };
    void clear_index_deep(int deep){
        T* ptr=first;
        while (ptr!=last)
        {
            clear_index_deep(ptr,deep);
            ptr=ptr->rightIndex[deep];
        }
        clear_index_deep(ptr,deep);
    };
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
    int right_to_indexed_child(T* node,int deep){
        if(!node) return 0;
        int count=1;
        while (true)
        {
            if(node->rightIndex.size()<deep+1) return count;
            node=node->rightIndex[deep];
            ++count;
        }
        return count;
    };
    int traverse_to_indexed_child(T*& left,T*& right,T* node,int deep){
        int count=1;
        T* ptr_left=node;
        T* ptr_right=node;
        bool moved=false;
        while(true){
            moved=false;
            if(!ptr_left||!ptr_right) return 0;
            if(ptr_left->rightIndex.size()<=deep+1
                &&ptr_left->leftIndex.size()==deep+1){
                ptr_left=ptr_left->leftIndex[deep];
                ++count;
                moved=true;
            }
            if(ptr_right->leftIndex.size()<=deep+1
                &&ptr_right->rightIndex.size()==deep+1){
                ptr_right=ptr_right->rightIndex[deep];
                ++count;
                moved=true;
            }
            if((ptr_left->leftIndex.size()<deep+1||ptr_left->rightIndex.size()>deep+1)
                &&(ptr_right->rightIndex.size()<deep+1||ptr_right->leftIndex.size()>deep+1))
                break;
            if(!moved) break;
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
        clear_index_deep(node,0);
    };
    void clear_index_deep(T* node,int deep){//清理deep外的索引
        if(!node||deep<0) return;
        if(node!=first)
            node->leftIndex.resize(deep+1);
        if(node!=last)
            node->rightIndex.resize(deep+1);
    };
    //不保证中间节点如何
    bool connect_node(T* left,T* right,int deep){
        if(!left||!right||left==last||right==first||left==right
            ||left->rightIndex.size()<deep+1
            ||right->leftIndex.size()<deep+1)
            return false;
        left->rightIndex[deep]=right;
        right->leftIndex[deep]=left;
        return true;
    };
    bool connect_node_push(T* left,T* right){
        if(!left||!right||left==last||right==first||left==right
            ||!(left->rightIndex.size()==right->leftIndex.size()))
            return false;
        left->rightIndex.push_back(right);
        right->leftIndex.push_back(left);
        return true;
    };
public:
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
    void delete_build_and_index(T* ptr){//会清除底层，如果多视图则主视图要最后调用
        if(!ptr||ptr==first||ptr==last) return;
        T* left=ptr->leftIndex[0];
        T* right=ptr->rightIndex[0];
        int c_i_d=-1;
        while (true)
        {
            int deep=ptr->leftIndex.size()-1;
            for(int i=c_i_d+1;i<=deep;++i){
                connect_node(ptr->leftIndex[i],ptr->rightIndex[i],i);
            }
            clear_index_deep(ptr,c_i_d);
            if(right->leftIndex.size()>1&&left->rightIndex.size()>1&&right!=last){
                if(ptr==right) break;
                ptr=right;
                c_i_d=0;
                continue;
            }
            break;
        };
        insert_build_and_index(right,0);
        sink_clear_index();
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
            ptr=ptr->rightIndex[deep];
        }
    };
};

template<typename T,typename... T_D> 
    requires NodeBase<T,T_D...>
class Allocator{
private:
    std::allocator<T> allocator;
    using traits = std::allocator_traits<decltype(allocator)>;
    std::vector<T*> allocator_ptrs;
    std::vector<T*> free_list;
    size_t allocator_index=0;
    int allocate_size=1024;
public:
    T* first=nullptr;
public:
    Allocator(int allocate_size):allocate_size(allocate_size){
            allocator_index=0;
            allocator_ptrs.push_back(allocator.allocate(allocate_size));
    };
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
            traits::construct(allocator,node,td...);
            return node;
        }
        ++allocator_index;
        if(allocate_size<allocator_index){
            allocator_index=1;
            allocator_ptrs.push_back(allocator.allocate(allocate_size));
        }
        traits::construct(allocator,allocator_ptrs.back()+allocator_index-1,td...);
        return allocator_ptrs.back()+allocator_index-1;
    };
    T* get_v_node(Node<T_D...>* node){
        if(free_list.size()>0){
            T* v_node=free_list.back();
            free_list.pop_back();
            traits::construct(allocator,v_node,node);
            return v_node;
        }
        ++allocator_index;
        if(allocate_size<allocator_index){
            allocator_index=1;
            allocator_ptrs.push_back(allocator.allocate(allocate_size));
        }
        traits::construct(allocator,allocator_ptrs.back()+allocator_index-1,node);
        return allocator_ptrs.back()+allocator_index-1;
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
    Node<T_D...>* first=nullptr;//不需要手动管理，分配器会管理
    Node<T_D...>* last=nullptr;

    Algorithm<Node<T_D...>,keyIndex,T_D...> algorithm;
    std::vector<std::shared_ptr<mSkipList_view<keyIndex,T_D...>>> views;
    const int key_index=keyIndex;

    Allocator<Node<T_D...>,T_D...> allocator;


    bool connect_node(Node<T_D...>* left,Node<T_D...>* right){
        if(!left||!right||left==right)
            return false;
        left->rightIndex[0]=right;
        right->leftIndex[0]=left;
        return true;
    };
    bool insert_right(Node<T_D...>* left,Node<T_D...>* right,Node<T_D...>* node){
        if(!left||!right||!node
            ||!(left->rightIndex[0]==right&&right->leftIndex[0]==left)
            ||node->leftIndex.size()!=0||node->rightIndex.size()!=0)
            return false;
        left->rightIndex[0]=node;
        node->leftIndex.push_back(left);
        right->leftIndex[0]=node;
        node->rightIndex.push_back(right);
        return true;
    };
public:
    void insert(T_D... td){
        auto key=std::get<keyIndex>(std::make_tuple(td...));
        Node<T_D...>* node=algorithm.find(key);//会返回应插入位置的左边节点，或者有着这个key的节点
        if(algorithm.a_is_equal_to_b(node,node->data().data(),nullptr,key)){
            node->data().data()=std::move(std::make_tuple(td...));
            return;
        }
        insert_right(node,node->rightIndex[0],allocator.get_node(td...));
        algorithm.insert_build_and_index(node->rightIndex[0],0);
    };
    template<typename Type>
    const Type& get(std::tuple_element_t<keyIndex,std::tuple<T_D...>> key){
        Node<T_D...>* node=algorithm.find(key);//会返回应插入位置的左边节点，或者有着这个key的节点
        if(algorithm.a_is_equal_to_b(node,node->data().data(),nullptr,key)){
            return std::get<keyIndex>(node->data().data());
        }
        throw std::runtime_error("key not found.");
    };
    void erase(std::tuple_element_t<keyIndex,std::tuple<T_D...>> key){
        auto node=algorithm.find(key);
        if(node==first||node==last) return;
        if(algorithm.a_is_equal_to_b(node,node->data().data(),nullptr,key)){
            algorithm.delete_build_and_index(node);
            allocator.del_node(node);
            return;
        }
        throw std::runtime_error("key not found.");
    };
    int get_deep(){
        return first->rightIndex.size();
    };
    int max_deep() const{
        return algorithm.max_deep+1;
    };
    void set_max_deep(int max_deep){
        algorithm.max_deep=max_deep-1;
    };
    int gap() const{
        return algorithm.gap;
    };
    void set_gap(int gap){
        algorithm.gap=gap;
    };
public:
    mSkipList(T_D... td):allocator(4096){//td可以是任意数据，这里只是填入便于构造哨兵
        first=allocator.get_node(td...);
        algorithm.first=first;
        allocator.first=first;
        last=allocator.get_node(td...);
        algorithm.last=last;
        first->rightIndex.push_back(last);
        last->leftIndex.push_back(first);
    };
    mSkipList(int allocate_size=4096,int gap=3,int max_deep=-1,T_D... td):allocator(allocate_size){//td可以是任意数据，这里只是填入便于构造哨兵
        first=allocator.get_node(td...);
        algorithm.first=first;
        allocator.first=first;
        last=allocator.get_node(td...);
        algorithm.last=last;
        first->rightIndex.push_back(last);
        last->leftIndex.push_back(first);
        algorithm.gap=gap;
        algorithm.max_deep=max_deep;
    };

    Node<T_D...>* fir(){//test
        return first;
    };
};

#endif // MSKIPLIST