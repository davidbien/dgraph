#pragma once

//          Copyright David Lawrence Bien 1997 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _gr_mmio.h
// graph I/O through mapped memory.
// dbien: 21APR2020

#include <fcntl.h>
#include "_compat.h"
#ifndef WIN32
#include <unistd.h>
#include <sys/mman.h>
#endif //!WIN32
#include "_gr_inc.h"
#include "_fdobjs.h"

__DGRAPH_BEGIN_NAMESPACE

// Specialize for mmout - default version just writes raw memory:
template < class t_TyWrite >
__INLINE size_t
_StRawWriteGraphEl( void * _pvWrite, ssize_t _sstLeft, t_TyWrite const & _rEl )
{
	if ( sizeof( _rEl ) )
	{
    if ( sizeof( _rEl ) <= _sstLeft )
      memcpy( _pvWrite, &_rEl, sizeof( _rEl ) );
    return sizeof( _rEl );
  }
}

// Specialize for mmin - default version just reads raw memory:
template < class t_TyRead >
__INLINE size_t
_StRawReadGraphEl( const void * _pvRead, ssize_t _sstLeft, t_TyRead & _rEl )
{
	if ( sizeof( _rEl ) )
	{
    __THROWPT( e_ttFileInput );
    if ( _sstLeft < sizeof( _rEl ) )
      THROWNAMEDEXCEPTION( "EOF reading element." );
    memcpy( &_rEl, _pvRead, sizeof( _rEl ) );
    return sizeof( _rEl );
	}
}

struct _mm_RawElIO
{
  template < class t_TyEl >
  size_t StWrite( void * _pvWrite, ssize_t _sstLeft, t_TyEl const & _rel )
  {
    return _StRawWriteGraphEl( _pvWrite, _sstLeft, _rel );
  }
  template < class t_TyEl >
  size_t StRead( const void * _pvRead, ssize_t _sstLeft, t_TyEl & _rel )
  {
    return _StRawReadGraphEl( _pvRead, _sstLeft, _rel );
  }
};

template <  class t_TyOutputNodeEl,
            class t_TyOutputLinkEl = t_TyOutputNodeEl,
            size_t t_knGrowFileByBytes = 65536 >
struct _mmout_object
{
  typedef int _TyInitArg;
  typedef vtySeekOffset _TyStreamPos;
  typedef t_TyOutputNodeEl _TyIONodeEl;
  typedef t_TyOutputLinkEl _TyIOLinkEl;
  static const size_t s_knGrowFileByBytes = t_knGrowFileByBytes;

  vtyFileHandle m_hFile{vkhInvalidFileHandle}; // This object doesn't own the lifetime of the open file.
  FileMappingObj m_fmoFile; // We do own the lifetime of our mapping however.
  //uint8_t * m_pbyMappedBegin{(uint8_t*)vkpvNullMapping};
  uint8_t * m_pbyMappedCur{(uint8_t*)vkpvNullMapping};
  uint8_t * m_pbyMappedEnd{(uint8_t*)vkpvNullMapping};

  t_TyOutputNodeEl  m_one;
  t_TyOutputLinkEl  m_ole;

