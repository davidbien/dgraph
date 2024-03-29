#ifndef __GR_SHWR_H
#define __GR_SHWR_H

//          Copyright David Lawrence Bien 1997 - 2021.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt).

// _gr_shwr.h

// This module implements wrappers for shadow graph elements.

__DGRAPH_BEGIN_NAMESPACE

template <  class t_TyLinkEl, class t_TyNodeEl, class t_TyGraphLinkBase,
            class t_TyShadowNode, class t_TyShadowLink,
            class t_TyShadowEl = _sgraph_element< t_TyShadowLink > >
class _sgraph_link : public t_TyGraphLinkBase
{
  typedef t_TyGraphLinkBase                                             _TyBase;
  typedef _sgraph_link< t_TyLinkEl, t_TyNodeEl, t_TyGraphLinkBase,
                        t_TyShadowNode, t_TyShadowLink, t_TyShadowEl >  _TyThis;
public:

  typedef t_TyLinkEl                                            _TyLinkEl;
  typedef t_TyNodeEl                                            _TyNodeEl;
  typedef t_TyGraphLinkBase                                     _TyGraphLinkBase;
  typedef typename t_TyGraphLinkBase::_TyGraphLinkBase          _TyGraphLinkBaseBase;
  typedef typename _TyGraphLinkBase::_TyGraphNodeBase           _TyGraphNodeBase;
  typedef typename _TyGraphNodeBase::_TyGraphNodeBase           _TyGraphNodeBaseBase;
  typedef _sgraph_node< _TyNodeEl, _TyLinkEl, _TyGraphNodeBase,
                        t_TyShadowNode, t_TyShadowLink,
                        _sgraph_element< t_TyShadowNode > >     _TyGraphNode;

  typedef t_TyShadowEl                                          _TyShadowEl;

public:

  _sgraph_link( )
  {
  } 

  // This accesses the constructable object within the _graph_link.
  _TyShadowEl & RElObject() noexcept(true)
  {
    return m_tElShadow;
  }

  // Element access:
  _TyLinkEl & REl() noexcept(true)
  {
    return m_tElShadow.REl();
  }
  const _TyLinkEl & REl() const noexcept(true)
  {
    return m_tElShadow.REl();
  }
  _TyLinkEl & RElNonConst() noexcept(true)
  {
    return m_tElShadow.RElNonConst();
  }
  const _TyLinkEl & RElConst() const noexcept(true)
  {
    return m_tElShadow.RElConst();
  }

  operator _TyLinkEl & () noexcept(true) 
  {
    return RElNonConst();
  }
  operator const _TyLinkEl & () const noexcept(true) 
  { 
    return RElConst(); 
  }

  // typed child/parent node access:
  _TyGraphNode * PGNChild()
  {
    return static_cast< _TyGraphNode* >( PGNBChild() );
  }
  _TyGraphNode * PGNParent()
  {
    return static_cast< _TyGraphNode* >( PGNBParent() );
  }
  
  _TyThis ** PPGLGetNextChild()
  {
    return (_TyThis**)PPGLBGetNextChild();
  }
  _TyThis ** PPGLGetNextParent()
  {
    return (_TyThis**)PPGLBGetNextParent();
  }
  _TyThis ** PPGLGetThisChild()
  {
    return (_TyThis**)PPGLBGetThisChild();
  }
  _TyThis ** PPGLGetThisParent()
  {
    return (_TyThis**)PPGLBGetThisParent();
  }

  void InsertParent( _TyThis ** _ppgl )
  {
    _TyGraphLinkBase::InsertParent( (_TyGraphLinkBase**)_ppgl );
  }
  void InsertChild( _TyThis ** _ppgl )
  {
    _TyGraphLinkBase::InsertChild( (_TyGraphLinkBase**)_ppgl );
  }

protected:
  _TyShadowEl m_tElShadow;
};

template <  class t_TyNodeEl, class t_TyLinkEl, class t_TyGraphNodeBase,
            class t_TyShadowNode, class t_TyShadowLink,
            class t_TyShadowEl = _sgraph_element< t_TyShadowNode > >
class _sgraph_node : public t_TyGraphNodeBase
{
  typedef t_TyGraphNodeBase                                             _TyBase;
  typedef _sgraph_node< t_TyNodeEl, t_TyLinkEl, t_TyGraphNodeBase,
                        t_TyShadowNode, t_TyShadowLink, t_TyShadowEl >  _TyThis;
public:

  typedef t_TyNodeEl                                            _TyNodeEl;
  typedef t_TyLinkEl                                            _TyLinkEl;
  typedef t_TyGraphNodeBase                                     _TyGraphNodeBase;
  typedef typename t_TyGraphNodeBase::_TyGraphNodeBase          _TyGraphNodeBaseBase;
  typedef typename _TyGraphNodeBase::_TyGraphLinkBase           _TyGraphLinkBase;
  typedef typename _TyGraphLinkBase::_TyGraphLinkBase           _TyGraphLinkBaseBase;
  typedef _sgraph_link< _TyLinkEl, _TyNodeEl, _TyGraphLinkBase,
                        t_TyShadowNode, t_TyShadowLink,
                        _sgraph_element< t_TyShadowLink >  >    _TyGraphLink;

  typedef t_TyShadowEl                                          _TyShadowEl;

public:

  _sgraph_node()
  {
  }

  _TyShadowEl & RElObject() noexcept(true)
  {
    return m_tElShadow;
  }

  // Element access:
  _TyNodeEl & REl() noexcept(true)
  {
    return m_tElShadow.REl();
  }
  const _TyNodeEl & REl() const noexcept(true)
  {
    return m_tElShadow.REl();
  }
  _TyNodeEl & RElNonConst() noexcept(true)
  {
    return m_tElShadow.RElNonConst();
  }
  const _TyNodeEl & RElConst() const noexcept(true)
  {
    return m_tElShadow.RElConst();
  }

  operator _TyNodeEl & () noexcept(true) 
  { 
    return RElNonConst(); 
  }
  operator const _TyNodeEl & () const noexcept(true) 
  { 
    return RElConst(); 
  }

  // child/parent typed link list access:
  _TyGraphLink **           PPGLChildHead() noexcept(true)
  {
    return (_TyGraphLink**)PPGLBChildHead();
  }
  _TyGraphLink * const *    PPGLChildHead() const noexcept(true)
  {
    return (_TyGraphLink* const *)PPGLBChildHead();
  }

  _TyGraphLink **     PPGLParentHead() noexcept(true)
  {
    return (_TyGraphLink**)PPGLBParentHead();
  }
  _TyGraphLink * const *  PPGLParentHead() const noexcept(true)
  {
    return (_TyGraphLink* const *)PPGLBParentHead();
  }

  _TyGraphLink **     PPGLRelationHead( bool _fChild ) noexcept(true)
  {
    return (_TyGraphLink**)PPGLBRelationHead( _fChild );
  }
  _TyGraphLink * const *      PPGLRelationHead( bool _fChild ) const noexcept(true)
  {
    return (_TyGraphLink* const *)PPGLBRelationHead( _fChild );  
  }

protected:
  _TyShadowEl m_tElShadow;
};

__DGRAPH_END_NAMESPACE

#endif //__GR_SHWR_H
