* Final Assignment of Utah EAE 6300.
* Header Only Library. 
* The Allocator includes two parts, a heap allocator and a fixed size allocator.
* The Heap Allocator is just copied from Assignment 1.08. So much could be improved.
* The Heap Allocator is based on Linked List, all the descriptors are dynamic allocated.
* Because I use global variables to implement my Memory System. So CRTDBG would warning there could be Memory Leak. That should be the global variables.
* Used hardcode in InitializeMemorySystem, maybe struct FSAInitData could be a better choice.