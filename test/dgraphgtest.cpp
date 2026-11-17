// dgraphgtest.cpp
// Google test unit tests for the dgraph directed graph implementation.
// These need no input files: graphs are generated randomly (deterministically from seeds) or built directly.
// What is tested:
//  1) Fuzzing: random graphs (with self-loops and multiple links between the same nodes) checked for:
//    a) Structural integrity of the parent and child lists.
//    b) Forward iteration in all four modes (closed/non-closed x down/up) visiting exactly the expected elements once.
//    c) Link-selecting (controlled) iteration.
//    d) Copy construction and assignment of an iterator at every point of an iteration.
//    e) Copying the graph: whole graph and from arbitrary nodes in either direction.
//    f) Saving to and loading from a stream.
//    g) Destruction leaving nothing allocated or constructed.
//  2) List operations (exchange, move, counting) exhaustively over small lists against a simple model.
//  3) Graph assignment and custom copy objects.

#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif //WIN32
#define DBG_NEW new

#include <memory>
#include <exception>
typedef std::allocator< char >	_TyDefaultAllocator;
#include "_compat.h"
#include "_heapchk.h"

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <deque>
#include <unordered_map>
#include <algorithm>
#include <filesystem>
#include "syslogmgr.inl"
#include "_compat.inl"
#include "_gr_inc.h"
#include "gtest/gtest.h"

std::string g_strProgramName;

// We count allocations and constructions of graph elements to detect leaks: see __DGRAPH_COUNT_EL_ALLOC_LIFETIME in _gr_inc.h.

