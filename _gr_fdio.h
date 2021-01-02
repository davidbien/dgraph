#pragma once

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _gr_fdio.h
// graph I/O through files - originally through file descriptors then ported to Windows files.
// dbien: 21APR2020

#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif //!WIN32
#include "_compat.h"
#include "_gr_inc.h"

__DGRAPH_BEGIN_NAMESPACE

// Specialize for fdout - default version just writes raw memory:
template < class t_TyWrite >
__INLINE void
_RawWriteGraphEl( vtyFileHandle _hFile, t_TyWrite const & _rEl )
{	
	if ( sizeof( _rEl ) )
	{
    size_t stWritten;
    int iWriteResult = FileWrite(_hFile, &_rEl, sizeof( _rEl ), &stWritten );
    __THROWPT( e_ttFileOutput );
    if ( !!iWriteResult || ( stWritten != sizeof( _rEl ) ) )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), ( stWritten != sizeof( _rEl ) ) ? "Didn't write all the data? WTF?" : "FileWrite() failed." );
	}
}

// Specialize for fdin - default version just reads raw memory:
template < class t_TyRead >
__INLINE void
_RawReadGraphEl( vtyFileHandle _hFile, t_TyRead & _rEl )
{	
	if ( sizeof( _rEl ) )
	{
    size_t stRead;
    int iReadResult = FileRead(_hFile, &_rEl, sizeof( _rEl ), &stRead );
    __THROWPT( e_ttFileInput );
    if ( !!iReadResult || ( stRead != sizeof( _rEl ) ) )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), ( stRead != sizeof( _rEl ) ) ? "EOF before end of value." : "FileRead() failed." );
	}
}

struct _file_RawElIO
{
  template < class t_TyEl >
  void Write( vtyFileHandle _hFile, t_TyEl const & _rel )
  {
    _RawWriteGraphEl( _hFile, _rel );
  }
  template < class t_TyEl >
  void Read( vtyFileHandle _hFile, t_TyEl & _rel )
  {
    _RawReadGraphEl( _hFile, _rel );
  }
};

template <  class t_TyOutputNodeEl,
            class t_TyOutputLinkEl = t_TyOutputNodeEl >
struct _file_out_object
{
  typedef vtyFileHandle _TyInitArg;
  typedef off_t _TyStreamPos;
  typedef t_TyOutputNodeEl _TyIONodeEl;
  typedef t_TyOutputLinkEl _TyIOLinkEl;

  vtyFileHandle m_hFile{vkhInvalidFileHandle}; // This object doesn't own the lifetime of the open file.
  t_TyOutputNodeEl  m_one;
  t_TyOutputLinkEl  m_ole;

  _file_out_object( _file_out_object const & ) = delete;
  _file_out_object() = delete;
	_file_out_object(  vtyFileHandle _hFile,
                  t_TyOutputNodeEl const & _rone,
                  t_TyOutputLinkEl const & _role )
		: m_hFile( _hFile ),
      m_one( _rone ),
      m_ole( _role )
	{
      __THROWPT( e_ttMemory ); // in the cases where where are dynamic members within m_one or m_ole.
	}
	_file_out_object(  vtyFileHandle _hFile,
                  t_TyOutputNodeEl && _rrone,
                  t_TyOutputLinkEl && _rrole )
		: m_hFile( _hFile ),
      m_one( std::move( _rrone ) ),
      m_ole( std::move( _rrole ) )
	{
	}
	_TyStreamPos TellP() const
  {
    __THROWPT( e_ttFileOutput );
    return NFileSeekAndThrow( m_hFile, 0, vkSeekCur );
  }
	void SeekP( _TyStreamPos _sp )	
  {
    int iSeekResult = FileSeek( m_hFile, _sp, vkSeekBegin );
    __THROWPT( e_ttFileOutput );
    if ( !!iSeekResult )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "FileSeek() failed." );
  }
	void Write( const void * _pv, size_t _st )
	{
    if ( _st )
    {
      size_t stWritten;
      int iWrite = FileWrite( m_hFile, _pv, _st, &stWritten );
      __THROWPT( e_ttFileOutput );
      if ( !!iWrite || ( stWritten != _st ) )
        THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), ( stWritten != _st ) ? "Didn't write all the data? WTF?" : "FileWrite() failed." );
    }
	}

	template < class t_TyEl >
	void WriteNodeEl( t_TyEl const & _rel )
	{
    m_one.Write( m_hFile, _rel );
	}
	template < class t_TyEl >
	void WriteLinkEl( t_TyEl const & _rel )
	{
    m_ole.Write( m_hFile, _rel );
	}
};

template <  class t_TyInputNodeEl,
            class t_TyInputLinkEl = t_TyInputNodeEl >
struct _file_in_object
{
	typedef vtyFileHandle _TyInitArg;
	typedef off_t _TyStreamPos;
	typedef t_TyInputNodeEl _TyIONodeEl;
	typedef t_TyInputLinkEl _TyIOLinkEl;

	vtyFileHandle m_hFile{vkhInvalidFileHandle}; // This object doesn't own the lifetime of the open file.
  t_TyInputNodeEl m_ine;
  t_TyInputLinkEl m_ile;

	_file_in_object(  vtyFileHandle _hFile,
                    t_TyInputNodeEl const & _rine,
                    t_TyInputLinkEl const & _rile )
		: m_hFile( _hFile ),
      m_ine( _rine ),
      m_ile( _rile )
	{
      __THROWPT( e_ttMemory ); // in the cases where where are dynamic members within m_ine or m_ile.
	}
	_file_in_object(  vtyFileHandle _hFile,
                t_TyInputNodeEl && _rrine,
                t_TyInputLinkEl && _rrile )
		: m_hFile( _hFile ),
      m_ine( std::move( _rrine ) ),
      m_ile( std::move( _rrile ) )
	{
	}
	_TyStreamPos TellG() const
	{ 
    __THROWPT( e_ttFileInput | e_ttFatal );
    return NFileSeekAndThrow( m_hFile, 0, vkSeekCur );
  }
	void SeekG( _TyStreamPos _sp )	
  {
    int iSeekResult = FileSeek( m_hFile, _sp, vkSeekBegin );
    __THROWPT( e_ttFileInput | e_ttFatal );
    if ( !!iSeekResult )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "FileSeek() failed." );
	}
	void Read( void * _pv, size_t _st )
	{
    size_t stRead;
    int iRead = FileRead(m_hFile, _pv, _st, &stRead);
    __THROWPT( e_ttFileInput );
    if ( !!iRead || (stRead != _st ) )
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), ( stRead != _st ) ? "EOF before all data read." : "FileRead() failed.");
	}
	template < class t_TyEl >
	void ReadNodeEl( t_TyEl & _rel )
	{
		m_ine.Read( m_hFile, _rel );
	}
	template < class t_TyEl >
	void ReadLinkEl( t_TyEl & _rel )
	{
    	m_ile.Read( m_hFile, _rel );
	}
};

__DGRAPH_END_NAMESPACE
