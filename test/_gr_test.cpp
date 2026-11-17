//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _gr_test.cpp

// Graph test stuff.
// This creates a random graph and then copies, dumps, saves and loads it (through iostreams, files and mapped files),
//	comparing the results against the original.
// When built without __NDEBUG_THROW the debug throw points (see _dbgthrw.h) are enabled and exceptions are thrown at
//	random from them: each operation is retried until it succeeds. This tests that state is maintained correctly when
//	an exception is thrown from any of those points.
// Returns non-zero if any comparison failed or if any graph elements were leaked.

#ifdef WIN32
#include <stdlib.h>
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif //WIN32
#define DBG_NEW new

#include <memory>
#include <exception>
typedef std::allocator< char >	_TyDefaultAllocator;
#include "_compat.h"
#include "_heapchk.h"

#include <time.h>
#include <string>
#include <fstream>
#include <iostream>
#include "syslogmgr.inl"
#include "_compat.inl"
#include "_gr_inc.h"
#include "_fdobjs.h"

std::string g_strProgramName;

__BIENUTIL_USING_NAMESPACE
__DGRAPH_USING_NAMESPACE
#include "_gr_tst0.h"

// The number of comparisons, etc. that have failed - this becomes the return value of main().
static int gs_nFailures = 0;
// When throwing is enabled each operation is retried until it succeeds. If an operation never succeeds
//	then something is wrong - don't loop forever.
static const int gs_knMaxTries = 1000000;
// See test_copy().
static const int gs_knCopyThrowPointsPerThrow = 4000;

void
print_time( const char * _cpMesg, time_t * _ptStart )
{
	time_t	tCur;
	time( &tCur );
	cout << "[" << ptrdiff_t( tCur - *_ptStart ) << "]:" << _cpMesg;
	cout.flush();
}

// Print the current element counts - constructed should always equal allocated between operations.
void
print_counts( const char * _cpWhere )
{
#ifdef __DGRAPH_COUNT_EL_ALLOC_LIFETIME
	cout << _cpWhere << ": Nodes allocated/constructed [" << gs_iNodesAllocated << "/" << gs_iNodesConstructed 
			 << "] links allocated/constructed [" << gs_iLinksAllocated << "/" << gs_iLinksConstructed << "]." << endl;
#endif //__DGRAPH_COUNT_EL_ALLOC_LIFETIME
}

void
usage( char * _cpApp )
{
  cerr << "main(): usage\n" << _cpApp << " <num nodes> <num extra links> <rand seed> [<throw one only>]\n";
}

void
report_failure( const char * _cpMesg )
{
	++gs_nFailures;
	cout << "FAILURE: " << _cpMesg << "\n";
	cout.flush();
}

// Call when an exception has been caught: with throwing enabled this is expected - otherwise it is a failure.
void
handle_caught( const char * _cpWhere, exception & _rexc )
{
#ifndef __NDEBUG_THROW
	if ( _throw_object_base::ms_tsb.m_rgttTypeAccum )
	{
		// Set GR_TEST_VERBOSE in the environment to see each of the thrown exceptions.
		static const bool s_kfVerbose = !!getenv( "GR_TEST_VERBOSE" );
		if ( s_kfVerbose )
			cout << _cpWhere << ": Caught [" << _rexc.what() << "].\n";
		_throw_object_base::ms_tsb.handle_throw();
		return;
	}
#endif //!__NDEBUG_THROW
	// This was not thrown by a debug throw point - it is unexpected.
	cout << _cpWhere << ": Unexpected exception [" << _rexc.what() << "].\n";
	report_failure( _cpWhere );
}

#ifndef __NDEBUG_THROW
void
print_hit_stats()
{
	unsigned uHit;
	size_t uPossible;
	_throw_object_base::ms_tsb.get_hit_stats( uHit, uPossible );
	cout << "Hit [" << uHit << "] out of [" << uPossible << "].\n";
	_throw_object_base::ms_tsb.report_unhit( cout );
	_throw_object_base::ms_tsb.clear_hit_map();
}
#endif //!__NDEBUG_THROW

