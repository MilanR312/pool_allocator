#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <vector>
void printptr(void *p){
    std::cout << std::hex << std::showbase << reinterpret_cast<void*>(p) << std::dec << std::endl;
}

class memoryPool{
    using remote = char *;
    using remoteArray = remote *;
    //memory
    remoteArray data;

    //manages memory
    remoteArray start; //point at start of remote memory
    std::size_t len;
    std::size_t * lens;


    remoteArray first; //points to the first valid empty memory address
    remoteArray last;  //points to the element after the last valid memory address

    /* implement memory pool using a semi linked list where an allocated node gets removed
     * when allocating remove the first x nodes representing the data size
     * if deallocating push the node to the end of the list
     * allocates multiple sizes to allow for multiple datatypes
     */
    public:
    memoryPool(std::initializer_list<std::size_t> sizes){
        std::cout << "init managment\n";
        start = (remoteArray) malloc(sizeof(remote) * sizes.size());
        first = (remoteArray) malloc(sizeof(remote) * sizes.size());
        last  = (remoteArray) malloc(sizeof(remote) * sizes.size());
        lens = (std::size_t*) malloc(sizeof(std::size_t) * sizes.size());
        len = sizes.size();

        std::cout << "init main memory pool\n";
        for(int i = 0; i < len; i++){
            start[i] = (remote) malloc(  ( *(sizes.begin()+i) )* (1 << i)  );
            first[i] = start[i];
            last[i]  = start[i];
            lens[i] = *(sizes.begin() + i);
            std::cout << "pool " << i << " initialised with " << lens[i] << " spots and " << (1 << i) << " bytes per spot\n";
        }

    }
    void * alloc(std::size_t size){
        //fix this by possibly sorting and merging memory?
        if (size > (2 << len)) throw std::bad_alloc();
       
        //size 1 => pool 0 (2^0)
        //size 2 => pôol 1 (2^1)
        //size 3 => pool 2 (2^2)
        //size 4 => pool 2 (2^2)

        int pool_index = 0;
        while( size > (1<< pool_index)) pool_index++;
        std::cout << "allocating in pool " << pool_index << " for request of size " << size << " bytes\n";


        remote to_alloc = first[pool_index];
        //*(first[pool_index]) = 0; //not needed but helps debugging

        //(*first[pool_index]) = 0;
        //moves first element out of the chosen pool and goes to the next one, if overflow return to start
        first[pool_index] += size;
        //std::cout << "last possible in this pool=" 
        //    << reinterpret_cast<void*>(start[pool_index] + lens[pool_index] * (1 << pool_index))
        //    << "\n";
        if (first[pool_index] >= (start[pool_index] + lens[pool_index] * (1 << pool_index)))
            first[pool_index] = first[pool_index] - lens[pool_index];
        std::cout << "gave address " << reinterpret_cast<void*>(to_alloc) << "\n"; 
        return (void *) to_alloc;
    }
    void dealloc(void * p){
        //cant use size so better to look where pointer is
        int dealloc_index = 0;
        for (int i = 0; i < len; i++){
            if (start[i] > (char *)p){
                dealloc_index = i-1;
                break;
            }
        }
        std::cout << "starting dealloc for " << p << " in pool " << dealloc_index << "\n";

        last[dealloc_index] = (remote)p;
        last[dealloc_index] += (1 << dealloc_index);
        std::cout << "released address " << reinterpret_cast<void*>(p) << "\n\n";

        if (last[dealloc_index] > (start[dealloc_index]+lens[dealloc_index]))
            last[dealloc_index] = last[dealloc_index]-lens[dealloc_index];
    }

    void print_memory_mapping(){
        std::cout << "memory mappings\n";
        std::cout << "pool index | memory address in pool\n";
        for (int i = 0; i < len; i++){
            std::cout << "--------------------------------\n";
            std::cout << "pool " << i << "\n";
            for (int j = 0; j < lens[i]; j++){
                remote curr_address = start[i] + j * (1 << i);
                std::cout << j << " " << reinterpret_cast<void*>(curr_address) << " ";
                //std::cout << reinterpret_cast<void*>(first[i]) << " " << reinterpret_cast<void*>(last[i]);
                if (first[i] > last[i] && curr_address >= last[i] && curr_address < first[i]){
                    std::cout << "in use";
                } else if (first[i] < last[i] && (curr_address < first[i] || curr_address > last[i])){
                    std::cout << "in use";
                }
                //if (curr_address < first[i] && curr_address > last[i])
                //    std::cout << " in use";
                /*bool in_use = false;
                for (int k = 0; k < (1 << i); k++)
                    if (*(start[i] + j*(1<<i) + k) != 0) in_use = true;
                if (in_use) std::cout << " in use";
                */
                //std::cout << reinterpret_cast<void*>(first[i]);
                std::cout << "\n";
            }
        }
    }
};

template<typename T>
class pool_alloc{
    static memoryPool * m;    

    public:
    using value_type = T;

    static int alloc_index;
    explicit pool_alloc(){}
    template<typename U>
    pool_alloc(pool_alloc<U>&) noexcept {}


    [[nodiscard]] 
    static T * allocate(std::size_t n){
        auto p = (T*) m->alloc(sizeof(T) * n);
        return p;
    }
    static void deallocate(T * p, std::size_t n){
        m->dealloc(p);
    }
};
memoryPool mem({5,3,3,4,5});
template<typename T>
memoryPool* pool_alloc<T>::m = &mem;
/*
template<typename T, template <typename G> typename allocator>
struct complex{
    T a,b;
    static allocator<T> alloc;
    explicit complex(const allocator<T> & alloc = allocator()) : alloc(alloc){} ;

    void* operator new(size_t size){
        return alloc.allocate(size);
    }
};*/

int main(){
    std::vector<uint16_t, pool_alloc<uint16_t>> test;
    test.push_back(2);

    mem.print_memory_mapping();
    std::vector<uint16_t, pool_alloc<uint16_t>> test2;
    test2.push_back(3);
    test.push_back(5);

    //complex * x = new complex;
    //x->a = 1;
    //x->b = 2;
        //testing
    std::cout << test.at(0) << "\n";
    std::cout << test.at(1) << "\n";
    std::cout << test2.at(0) << "\n\n";
    //std::cout << x->a << " " << x->b << "\n";
    mem.print_memory_mapping();
}
