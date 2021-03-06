#include <Classes.hpp>
#include "rtti.hpp"

//---------------------------------------------------------------------------

TClassInfo * TClassInfo::sm_first = nullptr;
THashTable * TClassInfo::sm_classTable = nullptr;

TClassInfo::~TClassInfo()
{
  // remove this object from the linked list of all class infos: if we don't
  // do it, loading/unloading a DLL containing static wxClassInfo objects is
  // not going to work
  if (this == sm_first)
  {
    sm_first = m_next;
  }
  else
  {
    TClassInfo *info = sm_first;
    while (info)
    {
      if (info->m_next == this)
      {
        info->m_next = m_next;
        break;
      }

      info = info->m_next;
    }
  }
  Unregister();
}

TClassInfo * TClassInfo::FindClass(int classId)
{
  if (sm_classTable)
  {
    return (TClassInfo *)TClassInfo::sm_classTable->Get(classId);
  }
  else
  {
    for (TClassInfo * info = sm_first; info; info = info->m_next)
    {
      if (classId == info->GetClassId())
        return info;
    }

    return nullptr;
  }
}

void TClassInfo::Register()
{
#if _DEBUG
  // reentrance guard
  static int entry = 0;
#endif

  THashTable * classTable;

  if (!sm_classTable)
  {
    // keep the hash local initially, reentrance is possible
    classTable = new THashTable();
    classTable->reserve(50);
  }
  else
  {
    // guard against reentrance once the global has been created
    assert(++entry == 1 || "TClassInfo::Register() reentrance");
    classTable = sm_classTable;
  }

  assert(classTable->Get(m_classId) == nullptr);

  classTable->Put(m_classId, this);

  // if we're using a local hash we need to try to make it global
  if (sm_classTable != classTable)
  {
    if (!sm_classTable)
    {
      // make the hash global
      sm_classTable = classTable;
    }
    else
    {
      // the gobal hash has already been created by a reentrant call,
      // so delete the local hash and try again
      delete classTable;
      Register();
    }
  }

#if _DEBUG
  entry = 0;
#endif
}

void TClassInfo::Unregister()
{
  if (sm_classTable)
  {
    sm_classTable->Delete(m_classId);
    if (sm_classTable->GetCount() == 0)
    {
      delete sm_classTable;
    }
  }
}

//---------------------------------------------------------------------------

const TObject * NbStaticDownCastConst(TObjectClassId ClassId, const TObject * Object)
{
  if (Object != nullptr && Object->IsKindOf(ClassId))
    return Object;
  else
    return nullptr;
}

TObject * NbStaticDownCast(TObjectClassId ClassId, TObject * Object)
{
  if (Object != nullptr && Object->IsKindOf(ClassId))
    return Object;
  else
    return nullptr;
}

//---------------------------------------------------------------------------