// Retry <_rrftor> until it returns true - or we have a failure.
template < class t_TyFunctor >
bool
retry_until_success( const char * _cpWhat, t_TyFunctor && _rrftor )
{
	const int knFailuresStart = gs_nFailures;
	for ( int nTry = 0; nTry < gs_knMaxTries; ++nTry )
	{
		if ( _rrftor() )
			return true;
		if ( gs_nFailures != knFailuresStart )
			return false; // A real failure - retrying won't help.
	}
	cout << _cpWhat << ": Did not succeed after [" << gs_knMaxTries << "] tries.\n";
	report_failure( _cpWhat );
	return false;
}

template < class t_TyGraphLeft, class t_TyGraphRight >
bool
test_compare( t_TyGraphLeft const & _rgLeft,
              t_TyGraphRight const & _rgRight,
              time_t * _ptStart )
{
	// Turn off throw testing while comparing:
#ifndef __NDEBUG_THROW
	uint32_t grfWasOn = _throw_object_base::ms_tsb.m_grfOn;
	_throw_object_base::ms_tsb.set_on( 0 );
#endif //!__NDEBUG_THROW

	pair< typename t_TyGraphLeft::const_iterator_pos,
        typename t_TyGraphRight::const_iterator_pos >
		pitResult = _rgLeft.compare_values( _rgRight );
	bool fSame = pitResult.first.FAtEnd() && pitResult.second.FAtEnd();
	if ( fSame )
	{
		print_time( "test_compare(): Compare succeeded.\n", _ptStart );
	}
	else
	{
		print_time( "test_compare(): Compare FAILED.\n", _ptStart );
		report_failure( "test_compare(): The graphs differ - see left.txt and right.txt." );
    fstream fsDumpRight( "right.txt", ios::out );
		_rgRight.dump( fsDumpRight );
		fstream fsDumpLeft( "left.txt", ios::out );
		_rgLeft.dump( fsDumpLeft );
	}
#ifndef __NDEBUG_THROW
	_throw_object_base::ms_tsb.set_on( grfWasOn );
#endif //!__NDEBUG_THROW
	return fSame;
}

// Returns false if we should try again.
template < class t_TyGraphDst, class t_TyGraphSrc >
bool
test_copy(  t_TyGraphDst & _rgDst,
            t_TyGraphSrc const & _rgSrc,
            time_t * _ptStart )
{
	// A copy either completes or fails as a whole - unlike iteration it can't continue from where the exception was thrown.
	// With the normal throw rate a copy of any size would essentially never complete so we lower the rate
	//	to where about one in <gs_knCopyThrowPointsPerThrow> throw points throws.
#ifndef __NDEBUG_THROW
	const int kiThrowRateSave = _throw_object_base::ms_tsb.m_iThrowRate;
	_throw_object_base::ms_tsb.set_throw_rate( RAND_MAX / gs_knCopyThrowPointsPerThrow );
#endif //!__NDEBUG_THROW
	bool fCopied = false;
	try
	{
		_rgDst.replace_copy( _rgSrc );
		fCopied = true;
	}
	catch( exception & rexc )
	{
		handle_caught( "test_copy()", rexc );
	}
#ifndef __NDEBUG_THROW
	_throw_object_base::ms_tsb.set_throw_rate( kiThrowRateSave );
#endif //!__NDEBUG_THROW
	if ( !fCopied )
		return false;
	print_time( "test_copy(): After copy.\n", _ptStart );
	(void)test_compare( _rgDst, _rgSrc, _ptStart );
	(void)test_compare( _rgSrc, _rgDst, _ptStart );
	return true;
}

template < class t_TyGraph >
bool
try_dump( t_TyGraph const & _rg, ostream & _ros )
{
  try
  {
    typename t_TyGraph::_TyDumpIteratorConst
        dit(	_ros, _rg.get_root(), 0, false, true, _rg.get_base_path_allocator() );
    //dit.SetOutputOptions( e_doMapPointers );
    for ( ; !dit.FAtEnd(); )
    {
      try
      {
        ++dit;
      }
      catch( exception & rexc )
      {
				handle_caught( "try_dump(): in ++", rexc );
				if ( gs_nFailures )
					return true;
      }
    }
    return true;
  }
  catch( exception & rexc )
  {
		handle_caught( "try_dump()", rexc );
  }
	return false;
}

