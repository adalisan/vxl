// This is gel/vsol/vsol_group_3d.cxx
#include "vsol_group_3d.h"
//:
// \file

//*****************************************************************************
// External declarations for implementation
//*****************************************************************************
#include <vcl_cassert.h>

//***************************************************************************
// Initialization
//***************************************************************************

//---------------------------------------------------------------------------
//: Default Constructor: with no child
//---------------------------------------------------------------------------
vsol_group_3d::vsol_group_3d(void)
{
  storage_=new vcl_list<vsol_spatial_object_3d_sptr>();
}

//---------------------------------------------------------------------------
//: Copy constructor.
// Description: The objects of the group are not duplicated
//---------------------------------------------------------------------------
vsol_group_3d::vsol_group_3d(const vsol_group_3d &other)
{
  storage_=new vcl_list<vsol_spatial_object_3d_sptr>(*other.storage_);
}

//---------------------------------------------------------------------------
//: Destructor
// Description: The objects of the group are not deleted
//---------------------------------------------------------------------------
vsol_group_3d::~vsol_group_3d()
{
  delete storage_;
}

//---------------------------------------------------------------------------
//: Clone `this': creation of a new object and initialization
// See Prototype pattern
//---------------------------------------------------------------------------
vsol_spatial_object_3d_sptr vsol_group_3d::clone(void) const
{
  return new vsol_group_3d(*this);
}

//***************************************************************************
// Access
//***************************************************************************

//---------------------------------------------------------------------------
//: Return the object `i'
// Require: i>=0 and i<size()
//---------------------------------------------------------------------------
vsol_spatial_object_3d_sptr vsol_group_3d::object(const int i) const
{
  // require
  assert((i>=0)&&(i<size()));

  vcl_list<vsol_spatial_object_3d_sptr>::iterator j;
  int k;

  j=storage_->begin();
  for (k=0;k<i;++k)
    ++j;
  return *j;
}

//***************************************************************************
// Status report
//***************************************************************************

//---------------------------------------------------------------------------
//: Return the real type of a group. It is a SPATIALGROUP
//---------------------------------------------------------------------------
vsol_spatial_object_3d::vsol_spatial_object_3d_type
vsol_group_3d::spatial_type(void) const
{
  return vsol_spatial_object_3d::SPATIALGROUP;
}

//---------------------------------------------------------------------------
//: Compute the bounding box of `this'
// Require: size()>0
//---------------------------------------------------------------------------
void vsol_group_3d::compute_bounding_box(void)
{
  // require
  assert(size()>0);

  vcl_list<vsol_spatial_object_3d_sptr>::iterator i = storage_->begin();
  double xmin = (*i)->get_min_x();
  double ymin = (*i)->get_min_y();
  double zmin = (*i)->get_min_z();
  double xmax = (*i)->get_max_x();
  double ymax = (*i)->get_max_y();
  double zmax = (*i)->get_max_z();
  for (++i; i!=storage_->end(); ++i)
  {
    if ((*i)->get_min_x()<xmin) xmin=(*i)->get_min_x();
    if ((*i)->get_min_y()<ymin) ymin=(*i)->get_min_y();
    if ((*i)->get_min_z()<zmin) zmin=(*i)->get_min_z();
    if ((*i)->get_max_x()>xmax) xmax=(*i)->get_max_x();
    if ((*i)->get_max_y()>ymax) ymax=(*i)->get_max_y();
    if ((*i)->get_max_z()>zmax) zmax=(*i)->get_max_z();
  }
  if (!bounding_box_)
    bounding_box_=new vsol_box_3d;
  bounding_box_->set_min_x(xmin);
  bounding_box_->set_max_x(xmax);
  bounding_box_->set_min_y(ymin);
  bounding_box_->set_max_y(ymax);
  bounding_box_->set_min_z(zmin);
  bounding_box_->set_max_z(zmax);
}

//---------------------------------------------------------------------------
//: Return the number of direct children of the group
//---------------------------------------------------------------------------
int vsol_group_3d::size(void) const
{
  return storage_->size();
}

//---------------------------------------------------------------------------
//: Return the number of objects of the group
//---------------------------------------------------------------------------
int vsol_group_3d::deep_size(void) const
{
  int result=0;
  vcl_list<vsol_spatial_object_3d_sptr>::iterator i;
  for (i=storage_->begin(); i!=storage_->end(); ++i)
    {
      vsol_group_3d const* g=(*i)->cast_to_group();
      if (g!=0)
        result+=g->deep_size();
      else
        ++result;
    }
  return result;
}

//***************************************************************************
// Element change
//***************************************************************************

//---------------------------------------------------------------------------
//: Add an object `new_object'to `this'
// Require: !is_child(new_object)
//---------------------------------------------------------------------------
void vsol_group_3d::add_object(const vsol_spatial_object_3d_sptr &new_object)
{
  // require
  assert(!is_child(new_object));

  storage_->push_back(new_object);
}

//***************************************************************************
// Removal
//***************************************************************************

//---------------------------------------------------------------------------
//: Remove object `i' of `this' (not delete it)
// Require: i>=0 and i<size()
//---------------------------------------------------------------------------
void vsol_group_3d::remove_object(const int i)
{
  // require
  assert((i>=0)&&(i<size()));

  vcl_list<vsol_spatial_object_3d_sptr>::iterator j;
  int k;

  j=storage_->begin();
  for (k=0;k<i;++k)
    ++j;
  storage_->erase(j);
}

//---------------------------------------------------------------------------
//: Is `new_object' a child (direct or not) of `this' ?
//---------------------------------------------------------------------------
bool
vsol_group_3d::is_child(const vsol_spatial_object_3d_sptr &new_object) const
{
  vcl_list<vsol_spatial_object_3d_sptr>::iterator i;
  for (i=storage_->begin(); i!=storage_->end(); ++i)
  {
    if ((*i).ptr()==new_object.ptr())
      return true;
    vsol_group_3d const* g=(*i)->cast_to_group();
    if (g!=0 && g->is_child(new_object))
      return true;
  }
  return false;
}
