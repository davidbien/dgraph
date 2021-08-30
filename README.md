# Directed Graph Template Library

These are the notes I sent to the boost mailing list in 19APR2000.  
I imagine they are accurate since I was immersed in the implementation at the time.  
At this time I am less familiar with the implementation since I haven't work on it since then, except to port it to the latest C++ and platforms, etc.  

Over the past year or so I have written a directed graph template.
template < class t_TyNodeEl, class t_TyLinkEl, class t_TyAllocator,
           class t_TyGraphTraits = _graph_traits_map< ... > >
class _graph;
If anyone is interested in this, please reply. It would take a
reasonable ( but not unreasonable ) amount of work to publish this
library with documentation, etc., but I wouldn't mind too much.
Some properties:
- true cyclical directed graph ( can be used as acyclic )
- templatized by node element, link element, allocator, graph traits
- one memory object per node
- one memory object per link
- total memory objects in a graph, g with N nodes, L links =
  1 ( for g ) + N + L
- written in ANSI C++, compiles on intel4.0 and gcc2.95.2.
- const-correct ( at least that's the idea :-)
- micro-engineered ( so you don't have to :-)
- O(N+L) non-throwing destructor(assuming element's dtors don't throw)
- library is throw-state-safe i.e. a throw preserves originally state
   whereever possible / semantically desirable.
- copy in O( N + L + MlogM + KlogK )(approximate) where M is number
   of multi-parented ( childrened ) children ( parents ) and K is the
   number of links that leave those nodes in the opposite direction
   of the current copy direction. Since we don't have coloring built
   into the graph elements, we sometimes need to look up touched
   nodes/links. We only need to keep the lookups around for a given
   region of the graph - until the depth first search in the current
   direction is over - thus the number is generally less than given
   above - for multi-regioned graphs.
   Copy of graph that is tree is O(N+L).
   Copy is templatized by output graph type - allowing copy to create
   a different graph type than the original, i.e. a graph of doubles
   can be created from a graph of ints ( nodes and/or links ).
- support for "closed directed" copy - to allow copying of a portion
   of the graph. Same code and same order as above. Guarantee that
   only nodes and links that will be in resultant graph will get
   copied.
- serialization to/from stream support, currently support ostreams
   and OLE streams through templatized wrapper. Order of
   serialization is similar to copy. Note that, since the graph is
   serialized in depth-first order, lookup is minimized on save/load.
   Saving/loading a graph that is a tree is O(N+L).
- support for human readable dump ( depth first order ).
- support for many types of iterators:
  - graph iterator - iterates the entire graph in depth first order -
     either forward/backward and also supports "closed directed"
     iteration - to iterate a closed region of the graph.
  - selection iterator - iterates a the graph according to a selection
     function object ( templatized by ). Currently only selective link
     iteration is supported(it is all i needed to do some other work).
  - node iterator - maintains identity of a single node, allows
     movement from that node. ( safe version supported )
  - link position iterator - maintains a position in a parent/child
     adjacency list. ( safe version supported )
  - link identity iterator - maintains the identity of a link(safe sp)
  - graph path iterator - maintains a path in a graph. I have a safe
     version for this as well, but it is incomplete - to be done -
     it is a bit tougher than the others :-).
  - I/O iterators - I/O is performed using throw-state-safe iterators.
     This allows I/O to proceed correctly, regardless of low-memory
     situations, etc. The exception can be handled i.e. allow the
     user to free disk space or close applications, and the I/O
     operation can complete successfully.
- templatization by graph traits allows selective modification of
   implementation classes. Since implementation classes are not
   ( in almost all cases ) defined inside of _graph - but rather
   within a hierarchical graph traits type repository, implementations
   of various graph objects can be selective overridden. Thus, for
   instance, if one wanted to provide a "color" for both links and
   nodes that was known by the implementation, and then provide a
   graph copy that utilized this color ( a non-multi-thread approach )
   this can be done with (relative) ease.
   It also provides a simple template for the general purpose user,
   but with tremendous flexibility for the expert who is engineering
   a fast and/or specialized implementation.
- "safe" iterators ( not to imply thread safety - though this was the
    initial goal ) provide a means of "watching" a node, link, or
    link position ( or eventually a path, maybe ) within the graph.
    If the node goes away, then the graph object in the iterator is
    nullified. This could be made thread safe. A notification system
    could also easily be added through this mechanism.
- All implementations ( except for copy - TBD ) have been written so
    that the meat of the implementation goes into a non-templatized
    base class - that calls pointer-to-member functions defined by
    its parent class ( i.e. to do type specific stuff )
    This keeps the added code size for additional graph types at
    a minimum.