  _mmout_object( _mmout_object const & ) = delete;
  _mmout_object() = delete;
	_mmout_object(  int _hFile,
                  t_TyOutputNodeEl const & _rone,
                  t_TyOutputLinkEl const & _role )
		: m_hFile( _hFile ),
      m_one( _rone ),
      m_ole( _role )
	{
    _OpenMap();
	}
	_mmout_object(  int _hFile,
                  t_TyOutputNodeEl && _rrone,
                  t_TyOutputLinkEl && _rrole )
		: m_hFile( _hFile ),
      m_one( std::move( _rrone ) ),
      m_ole( std::move( _rrole ) )
	{
    _OpenMap();
	}
  ~_mmout_object() noexcept(false)
  {
    Assert( m_fmoFile.FIsOpen() && ( vkhInvalidFileHandle != m_hFile ) );
    if ( !m_fmoFile.FIsOpen() || ( vkhInvalidFileHandle == m_hFile ) )
      return;
    bool fInUnwinding = !!std::uncaught_exceptions();
    void * pvMappedSave = m_fmoFile.Pv();
    int iCloseFileMapping = m_fmoFile.Close();
    vtyErrNo errCloseFileMapping = !iCloseFileMapping ? vkerrNullErrNo : GetLastErrNo();
    vtyErrNo errTruncate = vkerrNullErrNo;
    if ( vkhInvalidFileHandle != m_hFile )
    {
      // We need to truncate the file to m_cpxMappedCur - m_pvMapped bytes.
      size_t stSizeTruncate = m_pbyMappedCur - (uint8_t*)pvMappedSave;
      int iTruncate = FileSetSize(m_hFile, stSizeTruncate);
      errTruncate = !iTruncate ? vkerrNullErrNo : GetLastErrNo();
    }
    vtyErrNo errFirst;
    unsigned nError;
    if ( ( (nError = 1), ( vkerrNullErrNo != ( errFirst = errTruncate ) ) ) || 
         ( (nError = 2), ( vkerrNullErrNo != ( errFirst = errCloseFileMapping ) ) ) )
    {
      // Ensure that the errno is represented in the last error:
      SetLastErrNo( errFirst );
      if ( !fInUnwinding )
      {
        const char * pcThrow;
        switch( nError )
        {
          case 1: pcThrow = "Error encountered truncating m_hFile[0x%lx]"; break;
          case 2: pcThrow = "Error encountered closing file mapping m_hFile[0x%lx]"; break;
          default: pcThrow = "Wh-what?! m_hFile[0x%lx]"; break;
        }
        THROWNAMEDEXCEPTIONERRNO( errFirst, pcThrow, (uint64_t)m_hFile );
      }
    }
    if ( !fInUnwinding )
      __THROWPT_DTOR( e_ttFileOutput | e_ttFatal, fInUnwinding );
  }
	_TyStreamPos TellP() const
  {
    return m_pbyMappedCur - (uint8_t*)m_fmoFile.Pv();
  }
	void SeekP( _TyStreamPos _sp )	
  {
    // We will let the caller set the position anywhere at all.
    m_pbyMappedCur = (uint8_t*)m_fmoFile.Pv() + _sp;
  }
	void Write( const void * _pv, size_t _st )
	{
    if ( ssize_t( _st ) > ( m_pbyMappedEnd - m_pbyMappedCur ) )
      _GrowMap( ssize_t(_st) - ( m_pbyMappedEnd - m_pbyMappedCur ) );
    memcpy( m_pbyMappedCur, _pv, _st );
    m_pbyMappedCur += _st;
	}
	template < class t_TyEl >
	void WriteNodeEl( t_TyEl const & _rel )
	{
    ssize_t sstLeft = ( m_pbyMappedEnd - m_pbyMappedCur );
    size_t stNeed = m_one.StWrite( m_pbyMappedCur, sstLeft, _rel );
    if ( stNeed > sstLeft )
    {
      _GrowMap( stNeed - sstLeft );
      size_t stNeed2 = m_one.StWrite( m_pbyMappedCur, ( m_pbyMappedEnd - m_pbyMappedCur ), _rel );
      Assert( stNeed == stNeed2 );
    }
    m_pbyMappedCur += stNeed;
	}
	template < class t_TyEl >
	void WriteLinkEl( t_TyEl const & _rel )
	{
    ssize_t sstLeft = ( m_pbyMappedEnd - m_pbyMappedCur );
    size_t stNeed = m_ole.StWrite( m_pbyMappedCur, sstLeft, _rel );
    if ( stNeed > sstLeft )
    {
      _GrowMap( stNeed - sstLeft );
      size_t stNeed2 = m_ole.StWrite( m_pbyMappedCur, ( m_pbyMappedEnd - m_pbyMappedCur ), _rel );
      Assert( stNeed == stNeed2 );
    }
    m_pbyMappedCur += stNeed;
	}
protected:
  void _OpenMap()
  {
    int iResult = FileSetSize( m_hFile, s_knGrowFileByBytes ); // Set initial size.
    __THROWPT( e_ttFileOutput | e_ttFatal );
    if ( !!iResult )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "FileSetSize() m_hFile[0x%lx]", (uint64_t)m_hFile);
    m_fmoFile.SetHMMFile( MapReadWriteHandle( m_hFile ) );
    __THROWPT( e_ttFileOutput | e_ttFatal );
    if ( !m_fmoFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "Mapping failed m_hFile[0x%lx]", (uint64_t)m_hFile);
    m_pbyMappedCur = (uint8_t*)m_fmoFile.Pv();
    m_pbyMappedEnd = m_pbyMappedCur + s_knGrowFileByBytes;
  }
  void _GrowMap( size_t _stByAtLeast )
  {
    VerifyThrow( m_fmoFile.FIsOpen() && ( vkhInvalidFileHandle != m_hFile ) );
    size_t stGrowBy = ( ( ( _stByAtLeast - 1 ) / s_knGrowFileByBytes ) + 1 ) * s_knGrowFileByBytes;
    size_t stMapped = m_pbyMappedEnd - (uint8_t*)m_fmoFile.Pv();
    size_t stCurOffset = m_pbyMappedCur - (uint8_t*)m_fmoFile.Pv();
    m_pbyMappedCur = (uint8_t*)vkpvNullMapping;
    m_pbyMappedEnd = (uint8_t*)vkpvNullMapping;
    (void)m_fmoFile.Close();
    int iFileSetSize = FileSetSize(m_hFile, stMapped + stGrowBy);
    __THROWPT( e_ttFileOutput | e_ttFatal );
    if (-1 == iFileSetSize)
      THROWNAMEDEXCEPTIONERRNO( GetLastErrNo(), "FileSetSize() failed for m_hFile[0x%lx].", (uint64_t)m_hFile );
    m_fmoFile.SetHMMFile( MapReadWriteHandle( m_hFile ) );
    __THROWPT( e_ttFileOutput | e_ttFatal );
    if ( !m_fmoFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "Remapping the failed for m_hFile[0x%lx].", (uint64_t)m_hFile );
    m_pbyMappedEnd = (uint8_t*)m_fmoFile.Pv() + stMapped;
    m_pbyMappedCur = (uint8_t*)m_fmoFile.Pv() + stCurOffset;
  }
};