namespace ns_DGraphGTest
{
__BIENUTIL_USING_NAMESPACE
__DGRAPH_USING_NAMESPACE

typedef dgraph< int, int, false > _TyGraphNotSafe;
typedef dgraph< int, int, true > _TyGraphSafe;

// Don't bury the log in thousands of failures when something fundamental breaks.
static const int s_knMaxFailuresReported = 20;
static bool FTooManyFailures()
{
  return testing::UnitTest::GetInstance()->current_test_info()->result()->total_part_count() >= s_knMaxFailuresReported;
}

struct ElCounts
{
  int m_nNodesAllocated{ gs_iNodesAllocated };
  int m_nLinksAllocated{ gs_iLinksAllocated };
  int m_nNodesConstructed{ gs_iNodesConstructed };
  int m_nLinksConstructed{ gs_iLinksConstructed };
  bool operator == ( ElCounts const & _r ) const
  {
    return ( m_nNodesAllocated == _r.m_nNodesAllocated ) && ( m_nLinksAllocated == _r.m_nLinksAllocated ) &&
           ( m_nNodesConstructed == _r.m_nNodesConstructed ) && ( m_nLinksConstructed == _r.m_nLinksConstructed );
  }
};
static std::ostream & operator << ( std::ostream & _ros, ElCounts const & _r )
{
  return _ros << "nodes allocated/constructed " << _r.m_nNodesAllocated << "/" << _r.m_nNodesConstructed
              << " links allocated/constructed " << _r.m_nLinksAllocated << "/" << _r.m_nLinksConstructed;
}

// Produce an exact structural description of the graph as seen from its root: nodes are numbered in discovery order
//  (child list in order then parent list in order). Two graphs have the same description if and only if they have the
//  same structure, list orders and element values. List integrity problems are marked with '!'.
template < class t_TyGraph >
std::string StrDescribeGraph( t_TyGraph const & _g )
{
  typedef typename t_TyGraph::_TyGraphNode _TyGN;
  typedef typename t_TyGraph::_TyGraphLink _TyGL;
  const _TyGN * pgnRoot = _g.get_root();
  if ( !pgnRoot )
    return "(empty)";
  std::stringstream ss;
  std::unordered_map< const _TyGN *, size_t > mapIds;
  std::vector< const _TyGN * > rgNodes;
  mapIds[ pgnRoot ] = 0;
  rgNodes.push_back( pgnRoot );
  auto lambdaId = [&]( const _TyGN * _pgn ) -> size_t
  {
    auto pib = mapIds.insert( std::make_pair( _pgn, rgNodes.size() ) );
    if ( pib.second )
      rgNodes.push_back( _pgn );
    return pib.first->second;
  };
  for ( size_t nNode = 0; nNode < rgNodes.size(); ++nNode )
  {
    const _TyGN * pgn = rgNodes[nNode];
    ss << nNode << "[" << pgn->RElConst() << "] C:";
    const _TyGL * const * ppgl = pgn->PPGLChildHead();
    for ( ; *ppgl; ppgl = (*ppgl)->PPGLGetNextChild() )
    {
      const _TyGL * pgl = *ppgl;
      if ( pgl->PGNParent() != pgn )
        ss << "!BADPARENT!";
      if ( pgl->PPGLGetThisChild() != ppgl )
        ss << "!BADPREVCHILD!";
      if ( !pgl->PGNChild() )
      {
        ss << "!NULLCHILD!";
        continue;
      }
      bool fFound = false; // The link must be in the child's parent list.
      for ( const _TyGL * const * ppglFind = pgl->PGNChild()->PPGLParentHead(); *ppglFind; ppglFind = (*ppglFind)->PPGLGetNextParent() )
        if ( *ppglFind == pgl )
          fFound = true;
      if ( !fFound )
        ss << "!NOTINPARENTLIST!";
      ss << " (" << pgl->RElConst() << ")>" << lambdaId( pgl->PGNChild() );
    }
    ss << " P:";
    ppgl = pgn->PPGLParentHead();
    for ( ; *ppgl; ppgl = (*ppgl)->PPGLGetNextParent() )
    {
      const _TyGL * pgl = *ppgl;
      if ( pgl->PGNChild() != pgn )
        ss << "!BADCHILD!";
      if ( pgl->PPGLGetThisParent() != ppgl )
        ss << "!BADPREVPARENT!";
      if ( !pgl->PGNParent() )
      {
        ss << "!NULLPARENT!";
        continue;
      }
      bool fFound = false; // The link must be in the parent's child list.
      for ( const _TyGL * const * ppglFind = pgl->PGNParent()->PPGLChildHead(); *ppglFind; ppglFind = (*ppglFind)->PPGLGetNextChild() )
        if ( *ppglFind == pgl )
          fFound = true;
      if ( !fFound )
        ss << "!NOTINCHILDLIST!";
      ss << " (" << pgl->RElConst() << ")<" << lambdaId( pgl->PGNParent() );
    }
    ss << "\n";
  }
  return ss.str();
}
static bool FDescriptionOk( std::string const & _rstr )
{
  return _rstr.find( '!' ) == std::string::npos;
}

// Collect every node connected to the root.
template < class t_TyGraph >
void CollectNodes( t_TyGraph const & _g, std::vector< typename t_TyGraph::_TyGraphNode * > & _rrgNodes )
{
  typedef typename t_TyGraph::_TyGraphNode _TyGN;
  typedef typename t_TyGraph::_TyGraphLink _TyGL;
  _rrgNodes.clear();
  const _TyGN * pgnRoot = _g.get_root();
  if ( !pgnRoot )
    return;
  std::set< const _TyGN * > setSeen;
  std::deque< const _TyGN * > dq;
  dq.push_back( pgnRoot );
  setSeen.insert( pgnRoot );
  while ( !dq.empty() )
  {
    const _TyGN * pgn = dq.front();
    dq.pop_front();
    _rrgNodes.push_back( const_cast< _TyGN * >( pgn ) );
    for ( const _TyGL * const * ppgl = pgn->PPGLChildHead(); *ppgl; ppgl = (*ppgl)->PPGLGetNextChild() )
      if ( setSeen.insert( (*ppgl)->PGNChild() ).second )
        dq.push_back( (*ppgl)->PGNChild() );
    for ( const _TyGL * const * ppgl = pgn->PPGLParentHead(); *ppgl; ppgl = (*ppgl)->PPGLGetNextParent() )
      if ( setSeen.insert( (*ppgl)->PGNParent() ).second )
        dq.push_back( (*ppgl)->PGNParent() );
  }
}

// Link selection objects for controlled iteration.
struct SelectLinkNotMultipleOf3
{
  template < class t_TyLink >
  bool operator()( const t_TyLink * _pgl ) const
  {
    return ( _pgl->RElConst() % 3 ) != 0;
  }
};
struct SelectAllLinks
{
  template < class t_TyLink >
  bool operator()( const t_TyLink * ) const
  {
    return true;
  }
};

// Compute independently what an iteration from the root must visit.
template < class t_TyGraph, class t_TySelect >
void ComputeExpectedIteration( t_TyGraph const & _g, bool _fClosedDirected, bool _fDirectionDown, t_TySelect const & _rselect,
                               std::set< const void * > & _rsetExpected )
{
  typedef typename t_TyGraph::_TyGraphNode _TyGN;
  typedef typename t_TyGraph::_TyGraphLink _TyGL;
  _rsetExpected.clear();
  const _TyGN * pgnRoot = _g.get_root();
  if ( !pgnRoot )
    return;
  std::deque< const _TyGN * > dq;
  dq.push_back( pgnRoot );
  _rsetExpected.insert( pgnRoot );
  while ( !dq.empty() )
  {
    const _TyGN * pgn = dq.front();
    dq.pop_front();
    if ( !_fClosedDirected || _fDirectionDown )
      for ( const _TyGL * const * ppgl = pgn->PPGLChildHead(); *ppgl; ppgl = (*ppgl)->PPGLGetNextChild() )
        if ( _rselect( *ppgl ) )
        {
          _rsetExpected.insert( *ppgl );
          if ( _rsetExpected.insert( (*ppgl)->PGNChild() ).second )
            dq.push_back( (*ppgl)->PGNChild() );
        }
    if ( !_fClosedDirected || !_fDirectionDown )
      for ( const _TyGL * const * ppgl = pgn->PPGLParentHead(); *ppgl; ppgl = (*ppgl)->PPGLGetNextParent() )
        if ( _rselect( *ppgl ) )
        {
          _rsetExpected.insert( *ppgl );
          if ( _rsetExpected.insert( (*ppgl)->PGNParent() ).second )
            dq.push_back( (*ppgl)->PGNParent() );
        }
  }
}

// Run the iterator to the end (or to a maximum number of elements) recording each element visited.
template < class t_TyIter >
void RunIteration( t_TyIter & _rit, std::vector< const void * > & _rrgVisited, size_t _nMaxElements )
{
  for ( ; !_rit.FAtEnd() && ( _rrgVisited.size() < _nMaxElements ); ++_rit )
    _rrgVisited.push_back( _rit.PGLCur() ? (const void*)_rit.PGLCur() : (const void*)_rit.PGNCur() );
}

template < class t_TyGraph, class t_TyIter, class t_TySelect >
void CheckIteration( t_TyGraph const & _g, t_TyIter & _rit, bool _fClosedDirected, bool _fDirectionDown, t_TySelect const & _rselect,
                     const char * _pszWhat, std::string const & _rstrCase, std::vector< const void * > * _prgVisited = nullptr )
{
  std::set< const void * > setExpected;
  ComputeExpectedIteration( _g, _fClosedDirected, _fDirectionDown, _rselect, setExpected );
  std::vector< const void * > rgVisited;
  RunIteration( _rit, rgVisited, setExpected.size() * 3 + 10 );
  std::set< const void * > setVisited( rgVisited.begin(), rgVisited.end() );
  std::stringstream ssCase;
  ssCase << _pszWhat << " closed-directed=" << _fClosedDirected << " down=" << _fDirectionDown << " (" << _rstrCase << ")";
  EXPECT_TRUE( _rit.FAtEnd() ) << "Iteration did not end: " << ssCase.str();
  EXPECT_EQ( setVisited.size(), rgVisited.size() ) << "Elements were visited more than once: " << ssCase.str();
  EXPECT_TRUE( setVisited == setExpected ) << "Visited " << setVisited.size() << " distinct elements but expected " << setExpected.size() << ": " << ssCase.str();
  if ( _prgVisited )
    _prgVisited->swap( rgVisited );
}

template < class t_TyGraph >
void TestIterations( t_TyGraph const & _g, std::string const & _rstrCase, bool _fTestIterCopy )
{
  typedef typename t_TyGraph::const_iterator _TyIter;
  typedef typename t_TyGraph::_TyGraphTraits:: template _get_link_select_iter< SelectLinkNotMultipleOf3 >::_TyGraphFwdIterSelectConst _TySelectIter;
  for ( int nMode = 0; nMode < 4; ++nMode )
  {
    const bool fClosedDirected = !!( nMode & 1 );
    const bool fDirectionDown = !!( nMode & 2 );
    std::vector< const void * > rgVisited;
    {//B
      _TyIter it( _g.get_root(), 0, fClosedDirected, fDirectionDown, _g.get_base_path_allocator() );
      CheckIteration( _g, it, fClosedDirected, fDirectionDown, SelectAllLinks(), "iteration", _rstrCase, &rgVisited );
    }//EB
    {//B
      _TySelectIter it( _g.get_root(), 0, fClosedDirected, fDirectionDown, _g.get_base_path_allocator(), SelectLinkNotMultipleOf3() );
      CheckIteration( _g, it, fClosedDirected, fDirectionDown, SelectLinkNotMultipleOf3(), "select iteration", _rstrCase );
    }//EB
    if ( !_fTestIterCopy )
      continue;
    // Copy construct and assign the iterator at every position and check that each continues identically.
    const size_t knMaxElements = rgVisited.size() * 3 + 10;
    for ( size_t nCopyAt = 0; nCopyAt <= rgVisited.size(); ++nCopyAt )
    {
      _TyIter it( _g.get_root(), 0, fClosedDirected, fDirectionDown, _g.get_base_path_allocator() );
      std::vector< const void * > rgOrig;
      RunIteration( it, rgOrig, nCopyAt );
      _TyIter itCopy( it );
      // Assign over an iterator which is itself part way through an iteration:
      _TyIter itAssigned( _g.get_root(), 0, fClosedDirected, fDirectionDown, _g.get_base_path_allocator() );
      std::vector< const void * > rgScratch;
      RunIteration( itAssigned, rgScratch, nCopyAt / 2 );
      itAssigned = it;
      std::vector< const void * > rgCopy( rgOrig );
      std::vector< const void * > rgAssigned( rgOrig );
      RunIteration( it, rgOrig, knMaxElements );
      RunIteration( itCopy, rgCopy, knMaxElements );
      RunIteration( itAssigned, rgAssigned, knMaxElements );
      if ( ( rgOrig != rgVisited ) || ( rgCopy != rgVisited ) || ( rgAssigned != rgVisited ) )
      {
        std::stringstream ssCase;
        ssCase << "closed-directed=" << fClosedDirected << " down=" << fDirectionDown << " copied at step " << nCopyAt << " (" << _rstrCase << ")";
        EXPECT_TRUE( rgOrig == rgVisited ) << "The copied-from iterator diverged: " << ssCase.str();
        EXPECT_TRUE( rgCopy == rgVisited ) << "The copy constructed iterator diverged: " << ssCase.str();
        EXPECT_TRUE( rgAssigned == rgVisited ) << "The assigned iterator diverged: " << ssCase.str();
        break;
      }
    }
  }
}

// Copy starting from arbitrary nodes in both directions using _graph_copy_struct directly.
template < class t_TyGraph >
void TestDirectionalCopy( t_TyGraph & _g, std::string const & _rstrOrig, std::string const & _rstrCase )
{
  typedef typename t_TyGraph::_TyGraphNode _TyGN;
  std::vector< _TyGN * > rgNodes;
  CollectNodes( _g, rgNodes );
  _TyGN * const pgnRootOrig = _g.get_root();
  for ( size_t nNode = 0; ( nNode < rgNodes.size() ) && ( nNode < 6 ); ++nNode )
    for ( int nDown = 0; nDown < 2; ++nDown )
    {
      // We expect the same graph as seen from this node.
      _g.set_root_node( rgNodes[nNode] );
      std::string strExpect = StrDescribeGraph( _g );
      _g.set_root_node( pgnRootOrig );
      t_TyGraph gDst;
      {//B
        _graph_copy_struct< t_TyGraph, t_TyGraph, typename t_TyGraph::_TyGraphBase::_TyPathNodeBaseAllocatorAsPassed >
          gcs( _g, gDst, false, !!nDown, _g.get_base_path_allocator() );
        gcs.m_pgnSrcCopyRoot = rgNodes[nNode];
        gcs.copy();
        gDst.set_root_node( gcs.PGNTransferNewRoot() );
      }//EB
      EXPECT_EQ( StrDescribeGraph( gDst ), strExpect ) << "Copy from node " << nNode << " down=" << nDown << " (" << _rstrCase << ")";
    }
  EXPECT_EQ( StrDescribeGraph( _g ), _rstrOrig ) << "The source graph was changed by copying (" << _rstrCase << ")";
}

// The random graph generator never creates links from a node to itself - add some at random positions in the lists.
template < class t_TyGraph >
void AddSelfLinks( t_TyGraph & _g, int _nSelfLinks, unsigned _uSeed )
{
  typedef typename t_TyGraph::_TyGraphNode _TyGN;
  if ( !_nSelfLinks )
    return;
  std::vector< _TyGN * > rgNodes;
  CollectNodes( _g, rgNodes );
  srand( _uSeed * 7919 + 13 );
  for ( int nLink = 0; nLink < _nSelfLinks; ++nLink )
  {
    _TyGN * pgn = rgNodes[ rand() % rgNodes.size() ];
    typename t_TyGraph::_TyGraphLink * pgl = _g.create_link1( 100 + nLink );
    auto ppglbChild = pgn->PPGLBChildHead();
    for ( int nSkip = rand() % 3; nSkip-- && *ppglbChild; )
      ppglbChild = (*ppglbChild)->PPGLBGetNextChild();
    auto ppglbParent = pgn->PPGLBParentHead();
    for ( int nSkip = rand() % 3; nSkip-- && *ppglbParent; )
      ppglbParent = (*ppglbParent)->PPGLBGetNextParent();
    pgn->AddChild( *pgn, *pgl, *ppglbChild, *ppglbParent );
  }
}

template < class t_TyGraph >
void TestOneRandomGraph( int _nNodes, int _nExtraLinks, unsigned _uSeed, int _nSelfLinks, bool _fTestIterCopy )
{
  std::stringstream ssCase;
  ssCase << "nodes=" << _nNodes << " extralinks=" << _nExtraLinks << " seed=" << _uSeed << " selflinks=" << _nSelfLinks;
  const std::string strCase = ssCase.str();
  const ElCounts ecStart;
  {//B
    t_TyGraph g;
    g.replace_random( _nNodes, _nExtraLinks, _uSeed );
    AddSelfLinks( g, _nSelfLinks, _uSeed );
    const int knLinks = _nNodes - 1 + ( ( _nNodes > 1 ) ? _nExtraLinks : 0 ) + _nSelfLinks;
    ElCounts ecGraph( ecStart );
    ecGraph.m_nNodesAllocated += _nNodes;
    ecGraph.m_nNodesConstructed += _nNodes;
    ecGraph.m_nLinksAllocated += knLinks;
    ecGraph.m_nLinksConstructed += knLinks;
    EXPECT_EQ( ElCounts(), ecGraph ) << strCase;

    const std::string strOrig = StrDescribeGraph( g );
    ASSERT_TRUE( FDescriptionOk( strOrig ) ) << "Generated graph is corrupt (" << strCase << "):\n" << strOrig;

    TestIterations( g, strCase, _fTestIterCopy );
    TestDirectionalCopy( g, strOrig, strCase );
    EXPECT_EQ( ElCounts(), ecGraph ) << "After directional copies: " << strCase;

    {//B - dump just to exercise the code - the output contains addresses.
      std::stringstream ssDump;
      g.dump( ssDump );
      EXPECT_FALSE( ssDump.str().empty() ) << strCase;
    }//EB
    {//B - copy construction.
      t_TyGraph gCopy( g );
      EXPECT_EQ( StrDescribeGraph( gCopy ), strOrig ) << "Copy constructed graph differs (" << strCase << ")";
      ElCounts ecBoth( ecGraph );
      ecBoth.m_nNodesAllocated += _nNodes;
      ecBoth.m_nNodesConstructed += _nNodes;
      ecBoth.m_nLinksAllocated += knLinks;
      ecBoth.m_nLinksConstructed += knLinks;
      EXPECT_EQ( ElCounts(), ecBoth ) << "After copy: " << strCase;
      auto prCompare = g.compare_values( gCopy );
      EXPECT_TRUE( prCompare.first.FAtEnd() && prCompare.second.FAtEnd() ) << "compare_values() found a difference (" << strCase << ")";
    }//EB
    EXPECT_EQ( ElCounts(), ecGraph ) << "After destroying copy: " << strCase;
    {//B - save and load.
      std::stringstream ss( std::ios::in | std::ios::out | std::ios::binary );
      g.save( ss );
      t_TyGraph gLoaded;
      gLoaded.replace_load( ss );
      EXPECT_EQ( StrDescribeGraph( gLoaded ), strOrig ) << "Loaded graph differs (" << strCase << ")";
    }//EB
    EXPECT_EQ( ElCounts(), ecGraph ) << "After destroying loaded graph: " << strCase;
    EXPECT_EQ( StrDescribeGraph( g ), strOrig ) << "The graph was changed by the tests (" << strCase << ")";
  }//EB
  EXPECT_EQ( ElCounts(), ecStart ) << "After destruction: " << strCase;
}

template < class t_TyGraph >
void TestRandomGraphs( int _nMaxNodes, unsigned _nSeeds, int _nMaxNodesIterCopy )
{
  for ( int nNodes = 1; nNodes <= _nMaxNodes; ++nNodes )
    for ( int nExtraLinks = 0; nExtraLinks <= 2 * nNodes; ++nExtraLinks )
      for ( unsigned uSeed = 1; uSeed <= _nSeeds; ++uSeed )
        for ( int nSelfLinks = 0; nSelfLinks <= 2; ++nSelfLinks )
        {
          TestOneRandomGraph< t_TyGraph >( nNodes, nExtraLinks, uSeed, nSelfLinks, nNodes <= _nMaxNodesIterCopy );
          if ( FTooManyFailures() )
            return;
        }
}

TEST( DGraphRandomGraphs, NotSafeGraph )
{
  TestRandomGraphs< _TyGraphNotSafe >( 10, 6, 6 );
}
TEST( DGraphRandomGraphs, SafeGraph )
{
  TestRandomGraphs< _TyGraphSafe >( 10, 6, 6 );
}

// This is the smallest graph for which forward iteration used to end early: on the way down both the root and node 1
//  are unfinished when node 1's last parent link is reached.
TEST( DGraphIteration, TwoUnfinishedNodesOneToBeRemoved )
{
  typedef _TyGraphNotSafe _TyGraph;
  _TyGraph g;
  _TyGraph::_TyGraphNode * pgn0 = g.create_node1( 0 );
  _TyGraph::_TyGraphNode * pgn1 = g.create_node1( 1 );
  _TyGraph::_TyGraphNode * pgn2 = g.create_node1( 2 );
  g.set_root_node( pgn0 );
  // 0 -> 1 twice, 1 -> 0, 2 -> 0.
  for ( int nLink = 0; nLink < 2; ++nLink )
    pgn0->AddChild( *pgn1, *g.create_link1( 10 + nLink ), *pgn0->PPGLBChildHead(), *pgn1->PPGLBParentHead() );
  pgn1->AddChild( *pgn0, *g.create_link1( 20 ), *pgn1->PPGLBChildHead(), *pgn0->PPGLBParentHead() );
  pgn2->AddChild( *pgn0, *g.create_link1( 30 ), *pgn2->PPGLBChildHead(), *pgn0->PPGLBParentHead() );
  size_t nNodes = 0, nLinks = 0;
  for ( _TyGraph::const_iterator it( g.begin() ); !it.FAtEnd(); ++it )
    it.PGLCur() ? ++nLinks : ++nNodes;
  EXPECT_EQ( nNodes, 3u );
  EXPECT_EQ( nLinks, 4u );
}

// List operations: a hub node with <k> children (or parents) whose link values are 0..k-1 in list order.
class DGraphListOps : public testing::Test
{
public:
  typedef _TyGraphNotSafe _TyGraph;
  typedef _TyGraph::_TyGraphNode _TyGN;
  typedef _TyGraph::_TyGraphLink _TyGL;
  typedef _TyGN::_TyGraphLinkBaseBase _TyGLB;
  typedef _TyGraph::_TyNodeIterNonConst _TyNodeIter;
  typedef _TyGraph::_TyLinkPosIterNonConst _TyLinkPosIter;
  static const int s_knMaxListSize = 6;