template < class t_TyGraph >
void
test_dump( t_TyGraph const & _rg, const char * _cpFileName, time_t * _ptStart )
{
	fstream fsDumpOut( _cpFileName, ios::out );
	if ( retry_until_success( "test_dump()", [&]() { return try_dump( _rg, fsDumpOut ); } ) )
		print_time( "test_dump(): Dump succeeded.\n", _ptStart );
#ifndef __NDEBUG_THROW
	print_hit_stats();
#endif //!__NDEBUG_THROW
}

// Save using the output iterator type <t_tyBinaryOutputIter> to <_out> ( an ostream or a file handle ).
// An exception thrown while moving to the next element leaves the iterator where it was - so we just try again.
template < class t_TyGraph, class t_tyBinaryOutputIter, class t_tyOut >
bool
try_save( t_TyGraph const & _rg, t_tyOut & _out )
{
	try
	{
		t_tyBinaryOutputIter boi( _out, _rg.begin() );
		while ( !boi.FAtEnd() )
		{
			try
			{
				++boi;
			}
			catch( exception & rexc )
			{
				handle_caught( "try_save(): in ++", rexc );
				if ( gs_nFailures )
					return true;
			}
		}
		return true;
	}
	catch( exception & rexc )
	{
		handle_caught( "try_save()", rexc );
	}
	return false;
}

template < class t_TyGraph, class t_tyBinaryInputIter, class t_tyIn >
bool
try_load( t_TyGraph & _rg, t_tyIn & _in )
{
	try
	{
		_rg.destroy();

		t_tyBinaryInputIter bii( _rg, _in, _rg.get_base_path_allocator() );
		do
		{
			try
			{
				++bii;
			}
			catch( exception & rexc )
			{
				handle_caught( "try_load(): in ++", rexc );
				if ( gs_nFailures )
					return true;
			}
		}
		while( !bii.FAtEnd() );

		_rg.set_root_node( bii.PGNTransferNewRoot() );
		return true;
	}
	catch( exception & rexc )
	{
		handle_caught( "try_load()", rexc );
	}
	return false;
}

template < class t_TyGraphSave, class t_TyGraphLoad >
bool
test_saveload_iostream(	t_TyGraphSave const & _rgSave, t_TyGraphLoad & _rgLoad,
												const char * _cpFileName, time_t * _ptStart )
{
	fstream fs( _cpFileName, ios::in | ios::out | ios::trunc | ios::binary );
	if ( !fs.is_open() )
	{
		cout << "test_saveload_iostream(): Unable to open file [" << _cpFileName << "].\n";
		report_failure( "test_saveload_iostream(): open" );
		return true;
	}
	fs.seekp( 0 );
	ostream & ros = fs;
	if ( !try_save< t_TyGraphSave, typename t_TyGraphSave::_TyBinaryOstreamIterConst >( _rgSave, ros ) )
		return false;
	print_time( "test_saveload_iostream(): Save succeeded.\n", _ptStart );
#ifndef __NDEBUG_THROW
	print_hit_stats();
	_throw_object_base::ms_tsb.reset_hit_once();
#endif //!__NDEBUG_THROW

	istream & ris = fs;
	if ( !retry_until_success( "test_saveload_iostream(): load", [&]()
				{
					fs.clear();
					fs.seekg( 0 );
					return try_load< t_TyGraphLoad, typename t_TyGraphLoad::_TyBinaryIstreamIterNonConst >( _rgLoad, ris );
				} ) )
		return true;
	print_time( "test_saveload_iostream(): After load.\n", _ptStart );
#ifndef __NDEBUG_THROW
	print_hit_stats();
#endif //!__NDEBUG_THROW
	(void)test_compare( _rgSave, _rgLoad, _ptStart );
	return true;
}

