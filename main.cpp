#include <cmath>
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
    using dataPool = char *;
    using dataPoolArray = dataPool *;
    using remote = dataPool *;
    using remoteArray = remote *;
    //memory
    dataPoolArray data;

    //manages memory
    remoteArray start; //point at start of remote memory
    std::size_t len;
    std::size_t * lens;


    int* first; //points to the first valid empty memory address
    int* last;  //points to the element after the last valid memory address

    bool verbose = false;

    /* implement memory pool using a circular buffer where an allocated node gets removed
     * when allocating remove the first x nodes representing the data size
     * if deallocating push the node to the end of the list
     * allocates multiple sizes to allow for multiple datatypes
     * 
     * start points to first element of the data pools
     * |  | -> [                 ]
     * |  | -> [                ]
     * |  | -> [       ]
     * |  | -> [           ]
     *
     */
    const int tab64[64] = {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5};

    int log2_64 (uint64_t value)
    {
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        return tab64[((uint64_t)((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
    }

    public:
    memoryPool(std::initializer_list<std::size_t> sizes){
        if (verbose)
            std::cout << "init managment\n";
        //initialise helper arrays for memory access
        first = (int*) malloc(sizeof(remote) * sizes.size());
        last  = (int*) malloc(sizeof(remote) * sizes.size());
        lens = (std::size_t*) malloc(sizeof(std::size_t) * sizes.size());

        data = (dataPoolArray) malloc(sizeof(dataPool) * sizes.size());
        len = sizes.size();
        if (verbose)
            std::cout << "init main memory pool\n";
        //initialise the main data and setup helper arrays
        for(int i = 0; i < len; i++){
            data[i] = (dataPool) malloc(  ( *(sizes.begin()+i) )* (1 << i)  );
            first[i] = 0;
            last[i]  = 0;
            lens[i] = *(sizes.begin() + i);
            if (verbose)
                std::cout << "pool " << i << " initialised with " << lens[i] << " spots and " << (1 << i) << " bytes per spot and starting address " << reinterpret_cast<void*>(data[i]) << "\n";
        }
    }
    void * alloc(std::size_t size){
        //fix this by possibly sorting and merging memory?
        if (size > (2 << len)) throw std::bad_alloc();
       
        //find the pool that can contain the size
        int pool_index = 0;
        while( size > (1<< pool_index)) pool_index++;
        //possible faster with log2 but not at smaller amount of pools?
        //pool_index = log2_64(size);

        if (verbose)
            std::cout << "allocating in pool " << pool_index << " for request of size " << size << " bytes\n";
        

        int &index_to_alloc_from = first[pool_index];

        //get memory address to allocate
        dataPool to_alloc = data[pool_index] + (1 << pool_index)*index_to_alloc_from;
        //*index_to_alloc_from = 0; //not needed but helps debugging

        //moves first element out of the chosen pool and goes to the next one, if overflow return to start
        index_to_alloc_from++;
        
        //circular buffer logic
        if (index_to_alloc_from >= lens[pool_index])
            index_to_alloc_from = 0;
        if (verbose)
            std::cout << "gave address " << reinterpret_cast<void*>(to_alloc) << "\n"; 
        return (void *) to_alloc;
    }
    void dealloc(void * p, std::size_t size){
        //int dealloc_index = log2_64(size);
        //also possible faster using log2 but not at smaller amount of pools
        int dealloc_index = 0;
        for (int i = 0; i < len; i++){
            const dataPool pool = data[i];
            if (pool > (char *)p){
                dealloc_index = i-1;
                break;
            }
        }
        if (verbose)
            std::cout << "starting dealloc for " << p << " in pool " << dealloc_index << "\n";
        int & index_to_dealloc_from = last[dealloc_index]; 
        index_to_dealloc_from++;

    
    }

    void print_memory_mapping(){
        std::cout << "memory mappings\n";
        std::cout << "pool index | memory address\n";
        for (int i = 0; i < len; i++){
            std::cout << "--------------------------------\n";
            std::cout << "pool " << i << "\n";
            for (int j = 0; j < lens[i]; j++){
                std::cout << j << " "
                    << reinterpret_cast<void*>(data[i] + j*(1 << i));
                
                if (first[i] > last[i] && j < first[i] && j >= last[i])
                    std::cout << "   in use ";
                if (first[i] < last[i] && ( j < last[i] || j > first[i]))
                    std::cout << "   in use ";

                //if (j == first[i]) std::cout << " <-first";
                //if (j == last[i] ) std::cout << " <-last"; 
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
        m->dealloc(p,n);
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
    for (int i = 0; i < 1'000'000; i++){
        std::vector<uint16_t, pool_alloc<uint16_t>> test;
        test.push_back(1);
        test.push_back(2);
        test.push_back(3);
        test.push_back(4);
        test.push_back(5);
    }
}
