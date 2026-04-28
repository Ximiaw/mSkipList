#ifndef MSKIPLIST
#define MSKIPLIST

#include<iostream>

#include<iterator>
#include<memory>
#include<vector>
#include<stdexcept>

template<typename K,typename V>class mSkipList{
public:
    struct KV{
        K key;
        V value;
    };//KV必须提供默认值和赋值构造函数，K必须提供==，<，>的重载
private:
    struct node
    {
        std::vector<node*> left;//这里索引0为原始数据层，为1就是第一层索引
        std::vector<node*> right;
        std::unique_ptr<KV> data=std::make_unique<KV>();
    };
    node* _first=nullptr;
    node* _last=nullptr;
    int _gap=3;//有三个中间节点就构建索引，主要影响插入删除的索引构建，topBuildAndIndex不受影响
    int _deep=-1;
    int _length=0;
public:
    class iterator{
    private:
        node* _first;
        node* _last;
        node* _ptr;
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = KV;
        using difference_type = std::ptrdiff_t;
        using pointer = KV*;
        using reference = KV&;
        iterator()=delete;
        explicit iterator(node* start,node* last):_first(start),_last(last),_ptr(start){};
        explicit iterator(node* start,node* last,node* ptr):_first(start),_last(last),_ptr(ptr){};
        reference operator*() const{ 
            return *_ptr->data; 
        };
        pointer operator->() const{
            return _ptr->data.get(); 
        };
        iterator& operator++(){ 
            if(_ptr&&_ptr->right.size()>0) _ptr=_ptr->right[0]; 
            else _ptr=nullptr; 
            return *this;
        };
        iterator operator++(int){ 
            iterator old{_first,_last,_ptr}; 
            if(_ptr&&_ptr->right.size()>0) _ptr=_ptr->right[0]; 
            else _ptr=nullptr; 
            return old; 
        };
        iterator& operator--(){
            if(!_ptr){
                _ptr=_last;
                return *this;
            }
            if(_ptr&&_ptr->left.size()>0) _ptr=_ptr->left[0]; 
            else _ptr=nullptr; 
            return *this; 
        };
        iterator operator--(int){ 
            iterator old{_first,_last,_ptr}; 
            if(!_ptr){
                _ptr=_last;
                return old;
            }
            if(_ptr&&_ptr->left.size()>0) _ptr=_ptr->left[0]; 
            else _ptr=nullptr; 
            return old; 
        };
        bool operator==(const iterator& other) const{ 
            return _ptr==other._ptr; 
        };
        bool operator!=(const iterator& other) const{ 
            return _ptr!=other._ptr; 
        };
    };
private:
    inline void kvCopy(KV* target,KV* source){ 
        target->key = source->key; 
        target->value = source->value; 
    };
    inline void otherCopyToThis(const mSkipList& other){
        if(other._length==0 || this==&other ) return;
        if(!_first) _first=new node;
        for(auto it=other.begin();it!=other.end();++it)
        {
            if(_last){
                _last->right.push_back(new node);
                _last->right[0]->left.push_back(_last);
                _last=_last->right[0];
            }else{
                _last=_first;
            }
            kvCopy(_last->data.get(),it.operator->());
        }
        _length+=other._length;
        _deep=0;
    };
    inline bool moveRight(node** right,int deep){
        int nodeCount=_gap%2==0?_gap/2:_gap/2+1;
        for (int i = 0; i < nodeCount; i++)
        {
            if((*right)->right.size()<deep+1) return false;
            (*right)=(*right)->right[deep];
        }
        return true;
    }
    inline bool moveLeft(node** left,int deep){
        int nodeCount=_gap%2==0?_gap/2:_gap/2+1;
        for (int i = 0; i < nodeCount; i++)
        {
            if((*left)->left.size()<deep+1) return false;
            (*left)=(*left)->left[deep];
        }
        return true;
    }
    inline void traverseToIndexedChild(node** pleft,node** pright,int& deep,int& count){
        while(true){
            if((*pleft)->right.size()<=deep+1){//left的右侧是否不包含下一层的索引
                if((*pleft)->left.size()>=deep+1){//left左侧是否有节点
                    (*pleft)=(*pleft)->left[deep];
                    ++count;
                }
            }
            if((*pright)->left.size()<=deep+1){//right的左侧是否不包含下一层的索引
                if((*pright)->right.size()>=deep+1){//right右侧是否有节点
                    (*pright)=(*pright)->right[deep];
                    ++count;
                }
            }
            if(((*pright)->right.size()<deep+1||(*pright)->left.size()>deep+1)&&
                ((*pleft)->left.size()<deep+1||(*pleft)->right.size()>deep+1)) break;
        }
    }
    inline void topBuildAndIndex(){//从first的最高层向上构建索引
        node* left=_first;//第一个节点
        node* right=left;
        int deep=_first->right.size()-1;
        int count=1;//对当前层的节点计数，即最左侧节点也要算上
        while (true)
        {
            if(!moveRight(&right,deep)){//若本节点为1,则检查第2、第3节点在deep+1层是否为空
                ++deep;//将层数索引更新到最新
                //检查本层是否为两个节点，从最右侧向左，其中最右侧的节点数需在外部加，如果是则跳出循环，deep为层深，其中第0层为原始数据
                //这时候right和left指向同一节点
                if(count<=(_gap%2==0?_gap/2:_gap/2+1)) break;
                count=1;
                left=_first;
                right=left;
                continue;
            }
            left->right.push_back(right);
            right->left.push_back(left);
            left=right;
            ++count;
        }
        _deep=_first->right.size()-1;
    }
    inline void initBuildAndIndex(){
        if(_length<=2) return;
        //能走到这里说明至少能建立一层索引，这时deep为0,我们利用deep为层数索引
        topBuildAndIndex();
    };
    inline void putBuildAndIndex(KV kv){
        //若为空
        if(!_first){
            _first=new node;
            kvCopy(_first->data.get(),&kv);
            _last=_first;
            _deep=0;
            _length=1;
            return;
        }
        //若等于
        node* ptr = find(kv.key);//必然拿到一个节点
        if(!ptr) return;
        if(ptr->data->key==kv.key){
            ptr->data->value=kv.value;
            return;
        }
        node* left=ptr;
        node* right=ptr;
        int deep=0;
        int count=1;//为1是因为含ptr
        bool build_index=false;
        bool isleft=ptr->data->key<kv.key;//ptr是否在应插入缝隙的左边
        enum class BuildsStatus{ first, last, middle} build_status=BuildsStatus::middle;
        if(isleft){//ptr在缝隙左边，则right右移
            //若在行末
            if(_last->data->key<kv.key){
                _last->right.push_back(new node);
                _last->right[0]->left.push_back(_last);
                _last=_last->right[0];
                ptr=left=right=_last;
                kvCopy(ptr->data.get(),&kv);
                build_status=BuildsStatus::last;
                ++_length;
            }else if(right->right.size()>=deep+1){
                right=right->right[deep];
                ++count;
            }
        }else{//ptr在缝隙右边，则left左移
            //若在行首
            if(_first->data->key>kv.key){
                _first->left.push_back(new node);
                _first->left[0]->right.push_back(_first);
                _first=_first->left[0];
                ptr=left=right=_first;
                kvCopy(ptr->data.get(),&kv);
                build_status=BuildsStatus::first;
                ++_length;
            }else if(left->left.size()>=deep+1){
                left=left->left[deep];
                ptr=left;
                ++count;
            }
        }
        while(true){
            traverseToIndexedChild(&left,&right,deep,count);
            //下面要求使用count，这时right或left可能被修改了，但ptr没有被修改
            if(build_status==BuildsStatus::first){
                right=left->right[0];
                //这时left和ptr均指向first，而right指向首节点或者距离最近的有着下一层节点的节点，count是包含双端节点的计数
                if(right->right.size()==0) break;
                if(right->right.size()!=right->right[0]->left.size()){//没有两个相邻的全索引节点，这里的right是原first，有着每一层的索引
                    for(int i=1;i<right->right.size();++i){
                        left->right.push_back(right);
                        right->left.push_back(left);
                    }
                    break;
                }
                ptr=right;
                right=right->right[0];
                ptr->left.clear();
                ptr->left.push_back(left);
                ptr->right.clear();
                ptr->right.push_back(right);
                for(int i=1;i<right->left.size();++i){
                    left->right.push_back(right);
                    right->left[i]=left;
                }
                break;
            }else if(build_status==BuildsStatus::last){
                //这时right和ptr均指向last，而left指向首节点或者距离最近的有着下一层节点的节点，count是包含双端节点的计数
                if(count<3) break;
                left->right.push_back(right);
                right->left.push_back(left);
                left=right;//调整left的位置，以便向上检查节点
                count=1;//right在右边缘，所以需要记1
                ++deep;
            }else if(build_status==BuildsStatus::middle){
                if(build_index){
                    std::cout<<left->right.size()<<"\t"<<right->left.size()<<std::endl;
                    //此时的deep直接作为vector的索引能拿到当前活动索引层的节点
                    //ptr指向新插入的节点，而left和right是指向首节点或者距离最近的有着下一层节点的节点，count是包含双端节点的计数
                    if(count<=_gap+2) break;
                    if(deep+1>_deep) break;//哪怕符合建立索引的规则，但是已经在最上层了，不允许继续建
                    ptr=left;
                    if(!moveRight(&ptr,deep)) break;
                    ++deep;
                    if(left->right.size()<deep+1||right->left.size()<deep+1) break;
                    left->right[deep] = ptr;
                    ptr->left.push_back(left);
                    right->left[deep] = ptr;
                    ptr->right.push_back(right);
                    left=right=ptr;
                    count=1;
                    continue;
                }
                //这时ptr指向应插入缝隙的左边节点，而left和right是指向首节点或者距离最近的有着下一层节点的节点，count是包含双端节点的计数
                node* r_ptr=ptr->right[0];
                ptr->right[0]=new node;
                ptr->right[0]->left.push_back(ptr);
                r_ptr->left[0]=ptr->right[0];
                ptr->right[0]->right.push_back(r_ptr);
                ptr=ptr->right[0];
                kvCopy(ptr->data.get(),&kv);
                ++count;
                ++_length;
                if(count<5) break;
                build_index=true;
                left=right=ptr;
                count=1;
            }
        }
        topBuildAndIndex();//前面只保证在现有层数上构建索引，这里检查并构建顶层索引

        
        // std::cout<<"总节点数为："<<_length<<"插入："<<kv.key<<std::endl;
        // for (int i =  _first->right.size()-1; i >= 0; i--)
        // {
        //     node* ptr=_first;
        //     while (true)
        //     {
        //         if(ptr->right.size()>=i+1&&ptr->left.size()>=i+1){
        //             std::cout<<ptr->left[i]->data->key<<"/"<<ptr->data->key<<"/"<<ptr->right[i]->data->key<<"\t";
        //         }else if(ptr->right.size()>=i+1){
        //             std::cout<<"-/"<<ptr->data->key<<"/"<<ptr->right[i]->data->key<<"\t";
        //         }else if(ptr->left.size()>=i+1){
        //             std::cout<<ptr->left[i]->data->key<<"/"<<ptr->data->key<<"/-"<<"\t";
        //         }else{
        //             std::cout<<"-\t";
        //         }
        //         if(ptr->right.size()>=1){
        //             ptr = ptr->right[0];
        //             continue;
        //         }
        //         break;
        //     }
        //     std::cout<<std::endl;
        // }
        
    };
    inline bool delBuildAndIndex(K key){
        //todo
        return true;
    };
    inline node* find(K key){//若为空则返回null，若查到则返回目标节点，若没查到则返回法间应插入缝隙的左/右节点
        if(!_first) return nullptr;
        if(_first->data->key>key) return _first;
        if(_last->data->key<key) return _last;
        int deep=_deep;
        node* ptr=_first;
        while (true)
        {
            if(ptr->right.size()<deep+1){
                deep--;
                continue;
            }
            if(ptr->right[deep]->data->key==key) return ptr->right[deep];
            if(ptr->right[deep]->data->key>key){
                if(deep==0) return ptr;
                deep--;
                continue;
            }else{
                ptr=ptr->right[deep];
            }
        }
    }; 
public:
    mSkipList(int gap=3):_gap(gap){};
    ~mSkipList(){ 
        clear(); 
    };
    mSkipList(const mSkipList& other){
        if(other._length==0) return;
        otherCopyToThis(other);
        initBuildAndIndex();
    };
    mSkipList(mSkipList&& other) noexcept{
        if(other._length==0) return;
        _first=other._first;
        _last=other._last;
        _length=other._length;
        _deep=other._deep;
        other._first=nullptr;
        other._last=nullptr;
        other._length=0;
        other._deep=-1;
    };
    mSkipList& operator=(const mSkipList& other){
        if(this==&other || other._length==0) return *this;
        clear();
        otherCopyToThis(other);
        initBuildAndIndex();
        return *this;
    };
    mSkipList& operator=(mSkipList&& other) noexcept{
        if(this==&other || other._length==0) return *this;
        _first=other._first;
        _last=other._last;
        _length=other._length;
        _deep=other._deep;
        other._first=nullptr;
        other._last=nullptr;
        other._length=0;
        other._deep=-1;
        return *this;
    };

    const V& get(K key){
        node* ptr=find(key); 
        bool exists = ptr?ptr->data->key==key:false; 
        if(!exists) throw std::runtime_error("key not found.");
        return ptr->data->value;
    };
    void put(K key,V value){
        putBuildAndIndex(KV{key,value});
        
        for (int i = _deep; i >= 0; i--)
        {
            node* ptr=_first;
            int count=1;
            while (true)
            {
                if(ptr->right.size()<i+1){
                    break;
                }
                ptr=ptr->right[i];
                ++count;
            }
            std::cout<<"第"<<i<<"层："<<count<<std::endl;
        }
        
    };
    void del(K key){
        delBuildAndIndex(key);
    };//迭代器不提供删除，想删除需要调用这里
    bool exists(K key){ 
        node* ptr=find(key); 
        return ptr?ptr->data->key==key:false; 
    };
    const int deep() const{ 
        return _deep; 
    };//原始数据在第0层
    const int length() const{ 
        return _length; 
    };
    void clear(){
        if(_length==0) return;
        //因为node内left和right存的都是指向原数据的指针，所以可以只delete第0层的数据
        node* left=_first;
        node* right=left;
        while(right->right.size()>=1){
            right=right->right[0];
            delete left;
            left=right;
        }
        if(right) delete right;
        _first=nullptr;
        _last=nullptr;
        _deep=-1;
        _length=0;
    };

    iterator begin(){ 
        return iterator{_first,_last}; 
    };
    iterator end(){ 
        return iterator{_first,_last,nullptr}; 
    };
};

#endif