template < class t_TyGraphSave, class t_TyGraphLoad, class t_tyBinaryOutputIter, class t_tyBinaryInputIter >
bool
test_saveload_file(	t_TyGraphSave const & _rgSave, t_TyGraphLoad & _rgLoad,
										const char * _cpFileName, time_t * _ptStart )
{
	FileObj fo( CreateReadWriteFile( _cpFileName ) );
	if ( !fo.FIsOpen() )
	{
		cout << "test_saveload_file(): Unable to open file [" << _cpFileName << "].\n";
		report_failure( "test_saveload_file(): open" );
		return true;
	}
	vtyFileHandle hFile = fo.HFileGet();
	if ( !try_save< t_TyGraphSave, t_tyBinaryOutputIter >( _rgSave, hFile ) )
		return false;
	print_time( "test_saveload_file(): Save succeeded.\n", _ptStart );
#ifndef __NDEBUG_THROW
	print_hit_stats();
	_throw_object_base::ms_tsb.reset_hit_once();
#endif //!__NDEBUG_THROW

	if ( !retry_until_success( "test_saveload_file(): load", [&]()
				{
					(void)NFileSeekAndThrow( hFile, 0, vkSeekBegin );
					return try_load< t_TyGraphLoad, t_tyBinaryInputIter >( _rgLoad, hFile );
				} ) )
		return true;
	print_time( "test_saveload_file(): After load.\n", _ptStart );
#ifndef __NDEBUG_THROW
	print_hit_stats();
#endif //!__NDEBUG_THROW
	(void)test_compare( _rgSave, _rgLoad, _ptStart );
	return true;
}

int
_try_main( int iNumNodes, int iNumExtraLinks, int iRandSeed, int iThrowOneOnly, time_t & tStart )
{
  typedef _TyDefaultAllocator _TyAllocator;
	typedef dgraph< int, int, false, _TyAllocator > _TyGraph;
	_TyGraph g;
	g.replace_random( iNumNodes, iNumExtraLinks, iRandSeed );
	// CreateTestGraph0( g );

	print_time( "main(): Random graph created.\n", &tStart );

#ifndef __NDEBUG_THROW
	// Set the seed at this point:
	_throw_object_base::ms_tsb.set_seed( iRandSeed );
	_throw_object_base::ms_tsb.set_on( (uint32_t)(-1) & ~e_ttFatal ); // Turn on all exceptions except for fatal ones by default.
	if ( iThrowOneOnly > 0 )
	{
		_throw_object_base::ms_tsb.throw_one_only( iThrowOneOnly );
	}
#endif //!__NDEBUG_THROW

	_TyGraph gCopy;
	(void)retry_until_success( "test_copy()", [&]() { return test_copy( gCopy, g, &tStart ); } );
	print_counts( "After copy" );
  test_dump( gCopy, "graph.txt", &tStart );
	print_counts( "After dump" );

  typedef dgraph< int, int, true, _TyAllocator > _TyGraphSafe;
	_TyGraphSafe gsCopyLoaded;
#ifndef __NDEBUG_THROW
	_throw_object_base::ms_tsb.reset_hit_once();
#endif //!__NDEBUG_THROW
	(void)retry_until_success( "test_saveload_iostream()", [&]() { return test_saveload_iostream( gCopy, gsCopyLoaded, "graph.bin", &tStart ); } );
	print_counts( "After iostream save and load" );
	(void)retry_until_success( "test_saveload_file()", [&]()
		{
			return test_saveload_file< _TyGraph, _TyGraphSafe, _TyGraph::_TyBinaryFiledesOuputIterConst, _TyGraphSafe::_TyBinaryFiledesInputIterNonConst >( gCopy, gsCopyLoaded, "graph_file.bin", &tStart );
		} );
	print_counts( "After file save and load" );
#ifndef __GR_TEST_NOMEMMAPPED
	(void)retry_until_success( "test_saveload_file(): mapped", [&]()
		{
			return test_saveload_file< _TyGraph, _TyGraphSafe, _TyGraph::_TyBinaryMemMappedOuputIterConst, _TyGraphSafe::_TyBinaryMemMappedInputIterNonConst >( gCopy, gsCopyLoaded, "graph_mapped.bin", &tStart );
		} );
	print_counts( "After mapped file save and load" );
#endif //!__GR_TEST_NOMEMMAPPED

	// Copy the graph to a graph of doubles:
	typedef dgraph< double, double, true, _TyAllocator > _TyGraphDoubleSafe;
	_TyGraphDoubleSafe gdCopy;
	(void)retry_until_success( "test_copy(): double", [&]() { return test_copy( gdCopy, g, &tStart ); } );
	print_counts( "After copy to double" );
  test_dump( gdCopy, "dgraph.txt", &tStart );
  typedef dgraph< double, double, false, _TyAllocator > _TyGraphDouble;
	_TyGraphDouble gdCopyLoaded;
	(void)retry_until_success( "test_saveload_iostream(): double", [&]() { return test_saveload_iostream( gdCopy, gdCopyLoaded, "dgraph.bin", &tStart ); } );
	print_counts( "After iostream save and load of double" );

#ifndef __NDEBUG_THROW
	// Don't throw while destroying the graphs.
	_throw_object_base::ms_tsb.set_on( 0 );
#endif //!__NDEBUG_THROW
	return 0;
}