  static void BuildHub( _TyGraph & _g, int _nRelations, bool _fChildren )
  {
    _g.destroy();
    _TyGN * pgnHub = _g.create_node1( 1000 );
    _g.set_root_node( pgnHub );
    for ( int nRel = _nRelations; nRel--; )
    {
      _TyGN * pgn = _g.create_node1( nRel );
      _TyGL * pgl = _g.create_link1( nRel );
      if ( _fChildren )
        pgnHub->AddChild( *pgn, *pgl, *pgnHub->PPGLBChildHead(), *pgn->PPGLBParentHead() );
      else
        pgnHub->AddParent( *pgn, *pgl, *pgnHub->PPGLBParentHead(), *pgn->PPGLBChildHead() );
    }
  }
  // Return the values of the links in list order - checking the integrity of the list.
  static std::vector< int > GetOrder( _TyGraph & _g, bool _fChildren, std::string & _rstrError )
  {
    std::vector< int > rgOrder;
    _rstrError.clear();
    _TyGN * pgnHub = _g.get_root();
    _TyGL ** ppgl = _fChildren ? pgnHub->PPGLChildHead() : pgnHub->PPGLParentHead();
    for ( int nLink = 0; *ppgl; ++nLink )
    {
      if ( nLink > 100 )
      {
        _rstrError = "cycle in list";
        break;
      }
      _TyGL * pgl = *ppgl;
      if ( ( _fChildren ? pgl->PPGLGetThisChild() : pgl->PPGLGetThisParent() ) != ppgl )
        _rstrError = "bad previous pointer";
      if ( ( _fChildren ? pgl->PGNParent() : pgl->PGNChild() ) != pgnHub )
        _rstrError = "bad hub node pointer";
      rgOrder.push_back( pgl->RElConst() );
      ppgl = _fChildren ? pgl->PPGLGetNextChild() : pgl->PPGLGetNextParent();
    }
    return rgOrder;
  }
  static _TyGLB * PGLBNth( _TyGraph & _g, bool _fChildren, int _nLink )
  {
    _TyGLB ** ppglb = _fChildren ? _g.get_root()->PPGLBChildHead() : _g.get_root()->PPGLBParentHead();
    while ( _nLink-- )
      ppglb = _fChildren ? (*ppglb)->PPGLBGetNextChild() : (*ppglb)->PPGLBGetNextParent();
    return *ppglb;
  }
  static _TyLinkPosIter LinkPosIterNth( _TyGraph & _g, bool _fChildren, int _nLink )
  {
    _TyGLB * pglb = PGLBNth( _g, _fChildren, _nLink );
    return _TyLinkPosIter( (_TyGL**)( _fChildren ? pglb->PPGLBGetThisChild() : pglb->PPGLBGetThisParent() ) );
  }
  static std::vector< int > Iota( int _n )
  {
    std::vector< int > rg( _n );
    for ( int n = 0; n < _n; ++n )
      rg[n] = n;
    return rg;
  }
  // Build a hub, apply the operation, and check the resulting list order.
  template < class t_TyFunctor >
  static void CheckOp( const char * _pszOp, int _nRelations, int _nArg1, int _nArg2, bool _fChildren, std::vector< int > const & _rrgExpect, t_TyFunctor && _rrftor )
  {
    if ( FTooManyFailures() )
      return;
    std::stringstream ssCase;
    ssCase << _pszOp << "(" << _nArg1 << "," << _nArg2 << ") on " << _nRelations << ( _fChildren ? " children" : " parents" );
    _TyGraph g;
    BuildHub( g, _nRelations, _fChildren );
    EXPECT_NO_THROW( _rrftor( g ) ) << ssCase.str();
    std::string strError;
    std::vector< int > rgOrder = GetOrder( g, _fChildren, strError );
    EXPECT_TRUE( strError.empty() ) << ssCase.str() << ": " << strError;
    EXPECT_EQ( rgOrder, _rrgExpect ) << ssCase.str();
    if ( !strError.empty() )
      g.set_root_node( 0 ); // The lists are corrupt - leak the graph rather than crash destroying it.
  }
};

TEST_F( DGraphListOps, Exchange )
{
  for ( int nChildren = 0; nChildren < 2; ++nChildren )
  {
    const bool fC = !!nChildren;
    for ( int nRelations = 1; nRelations <= s_knMaxListSize; ++nRelations )
      for ( int n1 = 0; n1 < nRelations; ++n1 )
        for ( int n2 = 0; n2 < nRelations; ++n2 )
        {
          std::vector< int > rgExpect = Iota( nRelations );
          std::swap( rgExpect[n1], rgExpect[n2] );
          // Any two links in either order, adjacent or not, including a link with itself.
          CheckOp( "link Exchange", nRelations, n1, n2, fC, rgExpect, [=]( _TyGraph & _g )
          {
            fC ? _TyGLB::ExchangeChildren( PGLBNth( _g, fC, n1 ), PGLBNth( _g, fC, n2 ) ) : _TyGLB::ExchangeParents( PGLBNth( _g, fC, n1 ), PGLBNth( _g, fC, n2 ) );
          } );
          CheckOp( "link ExchangeOrdered", nRelations, n1, n2, fC, rgExpect, [=]( _TyGraph & _g )
          {
            fC ? _TyGLB::ExchangeChildrenOrdered( PGLBNth( _g, fC, n1 ), PGLBNth( _g, fC, n2 ) ) : _TyGLB::ExchangeParentsOrdered( PGLBNth( _g, fC, n1 ), PGLBNth( _g, fC, n2 ) );
          } );
          if ( n1 < n2 )
            CheckOp( "node ExchangeOrdered", nRelations, n1, n2, fC, rgExpect, [=]( _TyGraph & _g )
            {
              fC ? _g.get_root()->ExchangeChildrenOrdered( n1, n2 ) : _g.get_root()->ExchangeParentsOrdered( n1, n2 );
            } );
          if ( n1 != n2 )
          {
            CheckOp( "node iterator Exchange", nRelations, n1, n2, fC, rgExpect, [=]( _TyGraph & _g )
            {
              _TyNodeIter ni( _g.get_root() );
              fC ? ni.ExchangeChildren( n1, n2 ) : ni.ExchangeParents( n1, n2 );
            } );
            CheckOp( "link iterator ExchangeSibling", nRelations, n1, n2, fC, rgExpect, [=]( _TyGraph & _g )
            {
              _TyLinkPosIter lpi1( LinkPosIterNth( _g, fC, n1 ) );
              _TyLinkPosIter lpi2( LinkPosIterNth( _g, fC, n2 ) );
              fC ? lpi1.ExchangeSiblingChildren( lpi2 ) : lpi1.ExchangeSiblingParents( lpi2 );
            } );
          }
        }
  }
}

// MoveUp( remove, insert ): the link at <remove> ends up immediately before the link originally at <insert> - or at the end.
// MoveDown( remove, insert ): the link at <remove> becomes the link at <insert>.
TEST_F( DGraphListOps, Move )
{
  for ( int nChildren = 0; nChildren < 2; ++nChildren )
  {
    const bool fC = !!nChildren;
    for ( int nRelations = 1; nRelations <= s_knMaxListSize; ++nRelations )
      for ( int nRemove = 0; nRemove < nRelations; ++nRemove )
        for ( int nInsert = 0; nInsert <= nRelations + 1; ++nInsert )
        {
          std::vector< int > rgExpect = Iota( nRelations );
          rgExpect.erase( rgExpect.begin() + nRemove );
          if ( nRemove < nInsert )
          {
            rgExpect.insert( rgExpect.begin() + ( (std::min)( nInsert, nRelations ) - 1 ), nRemove );
            CheckOp( "MoveUp", nRelations, nRemove, nInsert, fC, rgExpect, [=]( _TyGraph & _g )
            {
              fC ? _g.get_root()->MoveChildUp( nRemove, nInsert ) : _g.get_root()->MoveParentUp( nRemove, nInsert );
            } );
          }
          else
          {
            rgExpect.insert( rgExpect.begin() + nInsert, nRemove );
            CheckOp( "MoveDown", nRelations, nRemove, nInsert, fC, rgExpect, [=]( _TyGraph & _g )
            {
              fC ? _g.get_root()->MoveChildDown( nRemove, nInsert ) : _g.get_root()->MoveParentDown( nRemove, nInsert );
            } );
          }
          CheckOp( "Move", nRelations, nRemove, nInsert, fC, rgExpect, [=]( _TyGraph & _g )
          {
            fC ? _g.get_root()->MoveChild( nRemove, nInsert ) : _g.get_root()->MoveParent( nRemove, nInsert );
          } );
          CheckOp( "node iterator Move", nRelations, nRemove, nInsert, fC, rgExpect, [=]( _TyGraph & _g )
          {
            _TyNodeIter ni( _g.get_root() );
            fC ? ni.MoveChild( nRemove, nInsert ) : ni.MoveParent( nRemove, nInsert );
          } );
        }
  }
}

TEST_F( DGraphListOps, CountBeforeAfterAndPrevious )
{
  for ( int nChildren = 0; nChildren < 2; ++nChildren )
  {
    const bool fC = !!nChildren;
    for ( int nRelations = 1; nRelations <= s_knMaxListSize; ++nRelations )
    {
      _TyGraph g;
      BuildHub( g, nRelations, fC );
      for ( int nLink = 0; nLink < nRelations; ++nLink )
      {
        std::stringstream ssCase;
        ssCase << "link " << nLink << " of " << nRelations << ( fC ? " children" : " parents" );
        _TyGLB * pglb = PGLBNth( g, fC, nLink );
        EXPECT_EQ( fC ? pglb->UChildrenBefore() : pglb->UParentsBefore(), (_TyGNIndex)nLink ) << ssCase.str();
        _TyLinkPosIter lpi( LinkPosIterNth( g, fC, nLink ) );
        EXPECT_EQ( fC ? lpi.UChildrenBefore() : lpi.UParentsBefore(), (_TyGNIndex)nLink ) << ssCase.str();
        // "After" counts from the current link to the end of the list.
        EXPECT_EQ( fC ? lpi.UChildrenAfter() : lpi.UParentsAfter(), (_TyGNIndex)( nRelations - nLink ) ) << ssCase.str();
        if ( nLink )
          EXPECT_EQ( fC ? pglb->PGLBGetPrevChild() : pglb->PGLBGetPrevParent(), PGLBNth( g, fC, nLink - 1 ) ) << ssCase.str();
      }
    }
  }
}

TEST_F( DGraphListOps, IteratorSwap )
{
  _TyGraph g;
  BuildHub( g, 3, true );
  // swap() requires exactly matching types so we swap at the level of the base iterators.
  typedef _TyGraph::_TyGraphBase::_TyGraphNodeIterBaseNotSafe _TyNodeIterBase;
  typedef _TyGraph::_TyGraphBase::_TyGraphLinkPosIterBaseNotSafe _TyLinkPosIterBase;
  _TyNodeIter ni1( g.get_root() );
  _TyNodeIter ni2( static_cast< _TyGN * >( PGLBNth( g, true, 0 )->PGNBChild() ) );
  _TyGN * pgn1 = ni1.PGNCur();
  _TyGN * pgn2 = ni2.PGNCur();
  static_cast< _TyNodeIterBase & >( ni1 ).swap( static_cast< _TyNodeIterBase & >( ni2 ) );
  EXPECT_EQ( ni1.PGNCur(), pgn2 );
  EXPECT_EQ( ni2.PGNCur(), pgn1 );
  _TyLinkPosIter lpi1( LinkPosIterNth( g, true, 0 ) );
  _TyLinkPosIter lpi2( LinkPosIterNth( g, true, 1 ) );
  _TyGL * pgl1 = lpi1.PGLCur();
  _TyGL * pgl2 = lpi2.PGLCur();
  static_cast< _TyLinkPosIterBase & >( lpi1 ).swap( static_cast< _TyLinkPosIterBase & >( lpi2 ) );
  EXPECT_EQ( lpi1.PGLCur(), pgl2 );
  EXPECT_EQ( lpi2.PGLCur(), pgl1 );
}

// An index beyond the end of a list must throw a catchable exception and leave the list unchanged.
TEST_F( DGraphListOps, IndexBeyondEndThrows )
{
  for ( int nChildren = 0; nChildren < 2; ++nChildren )
  {
    const bool fC = !!nChildren;
    _TyGraph g;
    BuildHub( g, 3, fC );
    _TyGN * pgnHub = g.get_root();
    EXPECT_THROW( fC ? pgnHub->PGNBGetNthChild( 3 ) : pgnHub->PGNBGetNthParent( 3 ), _graph_nav_except );
    EXPECT_THROW( fC ? pgnHub->MoveChildUp( 5, 9 ) : pgnHub->MoveParentUp( 5, 9 ), _graph_nav_except );
    EXPECT_THROW( fC ? pgnHub->MoveChildDown( 7, 0 ) : pgnHub->MoveParentDown( 7, 0 ), _graph_nav_except );
    EXPECT_THROW( fC ? pgnHub->MoveChild( 5, 9 ) : pgnHub->MoveParent( 5, 9 ), _graph_nav_except );
    EXPECT_THROW( fC ? pgnHub->MoveChild( 7, 0 ) : pgnHub->MoveParent( 7, 0 ), _graph_nav_except );
    {//B
      _TyNodeIter ni( pgnHub );
      EXPECT_THROW( fC ? ni.MoveChild( 7, 0 ) : ni.MoveParent( 7, 0 ), _graph_nav_except );
      EXPECT_THROW( fC ? ni.GoChild( 3 ) : ni.GoParent( 3 ), _graph_nav_except );
      EXPECT_THROW( ni.GoRelation( fC, 3 ), _graph_nav_except );
      EXPECT_EQ( ni.PGNCur(), pgnHub );
    }//EB
    std::string strError;
    EXPECT_EQ( GetOrder( g, fC, strError ), Iota( 3 ) );
    EXPECT_TRUE( strError.empty() ) << strError;
  }
  // Exchanging with a null link:
  EXPECT_THROW( _TyGLB::ExchangeParents( nullptr, nullptr ), _graph_nav_except );
}

TEST( DGraphAssignment, SameTypeAssignmentCopiesTheGraph )
{
  typedef _TyGraphNotSafe _TyGraph;
  const ElCounts ecStart;
  {//B
    _TyGraph gA, gB;
    gA.replace_random( 5, 3, 1 );
    gB.replace_random( 7, 4, 2 );
    const std::string strB = StrDescribeGraph( gB );
    gA = gB;
    EXPECT_NE( gA.get_root(), gB.get_root() ) << "Assignment must not share the root between the two graphs.";
    EXPECT_EQ( StrDescribeGraph( gA ), strB );
    EXPECT_EQ( StrDescribeGraph( gB ), strB );
    EXPECT_EQ( gs_iNodesAllocated - ecStart.m_nNodesAllocated, 14 ) << "The old contents of the assigned-to graph must be destroyed.";
    // Self assignment and self copy must leave the graph alone:
    _TyGraph::_TyGraphNode * pgnRoot = gA.get_root();
    _TyGraph & rgA = gA; // avoid the compiler's self assignment warning.
    gA = rgA;
    EXPECT_EQ( gA.get_root(), pgnRoot );
    gA.replace_copy( gA );
    EXPECT_EQ( gA.get_root(), pgnRoot );
    EXPECT_EQ( StrDescribeGraph( gA ), strB );
    EXPECT_EQ( gs_iNodesAllocated - ecStart.m_nNodesAllocated, 14 );
  }//EB
  EXPECT_EQ( ElCounts(), ecStart );
}

// Copy objects which add 1000 to each element so that we can see that every element went through them.
struct CopyNodeAdd1000
{
  typedef _TyGraphNotSafe::_TyGraphNode _TyGN;
  void operator()( _TyGN & _rgnDst, _TyGN const & _rgnSrc )
  {
    _TyGraphNotSafe::_construct_node_el1< int >( &_rgnDst, _rgnSrc.RElConst() + 1000 );
  }
};
struct CopyLinkAdd1000
{
  typedef _TyGraphNotSafe::_TyGraphLink _TyGL;
  void operator()( _TyGL & _rglDst, _TyGL const & _rglSrc )
  {
    _TyGraphNotSafe::_construct_link_el1< int >( &_rglDst, _rglSrc.RElConst() + 1000 );
  }
};

TEST( DGraphCopy, CustomCopyObjectsAreUsedForEveryElement )
{
  typedef _TyGraphNotSafe _TyGraph;
  for ( unsigned uSeed = 1; uSeed <= 20; ++uSeed )
  {
    _TyGraph gSrc, gDst;
    gSrc.replace_random( 12, 10, uSeed );
    gDst.replace_copy( gSrc, CopyNodeAdd1000(), CopyLinkAdd1000() );
    size_t nNodes = 0, nLinks = 0, nNodesNotCopied = 0, nLinksNotCopied = 0;
    for ( _TyGraph::const_iterator it( gDst.begin() ); !it.FAtEnd(); ++it )
    {
      if ( it.PGLCur() )
      {
        ++nLinks;
        if ( it.PGLCur()->RElConst() < 1000 )
          ++nLinksNotCopied;
      }
      else
      {
        ++nNodes;
        if ( it.PGNCur()->RElConst() < 1000 )
          ++nNodesNotCopied;
      }
    }
    EXPECT_EQ( nNodes, 12u ) << "seed " << uSeed;
    EXPECT_EQ( nLinks, 21u ) << "seed " << uSeed;
    EXPECT_EQ( nNodesNotCopied, 0u ) << "seed " << uSeed;
    EXPECT_EQ( nLinksNotCopied, 0u ) << "seed " << uSeed;
  }
}

} // namespace ns_DGraphGTest

int main( int argc, char **argv )
{
#ifdef WIN32
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif //WIN32
  __BIENUTIL_USING_NAMESPACE

  g_strProgramName = argv[0];
#ifdef WIN32
  {//B
    // Sometimes in windows we get called from cmake and the path is using Linux path separators:
    std::filesystem::path pathProgramName( g_strProgramName );
    g_strProgramName = pathProgramName.lexically_normal().string().c_str();
  }//EB
#endif //WIN32
	n_SysLog::InitSysLog( g_strProgramName.c_str(), LOG_PERROR, LOG_USER );

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
