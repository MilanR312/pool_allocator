# pool allocator in c++
## description

pool allocator implemented in c++, should be useable with all stl containers   
utilises predefined pools of memory for allocation   
pool size can be specified in the constructor with each pool having space for 1<<pool_index bytes per space   

## improvements
- if the pool for size x is full then allocate in a bigger pool instead of reallocating    
- throw error if all pools are filled   
- possible kind of memory merging to allow for bigger sizes then largest pool (last pool combining or all pools?)

## problems
- undefined behavior when allocating from filled pool   