int
main( int argc, char ** argv )
{
#ifdef WIN32
#ifdef _DEBUG
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
#endif
#endif //WIN32
	g_strProgramName = argv[0];
	n_SysLog::InitSysLog( argv[0], LOG_PERROR, LOG_USER );

	// Extract the random graph parameters:
	if ( argc < 4 )
	{
		usage( argv[0] );
		return -1;
	}
	int	iNumNodes = atoi( argv[1] );
	if ( iNumNodes <= 0 )
	{
		usage( argv[0] );
		cerr << "Need positive number of nodes.\n";
		return -1;
	}
	int iNumExtraLinks = atoi( argv[2] );
	if ( iNumExtraLinks < 0 )
	{
		usage( argv[0] );
		cerr << "Need non-negative number of extra links.\n";
		return -1;
	}
	int iRandSeed = atoi( argv[3] );

	int iThrowOneOnly = -1;
	if ( argc > 4 )
	{
		iThrowOneOnly = atoi( argv[4] );
	}

	time_t	tStart;
	time( &tStart );

	try
	{
		(void)_try_main( iNumNodes, iNumExtraLinks, iRandSeed, iThrowOneOnly, tStart );
	}
	catch( exception & rexc )
	{
		cout << "main(): Caught exception [" << rexc.what() << "].\n";
		report_failure( "main(): exception" );
	}
#ifndef __NDEBUG_THROW
	_throw_object_base::ms_tsb.set_on( 0 );
#endif //!__NDEBUG_THROW

	print_time( "main(): At exit.\n", &tStart );

#ifdef __DGRAPH_COUNT_EL_ALLOC_LIFETIME
	cout << "Nodes allocated [" << gs_iNodesAllocated << "].\n";
	cout << "Links allocated [" << gs_iLinksAllocated << "].\n";
	cout << "Nodes constructed [" << gs_iNodesConstructed << "].\n";
	cout << "Links constructed [" << gs_iLinksConstructed << "].\n";
	if ( gs_iNodesAllocated || gs_iLinksAllocated || gs_iNodesConstructed || gs_iLinksConstructed )
		report_failure( "main(): Graph elements were leaked." );
#endif //__DGRAPH_COUNT_EL_ALLOC_LIFETIME

#ifndef __NDEBUG_THROW
	cout << "Num throws [" << _throw_object_base::ms_tsb.m_uNumThrows << "].\n";
	unsigned uHit;
	size_t uPossible;
	_throw_object_base::ms_tsb.get_hit_stats( uHit, uPossible );
	cout << "Hit [" << uHit << "] out of [" << uPossible << "].\n";
	_throw_object_base::ms_tsb.report_unhit( cout );
#endif //!__NDEBUG_THROW

	cout << ( gs_nFailures ? "FAILED" : "SUCCEEDED" ) << ": [" << gs_nFailures << "] failures.\n";
	return gs_nFailures ? 1 : 0;
}