template <  class t_TyInputNodeEl,
            class t_TyInputLinkEl = t_TyInputNodeEl >
struct _mmin_object
{
	typedef int _TyInitArg;
	typedef vtySeekOffset _TyStreamPos;
	typedef t_TyInputNodeEl _TyIONodeEl;
	typedef t_TyInputLinkEl _TyIOLinkEl;

	vtyFileHandle m_hFile{vkhInvalidFileHandle}; // This object doesn't own the lifetime of the open file.
  FileMappingObj m_fmoFile; // We own the mapping.
  const uint8_t * m_pbyMappedCur{(uint8_t*)vkpvNullMapping};
  const uint8_t * m_pbyMappedEnd{(uint8_t*)vkpvNullMapping};

  t_TyInputNodeEl m_ine;
  t_TyInputLinkEl m_ile;

  _mmin_object( _mmin_object const & ) = delete;
  _mmin_object() = delete;
	_mmin_object(  int _hFile,
                t_TyInputNodeEl const & _rine,
                t_TyInputLinkEl const & _rile )
		: m_hFile( _hFile ),
      m_ine( _rine ),
      m_ile( _rile )
	{
    _OpenMap();
	}
	_mmin_object(  int _hFile,
                t_TyInputNodeEl && _rrine,
                t_TyInputLinkEl && _rrile )
		: m_hFile( _hFile ),
      m_ine( std::move( _rrine ) ),
      m_ile( std::move( _rrile ) )
	{
    _OpenMap();
	}
	_TyStreamPos TellG() const
	{
    return m_pbyMappedCur - (uint8_t*)m_fmoFile.Pv();
  }
	void SeekG( _TyStreamPos _sp )	
  {
    // We allow seeking beyond the end - but Read will throw if we try to read there.
    m_pbyMappedCur = (uint8_t*)m_fmoFile.Pv() + _sp;
	}
	void Read( void * _pv, size_t _st )
	{
    __THROWPT( e_ttFileInput ); // should be able to recover from this.
    if ( ssize_t(_st) > ( m_pbyMappedEnd - m_pbyMappedCur ) )
      THROWNAMEDEXCEPTION( "EOF.");
    memcpy( _pv, m_pbyMappedCur, _st );
    m_pbyMappedCur += _st;
	}
	template < class t_TyEl >
	void ReadNodeEl( t_TyEl & _rel )
	{
    ssize_t sstLeft = m_pbyMappedEnd - m_pbyMappedCur;
		size_t stRead = m_ine.StRead( m_pbyMappedCur, sstLeft, _rel ); // throws on EOF.
    m_pbyMappedCur += stRead;
	}
	template < class t_TyEl >
	void ReadLinkEl( t_TyEl & _rel )
	{
    ssize_t sstLeft = m_pbyMappedEnd - m_pbyMappedCur;
		size_t stRead = m_ile.StRead( m_pbyMappedCur, sstLeft, _rel ); // throws on EOF.
    m_pbyMappedCur += stRead;
	}
protected:
  void _OpenMap()
  {
    VerifyThrow( vkhInvalidFileHandle != m_hFile );
    // Now get the size of the file and then map it.
    vtyHandleAttr attrFile;
    int iResult = GetHandleAttrs( m_hFile, attrFile );
    if (-1 == iResult)
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "GetHandleAttrs() failed for m_hFile[0x%lx].", (uint64_t)m_hFile);
    size_t stSize = GetSize_HandleAttr( attrFile );
    if (0 == stSize )
      THROWNAMEDEXCEPTION("Can't map an empty m_hFile[0x%lx].", (uint64_t)m_hFile);
    __THROWPT( e_ttFileInput | e_ttFatal );
    if ( !FIsRegularFile_HandleAttr( attrFile ) )
      THROWNAMEDEXCEPTION("m_hFile[0x%lx] is not a regular file.", (uint64_t)m_hFile);
    m_fmoFile.SetHMMFile( MapReadOnlyHandle( m_hFile, nullptr ) );
    __THROWPT( e_ttFileInput | e_ttFatal );
    if ( !m_fmoFile.FIsOpen() )
      THROWNAMEDEXCEPTIONERRNO(GetLastErrNo(), "MapReadOnlyHandle() failed to map  m_hFile[0x%lx], size [%ld].", (uint64_t)m_hFile, stSize);
    m_pbyMappedCur = (const uint8_t*)m_fmoFile.Pv();
    m_pbyMappedEnd = m_pbyMappedCur + stSize;
  }
};

__DGRAPH_END_NAMESPACE

