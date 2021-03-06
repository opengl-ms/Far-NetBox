//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define CLEAN_SPACE_AVAILABLE

#include "SftpFileSystem.h"

#include "PuttyTools.h"
#include "Common.h"
#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "SecureShell.h"
#include <limits>

#include <memory>
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
#define FILE_OPERATION_LOOP_EX(ALLOW_SKIP, MESSAGE, OPERATION) \
  FILE_OPERATION_LOOP_CUSTOM(FTerminal, ALLOW_SKIP, MESSAGE, OPERATION, L"")
//---------------------------------------------------------------------------
#define SSH_FX_OK                                 0
#define SSH_FX_EOF                                1
#define SSH_FX_NO_SUCH_FILE                       2
#define SSH_FX_PERMISSION_DENIED                  3
#define SSH_FX_FAILURE                            4
#define SSH_FX_OP_UNSUPPORTED                     8

#define SSH_FXP_INIT               1
#define SSH_FXP_VERSION            2
#define SSH_FXP_OPEN               3
#define SSH_FXP_CLOSE              4
#define SSH_FXP_READ               5
#define SSH_FXP_WRITE              6
#define SSH_FXP_LSTAT              7
#define SSH_FXP_FSTAT              8
#define SSH_FXP_SETSTAT            9
#define SSH_FXP_FSETSTAT           10
#define SSH_FXP_OPENDIR            11
#define SSH_FXP_READDIR            12
#define SSH_FXP_REMOVE             13
#define SSH_FXP_MKDIR              14
#define SSH_FXP_RMDIR              15
#define SSH_FXP_REALPATH           16
#define SSH_FXP_STAT               17
#define SSH_FXP_RENAME             18
#define SSH_FXP_READLINK           19
#define SSH_FXP_SYMLINK            20
#define SSH_FXP_STATUS             101
#define SSH_FXP_HANDLE             102
#define SSH_FXP_DATA               103
#define SSH_FXP_NAME               104
#define SSH_FXP_ATTRS              105
#define SSH_FXP_EXTENDED           200
#define SSH_FXP_EXTENDED_REPLY     201
#define SSH_FXP_ATTRS              105

#define SSH_FILEXFER_ATTR_SIZE              0x00000001
#define SSH_FILEXFER_ATTR_UIDGID            0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS       0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME         0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED          0x80000000
#define SSH_FILEXFER_ATTR_ACCESSTIME        0x00000008
#define SSH_FILEXFER_ATTR_CREATETIME        0x00000010
#define SSH_FILEXFER_ATTR_MODIFYTIME        0x00000020
#define SSH_FILEXFER_ATTR_ACL               0x00000040
#define SSH_FILEXFER_ATTR_OWNERGROUP        0x00000080
#define SSH_FILEXFER_ATTR_SUBSECOND_TIMES   0x00000100
#define SSH_FILEXFER_ATTR_BITS              0x00000200
#define SSH_FILEXFER_ATTR_EXTENDED          0x80000000

#define SSH_FILEXFER_ATTR_COMMON \
  (SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_OWNERGROUP | \
   SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_ACCESSTIME | \
   SSH_FILEXFER_ATTR_MODIFYTIME)

#define SSH_FILEXFER_TYPE_REGULAR          1
#define SSH_FILEXFER_TYPE_DIRECTORY        2
#define SSH_FILEXFER_TYPE_SYMLINK          3
#define SSH_FILEXFER_TYPE_SPECIAL          4
#define SSH_FILEXFER_TYPE_UNKNOWN          5

#define SSH_FXF_READ            0x00000001
#define SSH_FXF_WRITE           0x00000002
#define SSH_FXF_APPEND          0x00000004
#define SSH_FXF_CREAT           0x00000008
#define SSH_FXF_TRUNC           0x00000010
#define SSH_FXF_EXCL            0x00000020
#define SSH_FXF_TEXT            0x00000040

#define SSH_FXF_ACCESS_DISPOSITION        0x00000007
#define     SSH_FXF_CREATE_NEW            0x00000000
#define     SSH_FXF_CREATE_TRUNCATE       0x00000001
#define     SSH_FXF_OPEN_EXISTING         0x00000002
#define     SSH_FXF_OPEN_OR_CREATE        0x00000003
#define     SSH_FXF_TRUNCATE_EXISTING     0x00000004
#define SSH_FXF_ACCESS_APPEND_DATA        0x00000008
#define SSH_FXF_ACCESS_APPEND_DATA_ATOMIC 0x00000010
#define SSH_FXF_ACCESS_TEXT_MODE          0x00000020
#define SSH_FXF_ACCESS_READ_LOCK          0x00000040
#define SSH_FXF_ACCESS_WRITE_LOCK         0x00000080
#define SSH_FXF_ACCESS_DELETE_LOCK        0x00000100

#define ACE4_READ_DATA         0x00000001
#define ACE4_LIST_DIRECTORY    0x00000001
#define ACE4_WRITE_DATA        0x00000002
#define ACE4_ADD_FILE          0x00000002
#define ACE4_APPEND_DATA       0x00000004
#define ACE4_ADD_SUBDIRECTORY  0x00000004
#define ACE4_READ_NAMED_ATTRS  0x00000008
#define ACE4_WRITE_NAMED_ATTRS 0x00000010
#define ACE4_EXECUTE           0x00000020
#define ACE4_DELETE_CHILD      0x00000040
#define ACE4_READ_ATTRIBUTES   0x00000080
#define ACE4_WRITE_ATTRIBUTES  0x00000100
#define ACE4_DELETE            0x00010000
#define ACE4_READ_ACL          0x00020000
#define ACE4_WRITE_ACL         0x00040000
#define ACE4_WRITE_OWNER       0x00080000
#define ACE4_SYNCHRONIZE       0x00100000

#define SSH_FILEXFER_ATTR_FLAGS_HIDDEN           0x00000004

#define SFTP_MAX_PACKET_LEN   1024000
//---------------------------------------------------------------------------
#define SFTP_EXT_OWNER_GROUP "owner-group-query@generic-extensions"
#define SFTP_EXT_OWNER_GROUP_REPLY "owner-group-query-reply@generic-extensions"
#define SFTP_EXT_NEWLINE "newline"
#define SFTP_EXT_SUPPORTED "supported"
#define SFTP_EXT_SUPPORTED2 "supported2"
#define SFTP_EXT_FSROOTS "fs-roots@vandyke.com"
#define SFTP_EXT_VENDOR_ID "vendor-id"
#define SFTP_EXT_VERSIONS "versions"
#define SFTP_EXT_SPACE_AVAILABLE "space-available"
#define SFTP_EXT_CHECK_FILE "check-file"
#define SFTP_EXT_CHECK_FILE_NAME "check-file-name"
#define SFTP_EXT_STATVFS "statvfs@openssh.com"
#define SFTP_EXT_STATVFS_VALUE_V2 L"2"
#define SFTP_EXT_STATVFS_ST_RDONLY 0x1
#define SFTP_EXT_STATVFS_ST_NOSUID 0x2
//---------------------------------------------------------------------------
#define OGQ_LIST_OWNERS 0x01
#define OGQ_LIST_GROUPS 0x02
//---------------------------------------------------------------------------
const int SFTPMinVersion = 0;
const int SFTPMaxVersion = 5;
const unsigned int SFTPNoMessageNumber = static_cast<unsigned int>(-1);

const int asNo =            0;
const int asOK =            1 << SSH_FX_OK;
const int asEOF =           1 << SSH_FX_EOF;
const int asPermDenied =    1 << SSH_FX_PERMISSION_DENIED;
const int asOpUnsupported = 1 << SSH_FX_OP_UNSUPPORTED;
const int asNoSuchFile =    1 << SSH_FX_NO_SUCH_FILE;
const int asAll = 0xFFFF;

#if defined(__BORLANDC__)
const int tfFirstLevel =   0x01;
const int tfNewDirectory = 0x02;
#endif
//---------------------------------------------------------------------------
#ifndef GET_32BIT
#define GET_32BIT(cp) \
    (((uint32_t)(uint8_t)(cp)[0] << 24) | \
    ((uint32_t)(uint8_t)(cp)[1] << 16) | \
    ((uint32_t)(uint8_t)(cp)[2] << 8) | \
    ((uint32_t)(uint8_t)(cp)[3]))
#endif
#ifndef PUT_32BIT
#define PUT_32BIT(cp, value) { \
    (cp)[0] = (uint8_t)((value) >> 24); \
    (cp)[1] = (uint8_t)((value) >> 16); \
    (cp)[2] = (uint8_t)((value) >> 8); \
    (cp)[3] = (uint8_t)(value); }
#endif
//---------------------------------------------------------------------------
#define SFTP_PACKET_ALLOC_DELTA 256
//---------------------------------------------------------------------------
#pragma warn -inl
//---------------------------------------------------------------------------
struct TSFTPSupport : public TObject
{
NB_DISABLE_COPY(TSFTPSupport)
public:
  TSFTPSupport() :
    AttribExtensions(new TStringList()),
    Extensions(new TStringList())
  {
    Reset();
  }

  ~TSFTPSupport()
  {
    SAFE_DESTROY(AttribExtensions);
    SAFE_DESTROY(Extensions);
  }

  void Reset()
  {
    AttributeMask = 0;
    AttributeBits = 0;
    OpenFlags = 0;
    AccessMask = 0;
    MaxReadSize = 0;
    OpenBlockMasks = 0;
    BlockMasks = 0;
    AttribExtensions->Clear();
    Extensions->Clear();
    Loaded = false;
  }

  uint32_t AttributeMask;
  uint32_t AttributeBits;
  uint32_t OpenFlags;
  uint32_t AccessMask;
  uint32_t MaxReadSize;
  uint32_t OpenBlockMasks;
  uint32_t BlockMasks;
  TStrings * AttribExtensions;
  TStrings * Extensions;
  bool Loaded;
};
//---------------------------------------------------------------------------
class TSFTPPacket : public TObject
{
public:
  explicit TSFTPPacket(uintptr_t codePage)
  {
    Init(codePage);
  }

  explicit TSFTPPacket(const TSFTPPacket & Source, uintptr_t codePage)
  {
    Init(codePage);
    *this = Source;
  }

  explicit TSFTPPacket(uint8_t AType, uintptr_t codePage)
  {
    Init(codePage);
    ChangeType(AType);
  }

  explicit TSFTPPacket(const uint8_t * Source, uintptr_t Len, uintptr_t codePage)
  {
    Init(codePage);
    FLength = Len;
    SetCapacity(FLength);
    memmove(GetData(), Source, Len);
  }

  explicit TSFTPPacket(const RawByteString & Source, uintptr_t codePage)
  {
    Init(codePage);
    FLength = (uintptr_t)Source.Length();
    SetCapacity(FLength);
    memmove(GetData(), Source.c_str(), Source.Length());
  }

  ~TSFTPPacket()
  {
    if (FData != nullptr)
    {
      nb_free(FData - FSendPrefixLen);
    }
    if (FReservedBy)
    {
      FReservedBy->UnreserveResponse(this);
    }
  }

  void ChangeType(uint8_t AType)
  {
    FPosition = 0;
    FLength = 0;
    SetCapacity(0);
    FType = AType;
    AddByte(FType);
    if (FType != SSH_FXP_INIT) // && (FType != 1)
    {
      AssignNumber();
      AddCardinal(FMessageNumber);
    }
  }

  void Reuse()
  {
    AssignNumber();

    assert(GetLength() >= 5);

    // duplicated in AddCardinal()
    uint8_t Buf[4];
    PUT_32BIT(Buf, FMessageNumber);

    memmove(FData + 1, Buf, sizeof(Buf));
  }

  void AddByte(uint8_t Value)
  {
    Add(&Value, sizeof(Value));
  }

  void AddCardinal(uint32_t Value)
  {
    // duplicated in Reuse()
    uint8_t Buf[4];
    PUT_32BIT(Buf, Value);
    Add(&Buf, sizeof(Buf));
  }

  void AddInt64(__int64 Value)
  {
    AddCardinal((uint32_t)(Value >> 32));
    AddCardinal((uint32_t)(Value & 0xFFFFFFFF));
  }

  void AddData(const void * Data, int32_t ALength)
  {
    AddCardinal(ALength);
    Add(Data, ALength);
  }

  void AddStringW(const UnicodeString & ValueW)
  {
    AddString(W2MB(ValueW.c_str(), (UINT)FCodePage).c_str());
  }

  void AddString(const RawByteString & Value)
  {
    AddCardinal(static_cast<uint32_t>(Value.Length()));
    Add(Value.c_str(), Value.Length());
  }

  inline void AddUtfString(const UTF8String & Value)
  {
    AddString(Value);
  }

  inline void AddUtfString(const UnicodeString & Value)
  {
    AddUtfString(UTF8String(Value));
  }

  inline void AddString(const UnicodeString & Value, bool Utf)
  {
    (void)Utf;
    AddStringW(Value);
  }

  // now purposeless alias to AddString
  inline void AddPathString(const UnicodeString & Value, bool Utf)
  {
    AddString(Value, Utf);
  }

  void AddProperties(unsigned short * Rights, TRemoteToken * Owner,
    TRemoteToken * Group, __int64 * MTime, __int64 * ATime,
    __int64 * Size, bool IsDirectory, intptr_t Version, bool Utf)
  {
    uint32_t Flags = 0;
    if (Size != nullptr)
    {
      Flags |= SSH_FILEXFER_ATTR_SIZE;
    }
    // both or neither
    assert((Owner != nullptr) == (Group != nullptr));
    if ((Owner != nullptr) && (Group != nullptr))
    {
      if (Version < 4)
      {
        assert(Owner->GetIDValid() && Group->GetIDValid());
        Flags |= SSH_FILEXFER_ATTR_UIDGID;
      }
      else
      {
        assert(Owner->GetNameValid() && Group->GetNameValid());
        Flags |= SSH_FILEXFER_ATTR_OWNERGROUP;
      }
    }
    if (Rights != nullptr)
    {
      Flags |= SSH_FILEXFER_ATTR_PERMISSIONS;
    }
    if ((Version < 4) && ((MTime != nullptr) || (ATime != nullptr)))
    {
      Flags |= SSH_FILEXFER_ATTR_ACMODTIME;
    }
    if ((Version >= 4) && (ATime != nullptr))
    {
      Flags |= SSH_FILEXFER_ATTR_ACCESSTIME;
    }
    if ((Version >= 4) && (MTime != nullptr))
    {
      Flags |= SSH_FILEXFER_ATTR_MODIFYTIME;
    }
    AddCardinal(Flags);

    if (Version >= 4)
    {
      AddByte(static_cast<uint8_t>(IsDirectory ?
        SSH_FILEXFER_TYPE_DIRECTORY : SSH_FILEXFER_TYPE_REGULAR));
    }

    if (Size != nullptr)
    {
      AddInt64(*Size);
    }

    if ((Owner != nullptr) && (Group != nullptr))
    {
      if (Version < 4)
      {
        assert(Owner->GetIDValid() && Group->GetIDValid());
        AddCardinal(static_cast<uint32_t>(Owner->GetID()));
        AddCardinal(static_cast<uint32_t>(Group->GetID()));
      }
      else
      {
        assert(Owner->GetNameValid() && Group->GetNameValid());
        AddString(Owner->GetName(), Utf);
        AddString(Group->GetName(), Utf);
      }
    }

    if (Rights != nullptr)
    {
      AddCardinal(*Rights);
    }

    if ((Version < 4) && ((MTime != nullptr) || (ATime != nullptr)))
    {
      // any way to reflect sbSignedTS here?
      // (note that casting __int64 > 2^31 < 2^32 to uint32_t is wrapped,
      // thus we never can set time after 2038, even if the server supports it)
      AddCardinal(static_cast<uint32_t>(ATime != nullptr ? *ATime : *MTime));
      AddCardinal(static_cast<uint32_t>(MTime != nullptr ? *MTime : *ATime));
    }
    if ((Version >= 4) && (ATime != nullptr))
    {
      AddInt64(*ATime);
    }
    if ((Version >= 4) && (MTime != nullptr))
    {
      AddInt64(*MTime);
    }
  }

  void AddProperties(const TRemoteProperties * Properties,
    unsigned short BaseRights, bool IsDirectory, intptr_t Version, bool Utf,
    TChmodSessionAction * Action)
  {
    enum TValid { valNone = 0, valRights = 0x01, valOwner = 0x02, valGroup = 0x04,
      valMTime = 0x08, valATime = 0x10 } Valid = valNone;
    unsigned short RightsNum = 0;
    TRemoteToken Owner;
    TRemoteToken Group;
    __int64 MTime;
    __int64 ATime;

    if (Properties != nullptr)
    {
      if (Properties->Valid.Contains(vpGroup))
      {
        Valid = (TValid)(Valid | valGroup);
        Group = Properties->Group;
      }

      if (Properties->Valid.Contains(vpOwner))
      {
        Valid = (TValid)(Valid | valOwner);
        Owner = Properties->Owner;
      }

      if (Properties->Valid.Contains(vpRights))
      {
        Valid = (TValid)(Valid | valRights);
        TRights Rights = TRights(BaseRights);
        Rights |= Properties->Rights.GetNumberSet();
        Rights &= static_cast<unsigned short>(~Properties->Rights.GetNumberUnset());
        if (IsDirectory && Properties->AddXToDirectories)
        {
          Rights.AddExecute();
        }
        RightsNum = Rights;

        if (Action != nullptr)
        {
          Action->Rights(Rights);
        }
      }

      if (Properties->Valid.Contains(vpLastAccess))
      {
        Valid = (TValid)(Valid | valATime);
        ATime = Properties->LastAccess;
      }

      if (Properties->Valid.Contains(vpModification))
      {
        Valid = (TValid)(Valid | valMTime);
        MTime = Properties->Modification;
      }
    }

    AddProperties(
      Valid & valRights ? &RightsNum : nullptr,
      Valid & valOwner ? &Owner : nullptr,
      Valid & valGroup ? &Group : nullptr,
      Valid & valMTime ? &MTime : nullptr,
      Valid & valATime ? &ATime : nullptr,
      nullptr, IsDirectory, Version, Utf);
  }

  uint8_t GetByte() const
  {
    Need(sizeof(uint8_t));
    uint8_t Result = FData[FPosition];
    FPosition++;
    return Result;
  }

  uint32_t GetCardinal() const
  {
    uint32_t Result;
    Need(sizeof(Result));
    Result = GET_32BIT(FData + FPosition);
    FPosition += sizeof(Result);
    return Result;
  }

  uint32_t GetSmallCardinal() const
  {
    uint32_t Result;
    Need(2);
    Result = (FData[FPosition] << 8) + FData[FPosition + 1];
    FPosition += 2;
    return Result;
  }

  __int64 GetInt64() const
  {
    __int64 Hi = GetCardinal();
    __int64 Lo = GetCardinal();
    return (Hi << 32) + Lo;
  }

  RawByteString GetRawByteString() const
  {
    RawByteString Result;
    uintptr_t Len = GetCardinal();
    Need(Len);
    // cannot happen anyway as Need() would raise exception
    assert(Len < SFTP_MAX_PACKET_LEN);
    Result.SetLength(Len);
    memmove((void *)Result.c_str(), FData + FPosition, Len);
    FPosition += Len;
    return Result;
  }

  inline UnicodeString GetUtfString() const
  {
    return UnicodeString(UTF8String(GetRawByteString().c_str()));
  }

  // For reading string that are character strings (not byte strings as
  // as file handles), and SFTP spec does not say explicitly that they
  // are in UTF. For most of them it actually does not matter as
  // the content should be pure ASCII (e.g. extension names, etc.)
  inline UnicodeString GetAnsiString()
  {
    return UnicodeString(AnsiString(GetRawByteString().c_str()).c_str());
  }

  inline RawByteString GetFileHandle() const
  {
    return GetRawByteString();
  }

  inline UnicodeString GetStringW() const
  {
    return MB2W(GetRawByteString().c_str(), (UINT)FCodePage);
  }

  inline UnicodeString GetString(bool Utf) const
  {
    (void)Utf;
    return GetStringW();
  }

  // now purposeless alias to GetString(bool)
  inline UnicodeString GetPathString(bool Utf)
  {
    return GetString(Utf);
  }

  void GetFile(TRemoteFile * File, intptr_t Version, TDSTMode DSTMode, bool Utf, bool SignedTS, bool Complete)
  {
    assert(File);
    uintptr_t Flags;
    UnicodeString ListingStr;
    uintptr_t Permissions = 0;
    bool ParsingFailed = false;
    if (GetType() != SSH_FXP_ATTRS)
    {
      File->SetFileName(GetPathString(Utf));
      if (Version < 4)
      {
        ListingStr = GetAnsiString();
      }
    }
    Flags = GetCardinal();
    if (Version >= 4)
    {
      uint8_t FXType = GetByte();
      // -:regular, D:directory, L:symlink, S:special, U:unknown
      // O:socket, C:char device, B:block device, F:fifo

      // SSH-2.0-cryptlib returns file type 0 in response to SSH_FXP_LSTAT,
      // handle this undefined value as "unknown"
      static wchar_t * Types = L"U-DLSUOCBF";
      if (FXType > (uint8_t)wcslen(Types))
      {
        throw Exception(FMTLOAD(SFTP_UNKNOWN_FILE_TYPE, static_cast<int>(FXType)));
      }
      File->SetType(Types[FXType]);
    }
    if (Flags & SSH_FILEXFER_ATTR_SIZE)
    {
      File->SetSize(GetInt64());
    }
    // SSH-2.0-3.2.0 F-SECURE SSH - Process Software MultiNet
    // sets SSH_FILEXFER_ATTR_UIDGID for v4, but does not include the UID/GUID
    if ((Flags & SSH_FILEXFER_ATTR_UIDGID) && (Version < 4))
    {
      File->GetFileOwner().SetID(GetCardinal());
      File->GetFileGroup().SetID(GetCardinal());
    }
    if (Flags & SSH_FILEXFER_ATTR_OWNERGROUP)
    {
      assert(Version >= 4);
      File->GetFileOwner().SetName(GetString(Utf));
      File->GetFileGroup().SetName(GetString(Utf));
    }
    if (Flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      Permissions = GetCardinal();
    }
    if (Version < 4)
    {
      if (Flags & SSH_FILEXFER_ATTR_ACMODTIME)
      {
        File->SetLastAccess(UnixToDateTime(
          SignedTS ?
            static_cast<__int64>(static_cast<int32_t>(GetCardinal())) :
            static_cast<__int64>(GetCardinal()),
          DSTMode));
        File->SetModification(UnixToDateTime(
          SignedTS ?
            static_cast<__int64>(static_cast<int32_t>(GetCardinal())) :
            static_cast<__int64>(GetCardinal()),
          DSTMode));
      }
    }
    else
    {
      if (Flags & SSH_FILEXFER_ATTR_ACCESSTIME)
      {
        File->SetLastAccess(UnixToDateTime(GetInt64(), DSTMode));
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip access time subseconds
        }
      }
      else
      {
        File->SetLastAccess(Now());
      }
      if (Flags & SSH_FILEXFER_ATTR_CREATETIME)
      {
        GetInt64(); // skip create time
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip create time subseconds
        }
      }
      if (Flags & SSH_FILEXFER_ATTR_MODIFYTIME)
      {
        File->SetModification(UnixToDateTime(GetInt64(), DSTMode));
        if (Flags & SSH_FILEXFER_ATTR_SUBSECOND_TIMES)
        {
          GetCardinal(); // skip modification time subseconds
        }
      }
      else
      {
        File->SetModification(Now());
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_ACL)
    {
      GetRawByteString();
    }

    if (Flags & SSH_FILEXFER_ATTR_BITS)
    {
      // while SSH_FILEXFER_ATTR_BITS is defined for SFTP5 only, vandyke 2.3.3 sets it
      // for SFTP4 as well
      uintptr_t Bits = GetCardinal();
      if (FLAGSET(Bits, SSH_FILEXFER_ATTR_FLAGS_HIDDEN))
      {
        File->SetIsHidden(true);
      }
    }

    if ((Version < 4) && (GetType() != SSH_FXP_ATTRS))
    {
      try
      {
        // update permissions and user/group name
        // modification time and filename is ignored
        File->SetListingStr(ListingStr);
      }
      catch (...)
      {
        // ignore any error while parsing listing line,
        // SFTP specification do not recommend to parse it
        ParsingFailed = true;
      }
    }

    if (GetType() == SSH_FXP_ATTRS || Version >= 4 || ParsingFailed)
    {
      wchar_t Type = L'-';
      if (FLAGSET(Flags, SSH_FILEXFER_ATTR_PERMISSIONS))
      {
        File->GetRights()->SetNumber(static_cast<unsigned short>(Permissions & TRights::rfAllSpecials));
        if (FLAGSET(Permissions, TRights::rfDirectory))
        {
          Type = FILETYPE_DIRECTORY;
        }
      }

      if (Version < 4)
      {
        File->SetType(Type);
      }
    }

    if (Flags & SSH_FILEXFER_ATTR_EXTENDED)
    {
      uintptr_t ExtendedCount = GetCardinal();
      for (intptr_t Index = 0; Index < static_cast<intptr_t>(ExtendedCount); ++Index)
      {
        GetRawByteString(); // skip extended_type
        GetRawByteString(); // skip extended_data
      }
    }

    if (Complete)
    {
      File->Complete();
    }
  }

  uint8_t * GetNextData(uintptr_t Size = 0)
  {
    if (Size > 0)
    {
      Need(Size);
    }
    return FPosition < FLength ? FData + FPosition : nullptr;
  }

  void DataUpdated(uintptr_t ALength)
  {
    FPosition = 0;
    FLength = ALength;
    FType = GetByte();
    if (FType != SSH_FXP_VERSION)
    {
      FMessageNumber = GetCardinal();
    }
    else
    {
      FMessageNumber = SFTPNoMessageNumber;
    }
  }

  void LoadFromFile(const UnicodeString & FileName)
  {
    std::unique_ptr<TStringList> DumpLines(new TStringList());
    RawByteString Dump;
    DumpLines->LoadFromFile(FileName);
    Dump = AnsiString(DumpLines->GetText());

    SetCapacity(1 * 1024 * 1024); // 20480);
    uint8_t Byte[3];
    memset(Byte, '\0', sizeof(Byte));
    intptr_t Index = 1;
    uintptr_t Length = 0;
    while (Index < Dump.Length())
    {
      char C = Dump[Index];
      if (IsHex(C))
      {
        if (Byte[0] == '\0')
        {
          Byte[0] = C;
        }
        else
        {
          Byte[1] = C;
          assert(Length < GetCapacity());
          GetData()[Length] = HexToByte(UnicodeString(reinterpret_cast<char *>(Byte)));
          Length++;
          memset(Byte, '\0', sizeof(Byte));
        }
      }
      ++Index;
    }
    DataUpdated(Length);
  }

  UnicodeString Dump() const
  {
    UnicodeString Result;
    for (uintptr_t Index = 0; Index < GetLength(); ++Index)
    {
      Result += ByteToHex(GetData()[Index]) + L",";
      if (((Index + 1) % 25) == 0)
      {
        Result += L"\n";
      }
    }
    return Result;
  }

  TSFTPPacket & operator = (const TSFTPPacket & Source)
  {
    SetCapacity(0);
    Add(Source.GetData(), Source.GetLength());
    DataUpdated(Source.GetLength());
    FPosition = Source.FPosition;
    return *this;
  }

  uintptr_t GetLength() const { return FLength; }
  uint8_t * GetData() const { return FData; }
  uintptr_t GetCapacity() const { return FCapacity; }
  uint8_t GetType() const { return FType; }
  uintptr_t GetMessageNumber() const { return static_cast<uintptr_t>(FMessageNumber); }
  void SetMessageNumber(uint32_t Value) { FMessageNumber = Value; }
  TSFTPFileSystem * GetReservedBy() const { return FReservedBy; }
  void SetReservedBy(TSFTPFileSystem * Value) { FReservedBy = Value; }

private:
  uint8_t * FData;
  uintptr_t FLength;
  uintptr_t FCapacity;
  mutable uintptr_t FPosition;
  uint8_t FType;
  uint32_t FMessageNumber;
  TSFTPFileSystem * FReservedBy;

  static int FMessageCounter;
  static const int FSendPrefixLen = 4;
  uintptr_t FCodePage;

  void Init(uintptr_t codePage)
  {
    FData = nullptr;
    FCapacity = 0;
    FLength = 0;
    FPosition = 0;
    FMessageNumber = SFTPNoMessageNumber;
    FType = static_cast<uint8_t>(-1);
    FReservedBy = nullptr;
    FCodePage = codePage;
  }

  void AssignNumber()
  {
    // this is not strictly thread-safe, but as it is accessed from multiple
    // threads only for multiple connection, it is not problem if two threads get
    // the same number
    FMessageNumber = (FMessageCounter << 8) + FType;
    FMessageCounter++;
  }

public:
  uint8_t GetRequestType() const
  {
    if (FMessageNumber != SFTPNoMessageNumber)
    {
      return static_cast<uint8_t>(FMessageNumber & 0xFF);
    }
    else
    {
      assert(GetType() == SSH_FXP_VERSION);
      return SSH_FXP_INIT;
    }
  }

  void Add(const void * AData, uintptr_t ALength)
  {
    if (GetLength() + ALength > GetCapacity())
    {
      SetCapacity(GetLength() + ALength + SFTP_PACKET_ALLOC_DELTA);
    }
    memmove(FData + GetLength(), AData, ALength);
    FLength += ALength;
  }

  void SetCapacity(uintptr_t ACapacity)
  {
    if (ACapacity != GetCapacity())
    {
      FCapacity = ACapacity;
      if (FCapacity > 0)
      {
        uint8_t * NData = static_cast<uint8_t *>(
          nb_malloc(FCapacity + FSendPrefixLen));
        NData += FSendPrefixLen;
        if (FData)
        {
          memmove(NData - FSendPrefixLen, FData - FSendPrefixLen,
            (FLength < FCapacity ? FLength : FCapacity) + FSendPrefixLen);
          nb_free(FData - FSendPrefixLen);
        }
        FData = NData;
      }
      else
      {
        if (FData)
        {
          nb_free(FData - FSendPrefixLen);
        }
        FData = nullptr;
      }
      if (FLength > FCapacity)
      {
        FLength = FCapacity;
      }
    }
  }

  UnicodeString GetTypeName() const
  {
    #define TYPE_CASE(TYPE) case TYPE: return TEXT(#TYPE)
    switch (GetType())
    {
      TYPE_CASE(SSH_FXP_INIT);
      TYPE_CASE(SSH_FXP_VERSION);
      TYPE_CASE(SSH_FXP_OPEN);
      TYPE_CASE(SSH_FXP_CLOSE);
      TYPE_CASE(SSH_FXP_READ);
      TYPE_CASE(SSH_FXP_WRITE);
      TYPE_CASE(SSH_FXP_LSTAT);
      TYPE_CASE(SSH_FXP_FSTAT);
      TYPE_CASE(SSH_FXP_SETSTAT);
      TYPE_CASE(SSH_FXP_FSETSTAT);
      TYPE_CASE(SSH_FXP_OPENDIR);
      TYPE_CASE(SSH_FXP_READDIR);
      TYPE_CASE(SSH_FXP_REMOVE);
      TYPE_CASE(SSH_FXP_MKDIR);
      TYPE_CASE(SSH_FXP_RMDIR);
      TYPE_CASE(SSH_FXP_REALPATH);
      TYPE_CASE(SSH_FXP_STAT);
      TYPE_CASE(SSH_FXP_RENAME);
      TYPE_CASE(SSH_FXP_READLINK);
      TYPE_CASE(SSH_FXP_SYMLINK);
      TYPE_CASE(SSH_FXP_STATUS);
      TYPE_CASE(SSH_FXP_HANDLE);
      TYPE_CASE(SSH_FXP_DATA);
      TYPE_CASE(SSH_FXP_NAME);
      TYPE_CASE(SSH_FXP_ATTRS);
      TYPE_CASE(SSH_FXP_EXTENDED);
      TYPE_CASE(SSH_FXP_EXTENDED_REPLY);
      default:
        return FORMAT(L"Unknown message (%d)", static_cast<int>(GetType()));
    }
  }

  uint8_t * GetSendData() const
  {
    uint8_t * Result = FData - FSendPrefixLen;
    // this is not strictly const-object operation
    PUT_32BIT(Result, GetLength());
    return Result;
  }

  uintptr_t GetSendLength() const
  {
    return FSendPrefixLen + GetLength();
  }

  uintptr_t GetRemainingLength() const
  {
    return GetLength() - FPosition;
  }

private:
  inline void Need(uintptr_t Size) const
  {
    if (FPosition + Size > FLength)
    {
      throw Exception(FMTLOAD(SFTP_PACKET_ERROR, static_cast<int>(FPosition), static_cast<int>(Size), static_cast<int>(FLength)));
    }
  }
};
//---------------------------------------------------------------------------
int TSFTPPacket::FMessageCounter = 0;
//---------------------------------------------------------------------------
class TSFTPQueue : public TObject
{
NB_DISABLE_COPY(TSFTPQueue)
public:
  explicit TSFTPQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) :
    FRequests(new TList()),
    FResponses(new TList()),
    FFileSystem(AFileSystem),
    FCodePage(CodePage)
  {
    assert(FFileSystem);
  }

  virtual ~TSFTPQueue()
  {
    assert(FResponses->GetCount() == FRequests->GetCount());
    for (intptr_t Index = 0; Index < FRequests->GetCount(); ++Index)
    {
      TSFTPQueuePacket * Request = static_cast<TSFTPQueuePacket*>(FRequests->GetItem(Index));
      assert(Request);
      SAFE_DESTROY(Request);

      TSFTPPacket * Response = static_cast<TSFTPPacket*>(FResponses->GetItem(Index));
      assert(Response);
      SAFE_DESTROY(Response);
    }
    SAFE_DESTROY(FRequests);
    SAFE_DESTROY(FResponses);
  }

  bool Init()
  {
    return SendRequests();
  }

  virtual void Dispose()
  {
    assert(FFileSystem->FTerminal->GetActive());

    while (FRequests->GetCount())
    {
      assert(FResponses->GetCount());

      TSFTPQueuePacket * Request = static_cast<TSFTPQueuePacket*>(FRequests->GetItem(0));
      assert(Request);

      TSFTPPacket * Response = static_cast<TSFTPPacket*>(FResponses->GetItem(0));
      assert(Response);

      try
      {
        FFileSystem->ReceiveResponse(Request, Response);
      }
      catch (Exception & E)
      {
        if (FFileSystem->FTerminal->GetActive())
        {
          FFileSystem->FTerminal->LogEvent(L"Error while disposing the SFTP queue.");
          FFileSystem->FTerminal->GetLog()->AddException(&E);
        }
        else
        {
          FFileSystem->FTerminal->LogEvent(L"Fatal error while disposing the SFTP queue.");
          throw;
        }
      }

      FRequests->Delete(0);
      SAFE_DESTROY(Request);
      FResponses->Delete(0);
      SAFE_DESTROY(Response);
    }
  }

  void DisposeSafe()
  {
    if (FFileSystem->FTerminal->GetActive())
    {
      Dispose();
    }
  }

  bool ReceivePacket(TSFTPPacket * Packet,
    int ExpectedType = -1, int AllowStatus = -1, void ** Token = nullptr)
  {
    assert(FRequests->GetCount());
    std::unique_ptr<TSFTPQueuePacket> Request(static_cast<TSFTPQueuePacket*>(FRequests->GetItem(0)));
    FRequests->Delete(0);
    assert(Request.get());
    if (Token != nullptr)
    {
      *Token = Request->Token;
    }

    std::unique_ptr<TSFTPPacket> Response(static_cast<TSFTPPacket*>(FResponses->GetItem(0)));
    FResponses->Delete(0);
    assert(Response.get());

    FFileSystem->ReceiveResponse(Request.get(), Response.get(),
      ExpectedType, AllowStatus);

    if (Packet)
    {
      *Packet = *Response.get();
    }

    bool Result = !End(Response.get());
    if (Result)
    {
      SendRequests();
    }

    return Result;
  }

  bool Next(int ExpectedType = -1, int AllowStatus = -1)
  {
    return ReceivePacket(nullptr, ExpectedType, AllowStatus);
  }

protected:
  TList * FRequests;
  TList * FResponses;
  TSFTPFileSystem * FFileSystem;
  uintptr_t FCodePage;

  class TSFTPQueuePacket : public TSFTPPacket
  {
  NB_DISABLE_COPY(TSFTPQueuePacket)
  public:
    explicit TSFTPQueuePacket(uintptr_t CodePage) :
      TSFTPPacket(CodePage),
      Token(nullptr)
    {
    }

    void * Token;
  };

  virtual bool InitRequest(TSFTPQueuePacket * Request) = 0;

  virtual bool End(TSFTPPacket * Response) = 0;

  virtual void SendPacket(TSFTPQueuePacket * Packet)
  {
    FFileSystem->SendPacket(Packet);
  }

  // sends as many requests as allowed by implementation
  virtual bool SendRequests() = 0;

  virtual bool SendRequest()
  {
    std::unique_ptr<TSFTPQueuePacket> Request(new TSFTPQueuePacket(FCodePage));
    if (!InitRequest(Request.get()))
    {
      Request.reset();
    }

    if (Request.get() != nullptr)
    {
      TSFTPPacket * Response = new TSFTPPacket(FCodePage);
      FRequests->Add(Request.get());
      FResponses->Add(Response);

      // make sure the response is reserved before actually ending the message
      // as we may receive response asynchronously before SendPacket finishes
      FFileSystem->ReserveResponse(Request.get(), Response);
      SendPacket(Request.release());
      return true;
    }

    return false;
  }
};
//---------------------------------------------------------------------------
class TSFTPFixedLenQueue : public TSFTPQueue
{
public:
  explicit TSFTPFixedLenQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) :
    TSFTPQueue(AFileSystem, CodePage),
    FMissedRequests(0)
  {
  }
  virtual ~TSFTPFixedLenQueue() {}

  bool Init(intptr_t QueueLen)
  {
    FMissedRequests = QueueLen - 1;
    return TSFTPQueue::Init();
  }

protected:
  intptr_t FMissedRequests;

  // sends as many requests as allowed by implementation
  virtual bool SendRequests()
  {
    bool Result = false;
    FMissedRequests++;
    while ((FMissedRequests > 0) && SendRequest())
    {
      Result = true;
      FMissedRequests--;
    }
    return Result;
  }
};
//---------------------------------------------------------------------------
class TSFTPAsynchronousQueue : public TSFTPQueue
{
public:
  explicit TSFTPAsynchronousQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) : TSFTPQueue(AFileSystem, CodePage)
  {
    FFileSystem->FSecureShell->RegisterReceiveHandler(MAKE_CALLBACK(TSFTPAsynchronousQueue::ReceiveHandler, this));
    FReceiveHandlerRegistered = true;
  }

  virtual ~TSFTPAsynchronousQueue()
  {
    UnregisterReceiveHandler();
  }

  virtual void Dispose()
  {
    // we do not want to receive asynchronous notifications anymore,
    // while waiting synchronously for pending responses
    UnregisterReceiveHandler();
    TSFTPQueue::Dispose();
  }

  bool Continue()
  {
    return SendRequest();
  }

protected:

  // event handler for incoming data
  void ReceiveHandler(TObject * /*Sender*/)
  {
    while (FFileSystem->PeekPacket() && ReceivePacketAsynchronously())
    {
      // loop
    }
  }

  virtual bool ReceivePacketAsynchronously() = 0;

  // sends as many requests as allowed by implementation
  virtual bool SendRequests()
  {
    // noop
    return true;
  }

  void UnregisterReceiveHandler()
  {
    if (FReceiveHandlerRegistered)
    {
      FReceiveHandlerRegistered = false;
      FFileSystem->FSecureShell->UnregisterReceiveHandler(MAKE_CALLBACK(TSFTPAsynchronousQueue::ReceiveHandler, this));
    }
  }

private:
  bool FReceiveHandlerRegistered;
};
//---------------------------------------------------------------------------
class TSFTPDownloadQueue : public TSFTPFixedLenQueue
{
NB_DISABLE_COPY(TSFTPDownloadQueue)
public:
  explicit TSFTPDownloadQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) :
    TSFTPFixedLenQueue(AFileSystem, CodePage)
  {
  }
  virtual ~TSFTPDownloadQueue() {}

  bool Init(intptr_t QueueLen, const RawByteString & AHandle, __int64 ATransfered,
    TFileOperationProgressType * AOperationProgress)
  {
    FHandle = AHandle;
    FTransfered = ATransfered;
    OperationProgress = AOperationProgress;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  void InitFillGapRequest(__int64 Offset, uint32_t Missing,
    TSFTPPacket * Packet)
  {
    InitRequest(Packet, Offset, Missing);
  }

  bool ReceivePacket(TSFTPPacket * Packet, uintptr_t & BlockSize)
  {
    void * Token;
    bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_DATA, asEOF, &Token);
    BlockSize = reinterpret_cast<uintptr_t>(Token);
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    uint32_t BlockSize = FFileSystem->DownloadBlockSize(OperationProgress);
    InitRequest(Request, FTransfered, BlockSize);
    Request->Token = reinterpret_cast<void*>(BlockSize);
    FTransfered += BlockSize;
    return true;
  }

  void InitRequest(TSFTPPacket * Request, __int64 Offset,
    uint32_t Size)
  {
    Request->ChangeType(SSH_FXP_READ);
    Request->AddString(FHandle);
    Request->AddInt64(Offset);
    Request->AddCardinal(Size);
  }

  virtual bool End(TSFTPPacket * Response)
  {
    return (Response->GetType() != SSH_FXP_DATA);
  }

private:
  TFileOperationProgressType * OperationProgress;
  __int64 FTransfered;
  RawByteString FHandle;
};
//---------------------------------------------------------------------------
class TSFTPUploadQueue : public TSFTPAsynchronousQueue
{
NB_DISABLE_COPY(TSFTPUploadQueue)
public:
  explicit TSFTPUploadQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) :
    TSFTPAsynchronousQueue(AFileSystem, CodePage),
    FStream(nullptr),
    OperationProgress(nullptr),
    FLastBlockSize(0),
    FEnd(false),
    FTransfered(0),
    FConvertToken(false),
    FConvertParams(0),
    FTerminal(nullptr)
  {
  }

  virtual ~TSFTPUploadQueue()
  {
    SAFE_DESTROY(FStream);
  }

  bool Init(const UnicodeString & AFileName,
    HANDLE AFile, TFileOperationProgressType * AOperationProgress,
    const RawByteString & AHandle, __int64 ATransfered,
    int ConvertParams)
  {
    FFileName = AFileName;
    FStream = new TSafeHandleStream(AFile);
    OperationProgress = AOperationProgress;
    FHandle = AHandle;
    FTransfered = ATransfered;
    FConvertParams = ConvertParams;

    return TSFTPAsynchronousQueue::Init();
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    FTerminal = FFileSystem->FTerminal;
    // Buffer for one block of data
    TFileBuffer BlockBuf;

    intptr_t BlockSize = GetBlockSize();
    bool Result = (BlockSize > 0);

    if (Result)
    {
      FILE_OPERATION_LOOP(FMTLOAD(READ_ERROR, FFileName.c_str()),
        BlockBuf.LoadStream(FStream, BlockSize, false);
      );

      FEnd = (BlockBuf.GetSize() == 0);
      Result = !FEnd;
      if (Result)
      {
        OperationProgress->AddLocallyUsed(BlockBuf.GetSize());

        // We do ASCII transfer: convert EOL of current block
        if (OperationProgress->AsciiTransfer)
        {
          __int64 PrevBufSize = BlockBuf.GetSize();
          BlockBuf.Convert(FTerminal->GetConfiguration()->GetLocalEOLType(),
            FFileSystem->GetEOL(), FConvertParams, FConvertToken);
          // update transfer size with difference araised from EOL conversion
          OperationProgress->ChangeTransferSize(OperationProgress->TransferSize -
            PrevBufSize + BlockBuf.GetSize());
        }

        if (FFileSystem->FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
        {
          FFileSystem->FTerminal->LogEvent(FORMAT(L"Write request offset: %d, len: %d",
            int(FTransfered), int(BlockBuf.GetSize())));
        }

        Request->ChangeType(SSH_FXP_WRITE);
        Request->AddString(FHandle);
        Request->AddInt64(FTransfered);
        Request->AddData(BlockBuf.GetData(), static_cast<uint32_t>(BlockBuf.GetSize()));
        FLastBlockSize = static_cast<uint32_t>(BlockBuf.GetSize());

        FTransfered += BlockBuf.GetSize();
      }
    }

    FTerminal = nullptr;
    return Result;
  }

  virtual void SendPacket(TSFTPQueuePacket * Packet)
  {
    TSFTPAsynchronousQueue::SendPacket(Packet);
    OperationProgress->AddTransfered(FLastBlockSize);
  }

  virtual bool ReceivePacketAsynchronously()
  {
    // do not read response to close request
    bool Result = (FRequests->GetCount() > 0);
    if (Result)
    {
      ReceivePacket(nullptr, SSH_FXP_STATUS);
    }
    return Result;
  }

  inline intptr_t GetBlockSize()
  {
    return FFileSystem->UploadBlockSize(FHandle, OperationProgress);
  }

  virtual bool End(TSFTPPacket * /*Response*/)
  {
    return FEnd;
  }

private:
  TStream * FStream;
  TFileOperationProgressType * OperationProgress;
  UnicodeString FFileName;
  uint32_t FLastBlockSize;
  bool FEnd;
  __int64 FTransfered;
  RawByteString FHandle;
  bool FConvertToken;
  int FConvertParams;
  TTerminal * FTerminal;
};
//---------------------------------------------------------------------------
class TSFTPLoadFilesPropertiesQueue : public TSFTPFixedLenQueue
{
NB_DISABLE_COPY(TSFTPLoadFilesPropertiesQueue)
public:
  explicit TSFTPLoadFilesPropertiesQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) :
    TSFTPFixedLenQueue(AFileSystem, CodePage),
    FIndex(0),
    FFileList(nullptr)
  {
  }
  virtual ~TSFTPLoadFilesPropertiesQueue() {}

  bool Init(uintptr_t QueueLen, TStrings * FileList)
  {
    FFileList = FileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool ReceivePacket(TSFTPPacket * Packet, TRemoteFile *& File)
  {
    void * Token;
    bool Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_ATTRS, asAll, &Token);
    File = static_cast<TRemoteFile *>(Token);
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->GetObject(FIndex));
      ++FIndex;

      bool MissingRights =
        (FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS) &&
           File->GetRights()->GetUnknown());
      bool MissingOwnerGroup =
        (FLAGSET(FFileSystem->FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP) &&
           !File->GetFileOwner().GetIsSet() || !File->GetFileGroup().GetIsSet());

      Result = (MissingRights || MissingOwnerGroup);
      if (Result)
      {
        Request->ChangeType(SSH_FXP_LSTAT);
        Request->AddPathString(FFileSystem->LocalCanonify(File->GetFileName()),
          FFileSystem->FUtfStrings);
        if (FFileSystem->FVersion >= 4)
        {
          Request->AddCardinal(
            FLAGMASK(MissingRights, SSH_FILEXFER_ATTR_PERMISSIONS) |
            FLAGMASK(MissingOwnerGroup, SSH_FILEXFER_ATTR_OWNERGROUP));
        }
        Request->Token = File;
      }
    }

    return Result;
  }

  virtual bool SendRequest()
  {
    bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool End(TSFTPPacket * /*Response*/)
  {
    return (FRequests->GetCount() == 0);
  }

private:
  TStrings * FFileList;
  intptr_t FIndex;
};
//---------------------------------------------------------------------------
class TSFTPCalculateFilesChecksumQueue : public TSFTPFixedLenQueue
{
NB_DISABLE_COPY(TSFTPCalculateFilesChecksumQueue)
public:
  explicit TSFTPCalculateFilesChecksumQueue(TSFTPFileSystem * AFileSystem, uintptr_t CodePage) :
    TSFTPFixedLenQueue(AFileSystem, CodePage),
    FIndex(0),
    FFileList(nullptr)
  {
  }
  virtual ~TSFTPCalculateFilesChecksumQueue() {}

  bool Init(intptr_t QueueLen, const UnicodeString & Alg, TStrings * FileList)
  {
    FAlg = Alg;
    FFileList = FileList;

    return TSFTPFixedLenQueue::Init(QueueLen);
  }

  bool ReceivePacket(TSFTPPacket * Packet, TRemoteFile *& File)
  {
    void * Token = nullptr;
    bool Result = false;
    SCOPE_EXIT
    {
      File = static_cast<TRemoteFile *>(Token);
    };
    {
      Result = TSFTPFixedLenQueue::ReceivePacket(Packet, SSH_FXP_EXTENDED_REPLY, asNo, &Token);
    }
    return Result;
  }

protected:
  virtual bool InitRequest(TSFTPQueuePacket * Request)
  {
    bool Result = false;
    while (!Result && (FIndex < FFileList->GetCount()))
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->GetObject(FIndex));
      assert(File != nullptr);
      ++FIndex;

      Result = !File->GetIsDirectory();
      if (Result)
      {
        assert(!File->GetIsParentDirectory() && !File->GetIsThisDirectory());

        Request->ChangeType(SSH_FXP_EXTENDED);
        Request->AddString(SFTP_EXT_CHECK_FILE_NAME);
        Request->AddPathString(FFileSystem->LocalCanonify(File->GetFullFileName()),
          FFileSystem->FUtfStrings);
        Request->AddString(FAlg);
        Request->AddInt64(0); // offset
        Request->AddInt64(0); // length (0 = till end)
        Request->AddCardinal(0); // block size (0 = no blocks or "one block")

        Request->Token = File;
      }
    }

    return Result;
  }

  virtual bool SendRequest()
  {
    bool Result =
      (FIndex < FFileList->GetCount()) &&
      TSFTPFixedLenQueue::SendRequest();
    return Result;
  }

  virtual bool End(TSFTPPacket * /*Response*/)
  {
    return (FRequests->GetCount() == 0);
  }

private:
  UnicodeString FAlg;
  TStrings * FFileList;
  int FIndex;
};
//---------------------------------------------------------------------------
#pragma warn .inl
//---------------------------------------------------------------------------
class TSFTPBusy : public TObject
{
NB_DISABLE_COPY(TSFTPBusy)
public:
  explicit TSFTPBusy(TSFTPFileSystem * FileSystem) :
    FFileSystem(FileSystem)
  {
    assert(FFileSystem != nullptr);
    FFileSystem->BusyStart();
  }

  ~TSFTPBusy()
  {
    FFileSystem->BusyEnd();
  }

private:
  TSFTPFileSystem * FFileSystem;
};
//===========================================================================
#if defined(__BORLANDC__)
struct TOpenRemoteFileParams
{
  uintptr_t LocalFileAttrs;
  UnicodeString RemoteFileName;
  TFileOperationProgressType * OperationProgress;
  const TCopyParamType * CopyParam;
  intptr_t Params;
  bool Resume;
  bool Resuming;
  TOverwriteMode OverwriteMode;
  __int64 DestFileSize; // output
  RawByteString RemoteFileHandle; // output
  TOverwriteFileParams * FileParams;
  bool Confirmed;
};
//---------------------------------------------------------------------------
struct TSinkFileParams : public TObject
{
  UnicodeString TargetDir;
  const TCopyParamType * CopyParam;
  intptr_t Params;
  TFileOperationProgressType * OperationProgress;
  bool Skipped;
  uintptr_t Flags;
};
#endif
//===========================================================================
TSFTPFileSystem::TSFTPFileSystem(TTerminal * ATerminal) :
  TCustomFileSystem(ATerminal)
{
  FCodePage = GetSessionData()->GetCodePageAsNumber();
}

void TSFTPFileSystem::Init(void * Data)
{
  FSecureShell = reinterpret_cast<TSecureShell *>(Data);
  assert(FSecureShell);
  FFileSystemInfoValid = false;
  FVersion = NPOS;
  FPacketReservations = new TList();
#if defined(__BORLANDC__)
  FPacketNumbers = VarArrayCreate(OPENARRAY(int, (0, 1)), varLongWord);
#else
  FPacketNumbers.clear();
#endif
  FPreviousLoggedPacket = 0;
  FNotLoggedPackets = 0;
  FBusy = 0;
  FAvoidBusy = false;
  FUtfStrings = false;
  FSignedTS = false;
  FSupport = new TSFTPSupport();
  FExtensions = new TStringList();
  FFixedPaths = nullptr;
  FFileSystemInfoValid = false;
}
//---------------------------------------------------------------------------
TSFTPFileSystem::~TSFTPFileSystem()
{
  SAFE_DESTROY(FSupport);
  ResetConnection();
  SAFE_DESTROY(FPacketReservations);
  SAFE_DESTROY(FExtensions);
  SAFE_DESTROY(FFixedPaths);
  SAFE_DESTROY(FSecureShell);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::Open()
{
  // this is used for reconnects only
  ResetConnection();
  FSecureShell->Open();
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::Close()
{
  FSecureShell->Close();
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CollectUsage()
{
  FSecureShell->CollectUsage();

  UnicodeString VersionCounter;
  switch (FVersion)
  {
    case 0:
      VersionCounter = L"OpenedSessionsSFTP0";
      break;
    case 1:
      VersionCounter = L"OpenedSessionsSFTP1";
      break;
    case 2:
      VersionCounter = L"OpenedSessionsSFTP2";
      break;
    case 3:
      VersionCounter = L"OpenedSessionsSFTP3";
      break;
    case 4:
      VersionCounter = L"OpenedSessionsSFTP4";
      break;
    case 5:
      VersionCounter = L"OpenedSessionsSFTP5";
      break;
  }
//  FTerminal->Configuration->Usage->Inc(VersionCounter);
}
//---------------------------------------------------------------------------
const TSessionInfo & TSFTPFileSystem::GetSessionInfo() const
{
  return FSecureShell->GetSessionInfo();
}
//---------------------------------------------------------------------------
const TFileSystemInfo & TSFTPFileSystem::GetFileSystemInfo(bool /*Retrieve*/)
{
  if (!FFileSystemInfoValid)
  {
    FFileSystemInfo.AdditionalInfo = L"";

    if (!IsCapable(fcRename))
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(FS_RENAME_NOT_SUPPORTED) + L"\r\n\r\n";
    }

    if (FExtensions->GetCount() > 0)
    {
      UnicodeString Name;
      UnicodeString Value;
      UnicodeString Line;
      FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_EXTENSION_INFO) + L"\r\n";
      for (intptr_t Index = 0; Index < FExtensions->GetCount(); ++Index)
      {
        UnicodeString Name = FExtensions->GetName(Index);
        UnicodeString Value = FExtensions->GetValue(Name);
        UnicodeString Line;
        if (Value.IsEmpty())
        {
          Line = Name;
        }
        else
        {
          Line = FORMAT(L"%s=%s", Name.c_str(), DisplayableStr(Value).c_str());
        }
        FFileSystemInfo.AdditionalInfo += FORMAT(L"  %s\r\n", Line.c_str());
      }
    }
    else
    {
      FFileSystemInfo.AdditionalInfo += LoadStr(SFTP_NO_EXTENSION_INFO) + L"\r\n";
    }

    FFileSystemInfo.ProtocolBaseName = L"SFTP";
    FFileSystemInfo.ProtocolName = FMTLOAD(SFTP_PROTOCOL_NAME2, FVersion);
    for (intptr_t Index = 0; Index < fcCount; ++Index)
    {
      FFileSystemInfo.IsCapable[Index] = IsCapable(Index);
    }

    FFileSystemInfoValid = true;
  }

  return FFileSystemInfo;
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::TemporaryTransferFile(const UnicodeString & FileName)
{
  return AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::GetStoredCredentialsTried()
{
  return FSecureShell->GetStoredCredentialsTried();
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::GetUserName()
{
  return FSecureShell->GetUserName();
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::Idle()
{
  // Keep session alive
  if ((GetSessionData()->GetPingType() != ptOff) &&
      ((Now() - FSecureShell->GetLastDataSent()) > GetSessionData()->GetPingIntervalDT()))
  {
    if ((GetSessionData()->GetPingType() == ptDummyCommand) &&
        FSecureShell->GetReady())
    {
      TSFTPPacket Packet(SSH_FXP_REALPATH, FCodePage);
      Packet.AddPathString(L"/", FUtfStrings);
      SendPacketAndReceiveResponse(&Packet, &Packet);
    }
    else
    {
      FSecureShell->KeepAlive();
    }
  }

  FSecureShell->Idle();
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ResetConnection()
{
  // there must be no valid packet reservation at the end
  for (intptr_t I = 0; I < FPacketReservations->GetCount(); I++)
  {
    assert(FPacketReservations->GetItem(I) == nullptr);
    TSFTPPacket * Item = static_cast<TSFTPPacket *>(FPacketReservations->GetItem(I));
    SAFE_DESTROY(Item);
  }
  FPacketReservations->Clear();
  FPacketNumbers.clear();
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::IsCapable(intptr_t Capability) const
{
  assert(FTerminal);
  switch (Capability)
  {
    case fcAnyCommand:
    case fcShellAnyCommand:
    case fcHardLink:
    case fcRemoteCopy:
      return false;

    case fcNewerOnlyUpload:
    case fcTimestampChanging:
    case fcIgnorePermErrors:
    case fcPreservingTimestampUpload:
    case fcSecondaryShell:
    case fcRemoveCtrlZUpload:
    case fcRemoveBOMUpload:
      return true;

    case fcRename:
    case fcRemoteMove:
      return (FVersion >= 2);

    case fcSymbolicLink:
    case fcResolveSymlink:
      return (FVersion >= 3);

    case fcModeChanging:
    case fcModeChangingUpload:
      return !FSupport->Loaded ||
        FLAGSET(FSupport->AttributeMask, SSH_FILEXFER_ATTR_PERMISSIONS);

    case fcGroupOwnerChangingByID:
      return (FVersion <= 3);

    case fcOwnerChanging:
    case fcGroupChanging:
      return
        (FVersion <= 3) ||
        ((FVersion >= 4) &&
         (!FSupport->Loaded ||
          FLAGSET(FSupport->AttributeMask, SSH_FILEXFER_ATTR_OWNERGROUP)));

    case fcNativeTextMode:
      return (FVersion >= 4);

    case fcTextMode:
      return (FVersion >= 4) ||
        strcmp(GetEOL(), EOLToStr(FTerminal->GetConfiguration()->GetLocalEOLType())) != 0;

    case fcUserGroupListing:
      return SupportsExtension(SFTP_EXT_OWNER_GROUP);

    case fcLoadingAdditionalProperties:
      // we allow loading properties only, if "supported" extension is supported and
      // the server support "permissions" and/or "owner/group" attributes
      // (no other attributes are loaded)
      return FSupport->Loaded &&
        ((FSupport->AttributeMask &
          (SSH_FILEXFER_ATTR_PERMISSIONS | SSH_FILEXFER_ATTR_OWNERGROUP)) != 0);

    case fcCheckingSpaceAvailable:
      return
        // extension announced in estension list of by
        // SFTP_EXT_SUPPORTED/SFTP_EXT_SUPPORTED2 extension
        // (SFTP version 5 and newer only)
        SupportsExtension(SFTP_EXT_SPACE_AVAILABLE) ||
        // extension announced by proprietary SFTP_EXT_STATVFS extension
        FSupportsStatVfsV2;

    case fcCalculatingChecksum:
      return SupportsExtension(SFTP_EXT_CHECK_FILE);

    default:
      assert(false);
      return false;
  }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::SupportsExtension(const UnicodeString & Extension) const
{
  return FSupport->Loaded && (FSupport->Extensions->IndexOf(Extension.c_str()) >= 0);
}
//---------------------------------------------------------------------------
inline void TSFTPFileSystem::BusyStart()
{
  if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
  {
    FBusyToken = ::BusyStart();
  }
  FBusy++;
  assert(FBusy < 10);
}
//---------------------------------------------------------------------------
inline void TSFTPFileSystem::BusyEnd()
{
  assert(FBusy > 0);
  FBusy--;
  if (FBusy == 0 && FTerminal->GetUseBusyCursor() && !FAvoidBusy)
  {
    ::BusyEnd(FBusyToken);
    FBusyToken = NULL;
  }
}
//---------------------------------------------------------------------------
uint32_t TSFTPFileSystem::TransferBlockSize(uint32_t Overhead,
  TFileOperationProgressType * OperationProgress,
  uint32_t MinPacketSize,
  uint32_t MaxPacketSize)
{
  const uint32_t minPacketSize = MinPacketSize ? MinPacketSize : 4096;

  // size + message number + type
  const uint32_t SFTPPacketOverhead = 4 + 4 + 1;
  uint32_t AMinPacketSize = FSecureShell->MinPacketSize();
  uint32_t AMaxPacketSize = FSecureShell->MaxPacketSize();
  bool MaxPacketSizeValid = (AMaxPacketSize > 0);
  uint32_t Result = static_cast<uint32_t>(OperationProgress->CPS());

  if ((MaxPacketSize > 0) &&
      ((MaxPacketSize < AMaxPacketSize) || !MaxPacketSizeValid))
  {
    AMaxPacketSize = MaxPacketSize;
    MaxPacketSizeValid = true;
  }

  if ((FMaxPacketSize > 0) &&
      ((FMaxPacketSize < AMaxPacketSize) || !MaxPacketSizeValid))
  {
    AMaxPacketSize = FMaxPacketSize;
    MaxPacketSizeValid = true;
  }

  if (Result == 0)
  {
    Result = static_cast<uint32_t>(OperationProgress->StaticBlockSize());
  }

  if (Result < minPacketSize)
  {
    Result = minPacketSize;
  }

  if (MaxPacketSizeValid)
  {
    Overhead += SFTPPacketOverhead;
    if (AMaxPacketSize < Overhead)
    {
      // do not send another request
      // (generally should happen only if upload buffer if full)
      Result = 0;
    }
    else
    {
      AMaxPacketSize -= Overhead;
      if (Result > AMaxPacketSize)
      {
        Result = AMaxPacketSize;
      }
    }
  }

  Result = static_cast<uint32_t>(OperationProgress->AdjustToCPSLimit(Result));

  return Result;
}
//---------------------------------------------------------------------------
uint32_t TSFTPFileSystem::UploadBlockSize(const RawByteString & Handle,
  TFileOperationProgressType * OperationProgress)
{
  // handle length + offset + data size
  const uintptr_t UploadPacketOverhead =
    sizeof(uint32_t) + sizeof(__int64) + sizeof(uint32_t);
  return TransferBlockSize(UploadPacketOverhead + static_cast<uint32_t>(Handle.Length()), OperationProgress,
    static_cast<uint32_t>(GetSessionData()->GetSFTPMinPacketSize()),
    static_cast<uint32_t>(GetSessionData()->GetSFTPMaxPacketSize()));
}
//---------------------------------------------------------------------------
uint32_t TSFTPFileSystem::DownloadBlockSize(
  TFileOperationProgressType * OperationProgress)
{
  uint32_t Result = TransferBlockSize(sizeof(uint32_t), OperationProgress,
    static_cast<uint32_t>(GetSessionData()->GetSFTPMinPacketSize()),
    static_cast<uint32_t>(GetSessionData()->GetSFTPMaxPacketSize()));
  if (FSupport->Loaded && (FSupport->MaxReadSize > 0) &&
      (Result > FSupport->MaxReadSize))
  {
    Result = FSupport->MaxReadSize;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SendPacket(const TSFTPPacket * Packet)
{
  BusyStart();
  SCOPE_EXIT
  {
    this->BusyEnd();
  };
  {
    if (FTerminal->GetLog()->GetLogging())
    {
      if ((FPreviousLoggedPacket != SSH_FXP_READ &&
           FPreviousLoggedPacket != SSH_FXP_WRITE) ||
          (Packet->GetType() != FPreviousLoggedPacket) ||
          (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
      {
        if (FNotLoggedPackets)
        {
          FTerminal->LogEvent(FORMAT(L"%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
            FNotLoggedPackets));
          FNotLoggedPackets = 0;
        }
        FTerminal->GetLog()->Add(llInput, FORMAT(L"Type: %s, Size: %d, Number: %d",
          Packet->GetTypeName().c_str(),
          static_cast<int>(Packet->GetLength()),
          static_cast<int>(Packet->GetMessageNumber())));
        /*if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2)
        {
          FTerminal->GetLog()->Add(llInput, Packet->Dump());
        }*/
        FPreviousLoggedPacket = Packet->GetType();
      }
      else
      {
        FNotLoggedPackets++;
      }
    }
    FSecureShell->Send(Packet->GetSendData(), Packet->GetSendLength());
  }
}
//---------------------------------------------------------------------------
uintptr_t TSFTPFileSystem::GotStatusPacket(TSFTPPacket * Packet,
  int AllowStatus)
{
  uintptr_t Code = Packet->GetCardinal();

  static int Messages[] = {
    SFTP_STATUS_OK,
    SFTP_STATUS_EOF,
    SFTP_STATUS_NO_SUCH_FILE,
    SFTP_STATUS_PERMISSION_DENIED,
    SFTP_STATUS_FAILURE,
    SFTP_STATUS_BAD_MESSAGE,
    SFTP_STATUS_NO_CONNECTION,
    SFTP_STATUS_CONNECTION_LOST,
    SFTP_STATUS_OP_UNSUPPORTED,
    SFTP_STATUS_INVALID_HANDLE,
    SFTP_STATUS_NO_SUCH_PATH,
    SFTP_STATUS_FILE_ALREADY_EXISTS,
    SFTP_STATUS_WRITE_PROTECT,
    SFTP_STATUS_NO_MEDIA,
    SFTP_STATUS_NO_SPACE_ON_FILESYSTEM,
    SFTP_STATUS_QUOTA_EXCEEDED,
    SFTP_STATUS_UNKNOWN_PRINCIPAL,
    SFTP_STATUS_LOCK_CONFLICT,
    SFTP_STATUS_DIR_NOT_EMPTY,
    SFTP_STATUS_NOT_A_DIRECTORY,
    SFTP_STATUS_INVALID_FILENAME,
    SFTP_STATUS_LINK_LOOP,
    SFTP_STATUS_CANNOT_DELETE,
    SFTP_STATUS_INVALID_PARAMETER,
    SFTP_STATUS_FILE_IS_A_DIRECTORY,
    SFTP_STATUS_BYTE_RANGE_LOCK_CONFLICT,
    SFTP_STATUS_BYTE_RANGE_LOCK_REFUSED,
    SFTP_STATUS_DELETE_PENDING,
    SFTP_STATUS_FILE_CORRUPT
  };
  if ((AllowStatus & (0x01 << Code)) == 0)
  {
    int Message;
    if (Code >= LENOF(Messages))
    {
      Message = SFTP_STATUS_UNKNOWN;
    }
    else
    {
      Message = Messages[Code];
    }
    UnicodeString MessageStr = LoadStr(Message);
    UnicodeString ServerMessage;
    UnicodeString LanguageTag;
    if ((FVersion >= 3) ||
        // if version is not decided yet (i.e. this is status response
        // to the init request), go on, only if there are any more data
        ((FVersion < 0) && (Packet->GetRemainingLength() > 0)))
    {
      // message is in UTF only since SFTP specification 01 (specification 00
      // is also version 3)
      // (in other words, always use UTF unless server is known to be buggy)
      ServerMessage = Packet->GetString(FUtfStrings);
      // SSH-2.0-Maverick_SSHD and SSH-2.0-CIGNA SFTP Server Ready! omit the language tag
      // and I believe I've seen one more server doing the same.
      if (Packet->GetRemainingLength() > 0)
      {
        LanguageTag = Packet->GetAnsiString();
        if ((FVersion >= 5) && (Message == SFTP_STATUS_UNKNOWN_PRINCIPAL))
        {
          UnicodeString Principals;
          while (Packet->GetNextData() != nullptr)
          {
            if (!Principals.IsEmpty())
            {
              Principals += L", ";
            }
            Principals += Packet->GetAnsiString();
          }
          MessageStr = FORMAT(MessageStr.c_str(), Principals.c_str());
        }
      }
    }
    else
    {
      ServerMessage = LoadStr(SFTP_SERVER_MESSAGE_UNSUPPORTED);
    }
    if (FTerminal->GetLog()->GetLogging())
    {
      FTerminal->GetLog()->Add(llOutput, FORMAT(L"Status code: %d, Message: %d, Server: %s, Language: %s ",
        static_cast<int>(Code),
        static_cast<int>(Packet->GetMessageNumber()),
        ServerMessage.c_str(),
        LanguageTag.c_str()));
    }
    if (!LanguageTag.IsEmpty())
    {
      LanguageTag = FORMAT(L" (%s)", LanguageTag.c_str());
    }
    UnicodeString HelpKeyword;
    switch (Code)
    {
      case SSH_FX_FAILURE:
        HelpKeyword = HELP_SFTP_STATUS_FAILURE;
        break;

      case SSH_FX_PERMISSION_DENIED:
        HelpKeyword = HELP_SFTP_STATUS_PERMISSION_DENIED;
        break;
    }
    UnicodeString Error = FMTLOAD(SFTP_ERROR_FORMAT3, MessageStr.c_str(),
      int(Code), LanguageTag.c_str(), ServerMessage.c_str());
    FTerminal->TerminalError(nullptr, Error, HelpKeyword);
    return 0;
  }
  else
  {
    if (!FNotLoggedPackets || Code)
    {
      FTerminal->GetLog()->Add(llOutput, FORMAT(L"Status code: %d", static_cast<int>(Code)));
    }
    return Code;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::RemoveReservation(intptr_t Reservation)
{
  for (intptr_t Index = Reservation+1; Index < FPacketReservations->GetCount(); ++Index)
  {
    FPacketNumbers[Index-1] = FPacketNumbers[Index];
  }
  TSFTPPacket * Packet = static_cast<TSFTPPacket *>(FPacketReservations->GetItem(Reservation));
  if (Packet)
  {
    assert(Packet->GetReservedBy() == this);
    Packet->SetReservedBy(nullptr);
  }
  FPacketReservations->Delete(Reservation);
}
//---------------------------------------------------------------------------
intptr_t TSFTPFileSystem::PacketLength(uint8_t * LenBuf, intptr_t ExpectedType)
{
  intptr_t Length = GET_32BIT(LenBuf);
  if (Length > SFTP_MAX_PACKET_LEN)
  {
    UnicodeString Message = FMTLOAD(SFTP_PACKET_TOO_BIG,
      int(Length), SFTP_MAX_PACKET_LEN);
    if (ExpectedType == SSH_FXP_VERSION)
    {
      RawByteString LenString(reinterpret_cast<char *>(LenBuf), 4);
      Message = FMTLOAD(SFTP_PACKET_TOO_BIG_INIT_EXPLAIN,
        Message.c_str(), DisplayableStr(LenString).c_str());
    }
    FTerminal->FatalError(nullptr, Message, HELP_SFTP_PACKET_TOO_BIG);
  }
  return Length;
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::PeekPacket()
{
  uint8_t * Buf = nullptr;
  bool Result = FSecureShell->Peek(Buf, 4);
  if (Result)
  {
    intptr_t Length = PacketLength(Buf, -1);
    Result = FSecureShell->Peek(Buf, 4 + Length);
  }
  return Result;
}
//---------------------------------------------------------------------------
uintptr_t TSFTPFileSystem::ReceivePacket(TSFTPPacket * Packet,
  int ExpectedType, int AllowStatus)
{
  TSFTPBusy Busy(this);

  uintptr_t Result = SSH_FX_OK;
  intptr_t Reservation = FPacketReservations->IndexOf(Packet);

  if ((Reservation < 0) || (Packet->GetCapacity() == 0))
  {
    bool IsReserved;
    do
    {
      IsReserved = false;

      assert(Packet);
      uint8_t LenBuf[4];
      FSecureShell->Receive(LenBuf, sizeof(LenBuf));
      intptr_t Length = PacketLength(LenBuf, ExpectedType);
      Packet->SetCapacity(Length);
      FSecureShell->Receive(Packet->GetData(), Length);
      Packet->DataUpdated(Length);

      if (FTerminal->GetLog()->GetLogging())
      {
        if ((FPreviousLoggedPacket != SSH_FXP_READ &&
             FPreviousLoggedPacket != SSH_FXP_WRITE) ||
            (Packet->GetType() != SSH_FXP_STATUS && Packet->GetType() != SSH_FXP_DATA) ||
            (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1))
        {
          if (FNotLoggedPackets)
          {
            FTerminal->LogEvent(FORMAT(L"%d skipped SSH_FXP_WRITE, SSH_FXP_READ, SSH_FXP_DATA and SSH_FXP_STATUS packets.",
              FNotLoggedPackets));
            FNotLoggedPackets = 0;
          }
          FTerminal->GetLog()->Add(llOutput, FORMAT(L"Type: %s, Size: %d, Number: %d",
            Packet->GetTypeName().c_str(),
            static_cast<int>(Packet->GetLength()),
            static_cast<int>(Packet->GetMessageNumber())));
          /*if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 2)
          {
            FTerminal->GetLog()->Add(llOutput, Packet->Dump());
          }*/
        }
        else
        {
          FNotLoggedPackets++;
        }
      }

      if ((Reservation < 0) ||
          Packet->GetMessageNumber() != FPacketNumbers[Reservation])
      {
        TSFTPPacket * ReservedPacket;
        for (intptr_t Index = 0; Index < FPacketReservations->GetCount(); ++Index)
        {
          unsigned int MessageNumber = (unsigned int)FPacketNumbers[Index];
          if (MessageNumber == Packet->GetMessageNumber())
          {
            ReservedPacket = static_cast<TSFTPPacket *>(FPacketReservations->GetItem(Index));
            IsReserved = true;
            if (ReservedPacket)
            {
              FTerminal->LogEvent(L"Storing reserved response");
              *ReservedPacket = *Packet;
            }
            else
            {
              FTerminal->LogEvent(L"Discarding reserved response");
              RemoveReservation(Index);
              if ((Reservation >= 0) && (Reservation > Index))
              {
                Reservation--;
                assert(Reservation == FPacketReservations->IndexOf(Packet));
              }
            }
            break;
          }
        }
      }
    }
    while (IsReserved);
  }

  // before we removed the reservation after check for packet type,
  // but if it raises exception, removal is unnecessarily
  // postponed until the packet is removed
  // (and it have not worked anyway until recent fix to UnreserveResponse)
  if (Reservation >= 0)
  {
    assert(Packet->GetMessageNumber() == FPacketNumbers[Reservation]);
    RemoveReservation(Reservation);
  }

  if (ExpectedType >= 0)
  {
    if (Packet->GetType() == SSH_FXP_STATUS)
    {
      if (AllowStatus < 0)
      {
        AllowStatus = (ExpectedType == SSH_FXP_STATUS ? asOK : asNo);
      }
      Result = GotStatusPacket(Packet, AllowStatus);
    }
    else if (ExpectedType != Packet->GetType())
    {
      FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_TYPE, static_cast<int>(Packet->GetType())));
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReserveResponse(const TSFTPPacket * Packet,
  TSFTPPacket * Response)
{
  if (Response != nullptr)
  {
    assert(FPacketReservations->IndexOf(Response) < 0);
    // mark response as not received yet
    Response->SetCapacity(0);
    Response->SetReservedBy(this);
  }
  FPacketReservations->Add(Response);
  if ((size_t)FPacketReservations->GetCount() >= FPacketNumbers.size())
  {
    FPacketNumbers.resize(FPacketReservations->GetCount() + 10);
  }
  FPacketNumbers[FPacketReservations->GetCount() - 1] = Packet->GetMessageNumber();
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::UnreserveResponse(TSFTPPacket * Response)
{
  intptr_t Reservation = FPacketReservations->IndexOf(Response);
  if (Response->GetCapacity() != 0)
  {
    // added check for already received packet
    // (it happens when the reserved response is received out of order,
    // unexpectedly soon, and then receivepacket() on the packet
    // is not actually called, due to exception)
    RemoveReservation(Reservation);
  }
  else
  {
    if (Reservation >= 0)
    {
      // we probably do not remove the item at all, because
      // we must remember that the response was expected, so we skip it
      // in receivepacket()
      FPacketReservations->SetItem(Reservation, nullptr);
    }
  }
}
//---------------------------------------------------------------------------
uintptr_t TSFTPFileSystem::ReceiveResponse(
  const TSFTPPacket * Packet, TSFTPPacket * AResponse, int ExpectedType,
  int AllowStatus)
{
  uintptr_t Result;
  uintptr_t MessageNumber = Packet->GetMessageNumber();
  std::unique_ptr<TSFTPPacket> Response(nullptr);
  if (!AResponse)
  {
    Response.reset(new TSFTPPacket(FCodePage));
    AResponse = Response.get();
  }
  Result = ReceivePacket(AResponse, ExpectedType, AllowStatus);
  if (MessageNumber != AResponse->GetMessageNumber())
  {
    FTerminal->FatalError(nullptr, FMTLOAD(SFTP_MESSAGE_NUMBER,
      static_cast<int>(AResponse->GetMessageNumber()),
      static_cast<int>(MessageNumber)));
  }
  return Result;
}
//---------------------------------------------------------------------------
uintptr_t TSFTPFileSystem::SendPacketAndReceiveResponse(
  const TSFTPPacket * Packet, TSFTPPacket * Response, int ExpectedType,
  int AllowStatus)
{
  uintptr_t Result;
  TSFTPBusy Busy(this);
  SendPacket(Packet);
  Result = ReceiveResponse(Packet, Response, ExpectedType, AllowStatus);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::RealPath(const UnicodeString & Path)
{
  try
  {
    FTerminal->LogEvent(FORMAT(L"Getting real path for '%s'",
      Path.c_str()));

    TSFTPPacket Packet(SSH_FXP_REALPATH, FCodePage);
    Packet.AddPathString(Path, FUtfStrings);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_NAME);
    if (Packet.GetCardinal() != 1)
    {
      FTerminal->FatalError(nullptr, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
    }

    UnicodeString RealDir = ::UnixExcludeTrailingBackslash(Packet.GetPathString(FUtfStrings));
    // ignore rest of SSH_FXP_NAME packet

    FTerminal->LogEvent(FORMAT(L"Real path is '%s'", RealDir.c_str()));

    return RealDir;
  }
  catch (Exception & E)
  {
    if (FTerminal->GetActive())
    {
      throw ExtException(&E, FMTLOAD(SFTP_REALPATH_ERROR, Path.c_str()));
    }
    else
    {
      throw;
    }
  }
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::RealPath(const UnicodeString & Path,
  const UnicodeString & BaseDir)
{
  UnicodeString APath;

  if (::UnixIsAbsolutePath(Path))
  {
    APath = Path;
  }
  else
  {
    if (!Path.IsEmpty())
    {
      // this condition/block was outside (before) current block
      // but it did not work when Path was empty
      if (!BaseDir.IsEmpty())
      {
        APath = ::UnixIncludeTrailingBackslash(BaseDir);
      }
      APath = APath + Path;
    }
    if (APath.IsEmpty())
    {
      APath = ::UnixIncludeTrailingBackslash(L".");
    }
  }
  return RealPath(APath);
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::LocalCanonify(const UnicodeString & Path)
{
  // TODO: improve (handle .. etc.)
  if (::UnixIsAbsolutePath(Path) ||
      (!FCurrentDirectory.IsEmpty() && ::UnixComparePaths(FCurrentDirectory, Path)))
  {
    return Path;
  }
  else
  {
    return ::AbsolutePath(FCurrentDirectory, Path);
  }
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::Canonify(const UnicodeString & Path)
{
  // inspired by canonify() from PSFTP.C
  UnicodeString Result;
  FTerminal->LogEvent(FORMAT(L"Canonifying: \"%s\"", Path.c_str()));
  Result = LocalCanonify(Path);
  bool TryParent = false;
  try
  {
    Result = RealPath(Result);
  }
  catch (...)
  {
    if (FTerminal->GetActive())
    {
      TryParent = true;
    }
    else
    {
      throw;
    }
  }

  if (TryParent)
  {
    UnicodeString APath = ::UnixExcludeTrailingBackslash(Result);
    UnicodeString Name = ::UnixExtractFileName(APath);
    if (Name == THISDIRECTORY || Name == PARENTDIRECTORY)
    {
      // Result = Result;
    }
    else
    {
      UnicodeString FPath = ::UnixExtractFilePath(APath);
      try
      {
        Result = RealPath(FPath);
        Result = ::UnixIncludeTrailingBackslash(Result) + Name;
      }
      catch (...)
      {
        if (FTerminal->GetActive())
        {
          // Result = Result;
        }
        else
        {
          throw;
        }
      }
    }
  }

  FTerminal->LogEvent(FORMAT(L"Canonified: \"%s\"", Result.c_str()));

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::AbsolutePath(const UnicodeString & Path, bool Local)
{
  if (Local)
  {
    return LocalCanonify(Path);
  }
  else
  {
    return RealPath(Path, GetCurrentDirectory());
  }
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::GetHomeDirectory()
{
  if (FHomeDirectory.IsEmpty())
  {
    FHomeDirectory = RealPath(THISDIRECTORY);
  }
  return FHomeDirectory;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::LoadFile(TRemoteFile * AFile, TSFTPPacket * Packet,
  bool Complete)
{
  Packet->GetFile(AFile, FVersion, GetSessionData()->GetDSTMode(),
    FUtfStrings, FSignedTS, Complete);
}
//---------------------------------------------------------------------------
TRemoteFile * TSFTPFileSystem::LoadFile(TSFTPPacket * Packet,
  TRemoteFile * ALinkedByFile, const UnicodeString & FileName,
  TRemoteFileList * TempFileList, bool Complete)
{
  std::unique_ptr<TRemoteFile> File(new TRemoteFile(ALinkedByFile));
  File->SetTerminal(FTerminal);
  if (!FileName.IsEmpty())
  {
    File->SetFileName(FileName);
  }
  // to get full path for symlink completion
  File->SetDirectory(TempFileList);
  LoadFile(File.get(), Packet, Complete);
  File->SetDirectory(nullptr);
  return File.release();
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::GetCurrentDirectory()
{
  return FCurrentDirectory;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DoStartup()
{
  // do not know yet
  FVersion = -1;
  FFileSystemInfoValid = false;
  TSFTPPacket Packet(SSH_FXP_INIT, FCodePage);
  uint32_t MaxVersion = static_cast<uint32_t>(GetSessionData()->GetSFTPMaxVersion());
  if (MaxVersion > SFTPMaxVersion)
  {
    MaxVersion = SFTPMaxVersion;
  }
  Packet.AddCardinal(MaxVersion);

  try
  {
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_VERSION);
  }
  catch (Exception &E)
  {
    FTerminal->FatalError(&E, LoadStr(SFTP_INITIALIZE_ERROR));
  }

  FVersion = Packet.GetCardinal();
  FTerminal->LogEvent(FORMAT(L"SFTP version %d negotiated.", FVersion));
  if (FVersion < SFTPMinVersion || FVersion > SFTPMaxVersion)
  {
    FTerminal->FatalError(nullptr, FMTLOAD(SFTP_VERSION_NOT_SUPPORTED,
      FVersion, SFTPMinVersion, SFTPMaxVersion));
  }

  FExtensions->Clear();
  FEOL = "\r\n";
  FSupport->Loaded = false;
  FSupportsStatVfsV2 = false;
  SAFE_DESTROY(FFixedPaths);

  if (FVersion >= 3)
  {
    while (Packet.GetNextData() != nullptr)
    {
      UnicodeString ExtensionName = Packet.GetAnsiString();
      RawByteString ExtensionData = Packet.GetRawByteString();
      UnicodeString ExtensionDisplayData = DisplayableStr(ExtensionData);

      if (ExtensionName == SFTP_EXT_NEWLINE)
      {
        FEOL = AnsiString(ExtensionData.c_str());
        FTerminal->LogEvent(FORMAT(L"Server requests EOL sequence %s.",
          ExtensionDisplayData.c_str()));
        if (FEOL.Length() < 1 || FEOL.Length() > 2)
        {
          FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_EOL, ExtensionDisplayData.c_str()));
        }
      }
      // do not allow "supported" to override "supported2" if both are received
      else if (((ExtensionName == SFTP_EXT_SUPPORTED) && !FSupport->Loaded) ||
               (ExtensionName == SFTP_EXT_SUPPORTED2))
      {
        FSupport->Reset();
        TSFTPPacket SupportedStruct(ExtensionData, FCodePage);
        FSupport->Loaded = true;
        FSupport->AttributeMask = SupportedStruct.GetCardinal();
        FSupport->AttributeBits = SupportedStruct.GetCardinal();
        FSupport->OpenFlags = SupportedStruct.GetCardinal();
        FSupport->AccessMask = SupportedStruct.GetCardinal();
        FSupport->MaxReadSize = SupportedStruct.GetCardinal();
        if (ExtensionName == SFTP_EXT_SUPPORTED)
        {
          while (SupportedStruct.GetNextData() != nullptr)
          {
            FSupport->Extensions->Add(SupportedStruct.GetAnsiString());
          }
        }
        else
        {
          FSupport->OpenBlockMasks = SupportedStruct.GetSmallCardinal();
          FSupport->BlockMasks = SupportedStruct.GetSmallCardinal();
          uintptr_t ExtensionCount;
          ExtensionCount = SupportedStruct.GetCardinal();
          for (uintptr_t I = 0; I < ExtensionCount; I++)
          {
            FSupport->AttribExtensions->Add(SupportedStruct.GetAnsiString());
          }
          ExtensionCount = SupportedStruct.GetCardinal();
          for (uintptr_t I = 0; I < ExtensionCount; I++)
          {
            FSupport->Extensions->Add(SupportedStruct.GetAnsiString());
          }
        }

        if (FTerminal->GetLog()->GetLogging())
        {
          FTerminal->LogEvent(FORMAT(
            L"Server support information:\n"
            L"  Attribute mask: %x, Attribute bits: %x, Open flags: %x\n"
            L"  Access mask: %x, Open block masks: %x, Block masks: %x, Max read size: %d\n",
            int(FSupport->AttributeMask),
             int(FSupport->AttributeBits),
             int(FSupport->OpenFlags),
             int(FSupport->AccessMask),
             int(FSupport->OpenBlockMasks),
             int(FSupport->BlockMasks),
             int(FSupport->MaxReadSize)));
          FTerminal->LogEvent(FORMAT(L"  Attribute extensions (%d)\n", FSupport->AttribExtensions->GetCount()));
          for (intptr_t Index = 0; Index < FSupport->AttribExtensions->GetCount(); ++Index)
          {
            FTerminal->LogEvent(
              FORMAT(L"    %s", FSupport->AttribExtensions->GetString(Index).c_str()));
          }
          FTerminal->LogEvent(FORMAT(L"  Extensions (%d)\n", FSupport->Extensions->GetCount()));
          for (intptr_t Index = 0; Index < FSupport->Extensions->GetCount(); ++Index)
          {
            FTerminal->LogEvent(
              FORMAT(L"    %s", FSupport->Extensions->GetString(Index).c_str()));
          }
        }
      }
      else if (ExtensionName == SFTP_EXT_VENDOR_ID)
      {
        TSFTPPacket VendorIdStruct(ExtensionData, FCodePage);
        UnicodeString VendorName(VendorIdStruct.GetAnsiString());
        UnicodeString ProductName(VendorIdStruct.GetAnsiString());
        UnicodeString ProductVersion(VendorIdStruct.GetAnsiString());
        __int64 ProductBuildNumber = VendorIdStruct.GetInt64();
        FTerminal->LogEvent(FORMAT(L"Server software: %s %s (%d) by %s",
          ProductName.c_str(), ProductVersion.c_str(), int(ProductBuildNumber), VendorName.c_str()));
      }
      else if (ExtensionName == SFTP_EXT_FSROOTS)
      {
        FTerminal->LogEvent(L"File system roots:\n");
        assert(FFixedPaths == nullptr);
        FFixedPaths = new TStringList();
        try
        {
          TSFTPPacket RootsPacket(ExtensionData, FCodePage);
          while (RootsPacket.GetNextData() != nullptr)
          {
            uintptr_t Dummy = RootsPacket.GetCardinal();
            if (Dummy != 1)
            {
              break;
            }
            else
            {
              uint8_t Drive = RootsPacket.GetByte();
              uint8_t MaybeType = RootsPacket.GetByte();
              FTerminal->LogEvent(FORMAT(L"  %c: (type %d)", static_cast<char>(Drive), static_cast<int>(MaybeType)));
              FFixedPaths->Add(FORMAT(L"%c:", static_cast<char>(Drive)));
            }
          }
        }
        catch (Exception & E)
        {
          DEBUG_PRINTF(L"before FTerminal->HandleException");
          FFixedPaths->Clear();
          FTerminal->LogEvent(FORMAT(L"Failed to decode %s extension",
            SFTP_EXT_FSROOTS));
          FTerminal->HandleException(&E);
        }
      }
      else if (ExtensionName == SFTP_EXT_VERSIONS)
      {
        try
        {
          // first try legacy decoding according to incorrect encoding
          // (structure-like) as of VShell.
          TSFTPPacket VersionsPacket(ExtensionData, FCodePage);
          UnicodeString Versions = VersionsPacket.GetAnsiString();
          if (VersionsPacket.GetNextData() != nullptr)
          {
            Abort();
          }
          FTerminal->LogEvent(FORMAT(L"SFTP versions supported by the server (VShell format): %s",
            Versions.c_str()));
        }
        catch (...)
        {
          // if that fails, fallback to proper decoding
          FTerminal->LogEvent(FORMAT(L"SFTP versions supported by the server: %s",
            AnsiString(ExtensionData.c_str()).c_str()));
        }
      }
      else if (ExtensionName == SFTP_EXT_STATVFS)
      {
        UnicodeString StatVfsVersion = AnsiString(ExtensionData.c_str());
        if (StatVfsVersion == SFTP_EXT_STATVFS_VALUE_V2)
        {
          FSupportsStatVfsV2 = true;
          FTerminal->LogEvent(FORMAT(L"Supports %s extension version %s", ExtensionName.c_str(), ExtensionDisplayData.c_str()));
        }
        else
        {
          FTerminal->LogEvent(FORMAT(L"Unsupported %s extension version %s", ExtensionName.c_str(), ExtensionDisplayData.c_str()));
        }
      }
      else
      {
        FTerminal->LogEvent(FORMAT(L"Unknown server extension %s=%s",
          ExtensionName.c_str(), ExtensionDisplayData.c_str()));
      }
      FExtensions->SetValue(ExtensionName, ExtensionDisplayData);
    }

    if (SupportsExtension(SFTP_EXT_VENDOR_ID))
    {
      const TConfiguration * Configuration = FTerminal->GetConfiguration();
      TSFTPPacket Packet((uint8_t)SSH_FXP_EXTENDED, FCodePage);
      Packet.AddString(RawByteString(SFTP_EXT_VENDOR_ID));
      Packet.AddString(Configuration->GetCompanyName());
      Packet.AddString(Configuration->GetProductName());
      Packet.AddString(Configuration->GetProductVersion());
      Packet.AddInt64(LOWORD(FTerminal->GetConfiguration()->GetFixedApplicationInfo()->dwFileVersionLS));
      SendPacket(&Packet);
      // we are not interested in the response, do not wait for it
      ReserveResponse(&Packet, nullptr);
    }
  }

  if (FVersion < 4)
  {
    // currently enable the bug for all servers (really known on OpenSSH)
    FSignedTS = (GetSessionData()->GetSFTPBug(sbSignedTS) == asOn) ||
      (GetSessionData()->GetSFTPBug(sbSignedTS) == asAuto);
    if (FSignedTS)
    {
      FTerminal->LogEvent(L"We believe the server has signed timestamps bug");
    }
  }
  else
  {
    FSignedTS = false;
  }

  // use UTF when forced or ...
  // when "auto" and the server is not known not to use UTF
  const TSessionInfo & Info = GetSessionInfo();
  bool BuggyUtf = Info.SshImplementation.Pos(L"Foxit-WAC-Server") == 1;
  FUtfStrings =
    (GetSessionData()->GetNotUtf() == asOff) ||
    ((GetSessionData()->GetNotUtf() == asAuto) && !BuggyUtf);

  if (FUtfStrings)
  {
    FTerminal->LogEvent(L"We will use UTF-8 strings when appropriate");
  }
  else
  {
    FTerminal->LogEvent(L"We will never use UTF-8 strings");
  }

  FMaxPacketSize = static_cast<uint32_t>(GetSessionData()->GetSFTPMaxPacketSize());
  if (FMaxPacketSize == 0)
  {
    if (FSecureShell->IsOpenSSH() && (FVersion == 3) && !FSupport->Loaded)
    {
      FMaxPacketSize = 4 + (256 * 1024); // len + 256kB payload
      FTerminal->LogEvent(FORMAT(L"Limiting packet size to OpenSSH sftp-server limit of %d bytes",
        int(FMaxPacketSize)));
    }
    // full string is "1.77 sshlib: Momentum SSH Server",
    // possibly it is sshlib-related
    else if (Info.SshImplementation.Pos(L"Momentum SSH Server") != 0)
    {
      FMaxPacketSize = 4 + (32 * 1024);
      FTerminal->LogEvent(FORMAT(L"Limiting packet size to Momentum sftp-server limit of %d bytes",
        int(FMaxPacketSize)));
    }
  }
}
//---------------------------------------------------------------------------
char * TSFTPFileSystem::GetEOL() const
{
  if (FVersion >= 4)
  {
    assert(!FEOL.IsEmpty());
    return const_cast<char *>(FEOL.c_str());
  }
  else
  {
    return EOLToStr(GetSessionData()->GetEOLType());
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::LookupUsersGroups()
{
  assert(SupportsExtension(SFTP_EXT_OWNER_GROUP));

  TSFTPPacket PacketOwners((uint8_t)SSH_FXP_EXTENDED, FCodePage);
  TSFTPPacket PacketGroups((uint8_t)SSH_FXP_EXTENDED, FCodePage);

  TSFTPPacket * Packets[] = { &PacketOwners, &PacketGroups };
  TRemoteTokenList * Lists[] = { &FTerminal->FUsers, &FTerminal->FGroups };
  wchar_t ListTypes[] = { OGQ_LIST_OWNERS, OGQ_LIST_GROUPS };

  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(Packets)); ++Index)
  {
    TSFTPPacket * Packet = Packets[Index];
    Packet->AddString(RawByteString(SFTP_EXT_OWNER_GROUP));
    Packet->AddByte((uint8_t)ListTypes[Index]);
    SendPacket(Packet);
    ReserveResponse(Packet, Packet);
  }

  for (intptr_t Index = 0; Index < static_cast<intptr_t>(LENOF(Packets)); ++Index)
  {
    TSFTPPacket * Packet = Packets[Index];

    ReceiveResponse(Packet, Packet, SSH_FXP_EXTENDED_REPLY, asOpUnsupported);

    if ((Packet->GetType() != SSH_FXP_EXTENDED_REPLY) ||
        (Packet->GetAnsiString() != SFTP_EXT_OWNER_GROUP_REPLY))
    {
      FTerminal->LogEvent(FORMAT(L"Invalid response to %s", SFTP_EXT_OWNER_GROUP));
    }
    else
    {
      TRemoteTokenList & List = *Lists[Index];
      uintptr_t Count = Packet->GetCardinal();

      List.Clear();
      for (uintptr_t Item = 0; Item < Count; Item++)
      {
        TRemoteToken Token(Packet->GetString(FUtfStrings));
        List.Add(Token);
        if (&List == &FTerminal->FGroups)
        {
          FTerminal->FMembership.Add(Token);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadCurrentDirectory()
{
  if (!FDirectoryToChangeTo.IsEmpty())
  {
    FCurrentDirectory = FDirectoryToChangeTo;
    FDirectoryToChangeTo = L"";
  }
  else if (FCurrentDirectory.IsEmpty())
  {
    // this happens only after startup when default remote directory is not specified
    FCurrentDirectory = GetHomeDirectory();
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::HomeDirectory()
{
  ChangeDirectory(GetHomeDirectory());
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::TryOpenDirectory(const UnicodeString & Directory)
{
  FTerminal->LogEvent(FORMAT(L"Trying to open directory \"%s\".", Directory.c_str()));
  TRemoteFile * File;
  CustomReadFile(Directory, File, SSH_FXP_LSTAT, nullptr, asOpUnsupported);
  if (File == nullptr)
  {
    // File can be nullptr only when server does not support SSH_FXP_LSTAT.
    // Fallback to legacy solution, which in turn does not allow entering
    // traverse-only (chmod 110) directories.
    // This is workaround for http://www.ftpshell.com/
    TSFTPPacket Packet(SSH_FXP_OPENDIR, FCodePage);
    Packet.AddPathString(UnixExcludeTrailingBackslash(Directory), FUtfStrings);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);
    RawByteString Handle = Packet.GetFileHandle();
    Packet.ChangeType(SSH_FXP_CLOSE);
    Packet.AddString(Handle);
    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS, asAll);
  }
  else
  {
    SAFE_DESTROY(File);
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::AnnounceFileListOperation()
{
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ChangeDirectory(const UnicodeString & Directory)
{
  UnicodeString Path, Current;

  Current = !FDirectoryToChangeTo.IsEmpty() ? FDirectoryToChangeTo : FCurrentDirectory;
  Path = RealPath(Directory, Current);

  // to verify existence of directory try to open it (SSH_FXP_REALPATH succeeds
  // for invalid paths on some systems, like CygWin)
  TryOpenDirectory(Path);

  // if open dir did not fail, directory exists -> success.
  FDirectoryToChangeTo = Path;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CachedChangeDirectory(const UnicodeString & Directory)
{
  FDirectoryToChangeTo = ::UnixExcludeTrailingBackslash(Directory);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadDirectory(TRemoteFileList * FileList)
{
  assert(FileList && !FileList->GetDirectory().IsEmpty());

  UnicodeString Directory;
  Directory = ::UnixExcludeTrailingBackslash(LocalCanonify(FileList->GetDirectory()));
  FTerminal->LogEvent(FORMAT(L"Listing directory \"%s\".", Directory.c_str()));

  // moved before SSH_FXP_OPENDIR, so directory listing does not retain
  // old data (e.g. parent directory) when reading fails
  FileList->Reset();

  TSFTPPacket Packet(SSH_FXP_OPENDIR, FCodePage);
  RawByteString Handle;

  try
  {
    Packet.AddPathString(Directory, FUtfStrings);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

    Handle = Packet.GetFileHandle();
  }
  catch (...)
  {
    if (FTerminal->GetActive())
    {
      FileList->AddFile(new TRemoteParentDirectory(FTerminal));
    }
    throw;
  }

  TSFTPPacket Response(FCodePage);
  SCOPE_EXIT
  {
    if (FTerminal->GetActive())
    {
      Packet.ChangeType(SSH_FXP_CLOSE);
      Packet.AddString(Handle);
      SendPacket(&Packet);
      // we are not interested in the response, do not wait for it
      ReserveResponse(&Packet, nullptr);
    }
  };
  {
    bool isEOF = false;
    int Total = 0;
    TRemoteFile * File;

    Packet.ChangeType(SSH_FXP_READDIR);
    Packet.AddString(Handle);

    SendPacket(&Packet);

    do
    {
      ReceiveResponse(&Packet, &Response);
      if (Response.GetType() == SSH_FXP_NAME)
      {
        TSFTPPacket ListingPacket(Response, FCodePage);

        Packet.ChangeType(SSH_FXP_READDIR);
        Packet.AddString(Handle);

        SendPacket(&Packet);
        ReserveResponse(&Packet, &Response);

        uintptr_t Count = ListingPacket.GetCardinal();

        for (intptr_t Index = 0; !isEOF && (Index < static_cast<intptr_t>(Count)); ++Index)
        {
          File = LoadFile(&ListingPacket, nullptr, L"", FileList);
          if (FTerminal->GetConfiguration()->GetActualLogProtocol() >= 1)
          {
            FTerminal->LogEvent(FORMAT(L"Read file '%s' from listing", File->GetFileName().c_str()));
          }
          FileList->AddFile(File);
          Total++;

          if (Total % 10 == 0)
          {
            FTerminal->DoReadDirectoryProgress(Total, isEOF);
          }
          if (isEOF)
          {
            FTerminal->DoReadDirectoryProgress(-2, isEOF);
          }
        }

        if (Count == 0)
        {
          FTerminal->LogEvent(L"Empty directory listing packet. Aborting directory reading.");
          isEOF = true;
        }
      }
      else if (Response.GetType() == SSH_FXP_STATUS)
      {
        isEOF = (GotStatusPacket(&Response, asEOF) == SSH_FX_EOF);
      }
      else
      {
        FTerminal->FatalError(nullptr, FMTLOAD(SFTP_INVALID_TYPE, static_cast<int>(Response.GetType())));
      }
    }
    while (!isEOF);

    if (Total == 0)
    {
      bool Failure = false;
      // no point reading parent of root directory,
      // moreover CompleteFTP terminates session upon attempt to do so
      if (IsUnixRootPath(FileList->GetDirectory()))
      {
        File = nullptr;
      }
      else
      {
        // Empty file list -> probably "permission denied", we
        // at least get link to parent directory ("..")
        try
        {
          FTerminal->SetExceptionOnFail(true);
          SCOPE_EXIT
          {
            FTerminal->SetExceptionOnFail(false);
          };
          {
            File = nullptr;
            FTerminal->ReadFile(
              ::UnixIncludeTrailingBackslash(FileList->GetDirectory()) + PARENTDIRECTORY, File);
          }
        }
        catch (Exception &E)
        {
          if (NB_STATIC_DOWNCAST(EFatal, static_cast<TObject *>(&E)) != nullptr)
          {
            throw;
          }
          else
          {
            File = nullptr;
            Failure = true;
          }
        }
      }

      // on some systems even getting ".." fails, we create dummy ".." instead
      if (File == nullptr)
      {
        File = new TRemoteParentDirectory(FTerminal);
      }

      assert(File && File->GetIsParentDirectory());
      FileList->AddFile(File);

      if (Failure)
      {
        throw ExtException(
          nullptr, FMTLOAD(EMPTY_DIRECTORY, FileList->GetDirectory().c_str()),
          HELP_EMPTY_DIRECTORY);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadSymlink(TRemoteFile * SymlinkFile,
  TRemoteFile *& AFile)
{
  assert(SymlinkFile && SymlinkFile->GetIsSymLink());
  assert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later

  // need to use full filename when resolving links within subdirectory
  // (i.e. for download)
  UnicodeString FileName = LocalCanonify(
    SymlinkFile->GetDirectory() != nullptr ? SymlinkFile->GetFullFileName() : SymlinkFile->GetFileName());

  TSFTPPacket ReadLinkPacket(SSH_FXP_READLINK, FCodePage);
  ReadLinkPacket.AddPathString(FileName, FUtfStrings);
  SendPacket(&ReadLinkPacket);
  ReserveResponse(&ReadLinkPacket, &ReadLinkPacket);

  // send second request before reading response to first one
  // (performance benefit)
  TSFTPPacket AttrsPacket(SSH_FXP_STAT, FCodePage);
  AttrsPacket.AddPathString(FileName, FUtfStrings);
  if (FVersion >= 4)
  {
    AttrsPacket.AddCardinal(SSH_FILEXFER_ATTR_COMMON);
  }
  SendPacket(&AttrsPacket);
  ReserveResponse(&AttrsPacket, &AttrsPacket);

  ReceiveResponse(&ReadLinkPacket, &ReadLinkPacket, SSH_FXP_NAME);
  if (ReadLinkPacket.GetCardinal() != 1)
  {
    FTerminal->FatalError(nullptr, LoadStr(SFTP_NON_ONE_FXP_NAME_PACKET));
  }
  SymlinkFile->SetLinkTo(ReadLinkPacket.GetPathString(FUtfStrings));
  FTerminal->LogEvent(FORMAT(L"Link resolved to \"%s\".", SymlinkFile->GetLinkTo().c_str()));

  ReceiveResponse(&AttrsPacket, &AttrsPacket, SSH_FXP_ATTRS);
  // SymlinkFile->FileName was used instead SymlinkFile->LinkTo before, why?
  AFile = LoadFile(&AttrsPacket, SymlinkFile,
    ::UnixExtractFileName(SymlinkFile->GetLinkTo()));
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ReadFile(const UnicodeString & FileName,
  TRemoteFile *& AFile)
{
  CustomReadFile(FileName, AFile, SSH_FXP_LSTAT);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::RemoteFileExists(const UnicodeString & FullPath,
  TRemoteFile ** AFile)
{
  bool Result;
  try
  {
    TRemoteFile * File;
    CustomReadFile(FullPath, File, SSH_FXP_LSTAT, nullptr, asNoSuchFile);
    Result = (File != nullptr);
    if (Result)
    {
      if (AFile)
      {
        *AFile = File;
      }
      else
      {
        SAFE_DESTROY(File);
      }
    }
  }
  catch (...)
  {
    if (!FTerminal->GetActive())
    {
      throw;
    }
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SendCustomReadFile(TSFTPPacket * Packet,
  TSFTPPacket * Response, uint32_t Flags)
{
  if (FVersion >= 4)
  {
    Packet->AddCardinal(Flags);
  }
  SendPacket(Packet);
  ReserveResponse(Packet, Response);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CustomReadFile(const UnicodeString & FileName,
  TRemoteFile *& AFile, uint8_t Type, TRemoteFile * ALinkedByFile,
  int AllowStatus)
{
  uint32_t Flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_PERMISSIONS |
    SSH_FILEXFER_ATTR_ACCESSTIME | SSH_FILEXFER_ATTR_MODIFYTIME |
    SSH_FILEXFER_ATTR_OWNERGROUP;
  TSFTPPacket Packet(Type, FCodePage);
  Packet.AddPathString(LocalCanonify(FileName), FUtfStrings);
  SendCustomReadFile(&Packet, &Packet, Flags);
  ReceiveResponse(&Packet, &Packet, SSH_FXP_ATTRS, AllowStatus);

  if (Packet.GetType() == SSH_FXP_ATTRS)
  {
    AFile = LoadFile(&Packet, ALinkedByFile, ::UnixExtractFileName(FileName));
  }
  else
  {
    assert(AllowStatus > 0);
    AFile = nullptr;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DoDeleteFile(const UnicodeString & FileName, uint8_t Type)
{
  TSFTPPacket Packet(Type, FCodePage);
  UnicodeString RealFileName = LocalCanonify(FileName);
  Packet.AddPathString(RealFileName, FUtfStrings);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DeleteFile(const UnicodeString & FileName,
  const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action)
{
  uint8_t Type;
  if (AFile && AFile->GetIsDirectory() && !AFile->GetIsSymLink())
  {
    if (FLAGCLEAR(Params, dfNoRecursive))
    {
      try
      {
        FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::DeleteFile, FTerminal), &Params);
      }
      catch (...)
      {
        Action.Cancel();
        throw;
      }
    }
    Type = SSH_FXP_RMDIR;
  }
  else
  {
    Type = SSH_FXP_REMOVE;
  }

  DoDeleteFile(FileName, Type);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::RenameFile(const UnicodeString & FileName,
  const UnicodeString & NewName)
{
  TSFTPPacket Packet(SSH_FXP_RENAME, FCodePage);
  UnicodeString RealName = LocalCanonify(FileName);
  Packet.AddPathString(RealName, FUtfStrings);
  UnicodeString TargetName;
  if (::UnixExtractFilePath(NewName).IsEmpty())
  {
    // rename case (TTerminal::RenameFile)
    TargetName = ::UnixExtractFilePath(RealName) + NewName;
  }
  else
  {
    TargetName = LocalCanonify(NewName);
  }
  Packet.AddPathString(TargetName, FUtfStrings);
  if (FVersion >= 5)
  {
    Packet.AddCardinal(0);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CopyFile(const UnicodeString & /*FileName*/,
  const UnicodeString & /*NewName*/)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CreateDirectory(const UnicodeString & DirName)
{
  TSFTPPacket Packet(SSH_FXP_MKDIR, FCodePage);
  UnicodeString CanonifiedName = Canonify(DirName);
  Packet.AddPathString(CanonifiedName, FUtfStrings);
  Packet.AddProperties(nullptr, 0, true, FVersion, FUtfStrings, nullptr);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CreateLink(const UnicodeString & FileName,
  const UnicodeString & PointTo, bool Symbolic)
{
  USEDPARAM(Symbolic);
  assert(Symbolic); // only symlinks are supported by SFTP
  assert(FVersion >= 3); // symlinks are supported with SFTP version 3 and later
  TSFTPPacket Packet(SSH_FXP_SYMLINK, FCodePage);

  bool Buggy = (GetSessionData()->GetSFTPBug(sbSymlink) == asOn) ||
    ((GetSessionData()->GetSFTPBug(sbSymlink) == asAuto) && FSecureShell->IsOpenSSH());

  if (!Buggy)
  {
    Packet.AddPathString(Canonify(FileName), FUtfStrings);
    Packet.AddPathString(PointTo, FUtfStrings);
  }
  else
  {
    FTerminal->LogEvent(L"We believe the server has SFTP symlink bug");
    Packet.AddPathString(PointTo, FUtfStrings);
    Packet.AddPathString(Canonify(FileName), FUtfStrings);
  }
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::ChangeFileProperties(const UnicodeString & FileName,
  const TRemoteFile * /*AFile*/, const TRemoteProperties * AProperties,
  TChmodSessionAction & Action)
{
  assert(AProperties != nullptr);

  UnicodeString RealFileName = LocalCanonify(FileName);
  TRemoteFile * File;
  ReadFile(RealFileName, File);
  std::unique_ptr<TRemoteFile> FilePtr(File);
  assert(FilePtr.get());
  if (FilePtr->GetIsDirectory() && !FilePtr->GetIsSymLink() && AProperties->Recursive)
  {
    try
    {
      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TTerminal::ChangeFileProperties, FTerminal),
        static_cast<void *>(const_cast<TRemoteProperties *>(AProperties)));
    }
    catch (...)
    {
      Action.Cancel();
      throw;
    }
  }

  // SFTP can change owner and group at the same time only, not individually.
  // Fortunately we know current owner/group, so if only one is present,
  // we can supplement the other.
  TRemoteProperties Properties(*AProperties);
  if (Properties.Valid.Contains(vpGroup) &&
      !Properties.Valid.Contains(vpOwner))
  {
    Properties.Owner = File->GetFileOwner();
    Properties.Valid << vpOwner;
  }
  else if (Properties.Valid.Contains(vpOwner) &&
           !Properties.Valid.Contains(vpGroup))
  {
    Properties.Group = File->GetFileGroup();
    Properties.Valid << vpGroup;
  }

  TSFTPPacket Packet(SSH_FXP_SETSTAT, FCodePage);
  Packet.AddPathString(RealFileName, FUtfStrings);
  Packet.AddProperties(&Properties, *File->GetRights(), File->GetIsDirectory(), FVersion, FUtfStrings, &Action);
  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_STATUS);
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::LoadFilesProperties(TStrings * FileList)
{
  bool Result = false;
  // without knowledge of server's capabilities, this all make no sense
  if (FSupport->Loaded)
  {
    TFileOperationProgressType Progress(MAKE_CALLBACK(TTerminal::DoProgress, FTerminal), MAKE_CALLBACK(TTerminal::DoFinished, FTerminal));
    Progress.Start(foGetProperties, osRemote, FileList->GetCount());

    FTerminal->FOperationProgress = &Progress; //-V506

    TSFTPLoadFilesPropertiesQueue Queue(this, FCodePage);
    SCOPE_EXIT
    {
      Queue.DisposeSafe();
      FTerminal->FOperationProgress = nullptr;
      Progress.Stop();
    };
    {
      static int LoadFilesPropertiesQueueLen = 5;
      if (Queue.Init(LoadFilesPropertiesQueueLen, FileList))
      {
        TRemoteFile * File;
        TSFTPPacket Packet(FCodePage);
        bool Next;
        do
        {
          Next = Queue.ReceivePacket(&Packet, File);
          assert((Packet.GetType() == SSH_FXP_ATTRS) || (Packet.GetType() == SSH_FXP_STATUS));
          if (Packet.GetType() == SSH_FXP_ATTRS)
          {
            assert(File != nullptr);
            Progress.SetFile(File->GetFileName());
            LoadFile(File, &Packet);
            Result = true;
            TOnceDoneOperation OnceDoneOperation;
            Progress.Finish(File->GetFileName(), true, OnceDoneOperation);
          }

          if (Progress.Cancel != csContinue)
          {
            Next = false;
          }
        }
        while (Next);
      }
    }
    // queue is discarded here
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::DoCalculateFilesChecksum(const UnicodeString & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum,
  TFileOperationProgressType * OperationProgress, bool FirstLevel)
{
  TOnceDoneOperation OnceDoneOperation; // not used

  // recurse into subdirectories only if we have callback function
  if (OnCalculatedChecksum != nullptr)
  {
    for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
    {
      TRemoteFile * File = static_cast<TRemoteFile *>(FileList->GetObject(Index));
      assert(File != nullptr);
      if (File->GetIsDirectory() && !File->GetIsSymLink() &&
          !File->GetIsParentDirectory() && !File->GetIsThisDirectory())
      {
        OperationProgress->SetFile(File->GetFileName());
        std::unique_ptr<TRemoteFileList> SubFiles(
          FTerminal->CustomReadDirectoryListing(File->GetFullFileName(), false));

        if (SubFiles.get() != nullptr)
        {
          std::unique_ptr<TStrings> SubFileList(new TStringList());
          bool Success = false;
          SCOPE_EXIT
          {
            if (FirstLevel)
            {
              OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
            }
          };
          {
            OperationProgress->SetFile(File->GetFileName());

            for (intptr_t Index = 0; Index < SubFiles->GetCount(); ++Index)
            {
              TRemoteFile * SubFile = SubFiles->GetFile(Index);
              SubFileList->AddObject(SubFile->GetFullFileName(), SubFile);
            }

            // do not collect checksums for files in subdirectories,
            // only send back checksums via callback
            DoCalculateFilesChecksum(Alg, SubFileList.get(), nullptr,
            OnCalculatedChecksum, OperationProgress, false);

            Success = true;
          }
        }
      }
    }
  }

  TSFTPCalculateFilesChecksumQueue Queue(this, FCodePage);
  SCOPE_EXIT
  {
    Queue.DisposeSafe();
  };
  {
    static int CalculateFilesChecksumQueueLen = 5;
    if (Queue.Init(CalculateFilesChecksumQueueLen, Alg, FileList))
    {
      TSFTPPacket Packet(FCodePage);
      bool Next = false;
      do
      {
        bool Success = false;
        UnicodeString Alg;
        UnicodeString Checksum;
        TRemoteFile * File = nullptr;

        SCOPE_EXIT
        {
          if (FirstLevel)
          {
            OperationProgress->Finish(File->GetFileName(), Success, OnceDoneOperation);
          }
        };
        {
          try
          {
            Next = Queue.ReceivePacket(&Packet, File);
            assert(Packet.GetType() == SSH_FXP_EXTENDED_REPLY);

            OperationProgress->SetFile(File->GetFileName());

            Alg = Packet.GetAnsiString();
            Checksum = BytesToHex(reinterpret_cast<const uint8_t*>(Packet.GetNextData(Packet.GetRemainingLength())), Packet.GetRemainingLength());
            OnCalculatedChecksum(File->GetFileName(), Alg, Checksum);

            Success = true;
          }
          catch (Exception & E)
          {
            FTerminal->CommandError(&E, FMTLOAD(CHECKSUM_ERROR,
              File != nullptr ? File->GetFullFileName().c_str() : L""));
            // TODO: retries? resume?
            Next = false;
          }

          if (Checksums != nullptr)
          {
            Checksums->Add(L"");
          }
        }

        if (OperationProgress->Cancel != csContinue)
        {
          Next = false;
        }
      }
      while (Next);
    }
  }
  // queue is discarded here
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CalculateFilesChecksum(const UnicodeString & Alg,
  TStrings * FileList, TStrings * Checksums,
  TCalculatedChecksumEvent OnCalculatedChecksum)
{
  TFileOperationProgressType Progress(MAKE_CALLBACK(TTerminal::DoProgress, FTerminal), MAKE_CALLBACK(TTerminal::DoFinished, FTerminal));
  Progress.Start(foCalculateChecksum, osRemote, FileList->GetCount());

  FTerminal->FOperationProgress = &Progress; //-V506

  SCOPE_EXIT
  {
    FTerminal->FOperationProgress = nullptr;
    Progress.Stop();
  };
  {
    DoCalculateFilesChecksum(Alg, FileList, Checksums, OnCalculatedChecksum,
      &Progress, true);
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CustomCommandOnFile(const UnicodeString & /* FileName */,
  const TRemoteFile * /* AFile */, const UnicodeString & /* Command */, intptr_t /* Params */,
  TCaptureOutputEvent /* OutputEvent */)
{
  assert(false);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::AnyCommand(const UnicodeString & /*Command*/,
  TCaptureOutputEvent /* OutputEvent */)
{
  assert(false);
}
//---------------------------------------------------------------------------
UnicodeString TSFTPFileSystem::FileUrl(const UnicodeString & FileName) const
{
  return FTerminal->FileUrl(SftpProtocol, FileName);
}
//---------------------------------------------------------------------------
TStrings * TSFTPFileSystem::GetFixedPaths()
{
  return FFixedPaths;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SpaceAvailable(const UnicodeString & Path,
  TSpaceAvailable & ASpaceAvailable)
{
  if (SupportsExtension(SFTP_EXT_SPACE_AVAILABLE))
  {
    TSFTPPacket Packet(SSH_FXP_EXTENDED);
    Packet.AddString(SFTP_EXT_SPACE_AVAILABLE);
    Packet.AddPathString(LocalCanonify(Path), FUtfStrings);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);

    ASpaceAvailable.BytesOnDevice = Packet.GetInt64();
    ASpaceAvailable.UnusedBytesOnDevice = Packet.GetInt64();
    ASpaceAvailable.BytesAvailableToUser = Packet.GetInt64();
    ASpaceAvailable.UnusedBytesAvailableToUser = Packet.GetInt64();
    ASpaceAvailable.BytesPerAllocationUnit = Packet.GetCardinal();
  }
  else if (ALWAYS_TRUE(FSupportsStatVfsV2))
  {
    // http://www.openbsd.org/cgi-bin/cvsweb/src/usr.bin/ssh/PROTOCOL?rev=HEAD;content-type=text/plain
    TSFTPPacket Packet(SSH_FXP_EXTENDED);
    Packet.AddString(SFTP_EXT_STATVFS);
    Packet.AddPathString(LocalCanonify(Path), FUtfStrings);

    SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_EXTENDED_REPLY);

    __int64 BlockSize = Packet.GetInt64(); // file system block size
    __int64 FundamentalBlockSize = Packet.GetInt64(); // fundamental fs block size
    __int64 Blocks = Packet.GetInt64(); // number of blocks (unit f_frsize)
    __int64 FreeBlocks = Packet.GetInt64(); // free blocks in file system
    __int64 AvailableBlocks = Packet.GetInt64(); // free blocks for non-root
    __int64 FileINodes = Packet.GetInt64(); // total file inodes
    __int64 FreeFileINodes = Packet.GetInt64(); // free file inodes
    __int64 AvailableFileINodes = Packet.GetInt64(); // free file inodes for to non-root
    __int64 SID = Packet.GetInt64(); // file system id
    __int64 Flags = Packet.GetInt64(); // bit mask of f_flag values
    __int64 NameMax = Packet.GetInt64(); // maximum filename length

    FTerminal->LogEvent(FORMAT(L"Block size: %s", IntToStr(BlockSize).c_str()));
    FTerminal->LogEvent(FORMAT(L"Fundamental block size: %s", IntToStr(FundamentalBlockSize).c_str()));
    FTerminal->LogEvent(FORMAT(L"Total blocks: %s", IntToStr(Blocks).c_str()));
    FTerminal->LogEvent(FORMAT(L"Free blocks: %s", IntToStr(FreeBlocks).c_str()));
    FTerminal->LogEvent(FORMAT(L"Free blocks for non-root: %s", IntToStr(AvailableBlocks).c_str()));
    FTerminal->LogEvent(FORMAT(L"Total file inodes: %s", IntToStr(FileINodes).c_str()));
    FTerminal->LogEvent(FORMAT(L"Free file inodes: %s", IntToStr(FreeFileINodes).c_str()));
    FTerminal->LogEvent(FORMAT(L"Free file inodes for non-root: %s", IntToStr(AvailableFileINodes).c_str()));
    FTerminal->LogEvent(FORMAT(L"File system ID: %s", BytesToHex(reinterpret_cast<const uint8_t *>(&SID), sizeof(SID)).c_str()));
    UnicodeString FlagStr;
    if (FLAGSET(Flags, SFTP_EXT_STATVFS_ST_RDONLY))
    {
      AddToList(FlagStr, L"read-only", L",");
      Flags -= SFTP_EXT_STATVFS_ST_RDONLY;
    }
    if (FLAGSET(Flags, SFTP_EXT_STATVFS_ST_NOSUID))
    {
      AddToList(FlagStr, L"no-setuid", L",");
      Flags -= SFTP_EXT_STATVFS_ST_NOSUID;
    }
    if (Flags != 0)
    {
      AddToList(FlagStr, UnicodeString(L"0x") + IntToHex(Flags, 2), L",");
    }
    if (FlagStr.IsEmpty())
    {
      FlagStr = L"none";
    }
    FTerminal->LogEvent(FORMAT(L"Flags: %s", FlagStr.c_str()));
    FTerminal->LogEvent(FORMAT(L"Max name length: %s", IntToStr(NameMax).c_str()));

    ASpaceAvailable.BytesOnDevice = BlockSize * Blocks;
    ASpaceAvailable.UnusedBytesOnDevice = BlockSize * FreeBlocks;
    ASpaceAvailable.BytesAvailableToUser = 0;
    ASpaceAvailable.UnusedBytesAvailableToUser = BlockSize * AvailableBlocks;
    ASpaceAvailable.BytesPerAllocationUnit =
      (BlockSize > UINT_MAX /*std::numeric_limits<unsigned long>::max()*/) ? 0 : static_cast<unsigned long>(BlockSize);
  }
}
//---------------------------------------------------------------------------
// transfer protocol
//---------------------------------------------------------------------------
void TSFTPFileSystem::CopyToRemote(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  assert(AFilesToCopy && OperationProgress);

  UnicodeString FileName, FileNameOnly;
  UnicodeString FullTargetDir = ::UnixIncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    bool Success = false;
    FileName = AFilesToCopy->GetString(Index);
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, AFilesToCopy->GetObject(Index));
    UnicodeString RealFileName = File ? File->GetFileName() : FileName;
    FileNameOnly = ExtractFileName(RealFileName, false);
    assert(!FAvoidBusy);
    FAvoidBusy = true;

    SCOPE_EXIT
    {
      FAvoidBusy = false;
      OperationProgress->Finish(RealFileName, Success, OnceDoneOperation);
    };
    {
      try
      {
        if (GetSessionData()->GetCacheDirectories())
        {
          FTerminal->DirectoryModified(TargetDir, false);

          if (DirectoryExists(ExtractFilePath(FileName)))
          {
            FTerminal->DirectoryModified(UnixIncludeTrailingBackslash(TargetDir) +
              FileNameOnly, true);
          }
        }
        SFTPSourceRobust(FileName, File, FullTargetDir, CopyParam, Params, OperationProgress,
          tfFirstLevel);
        Success = true;
      }
      catch (EScpSkipFile & E)
      {
        DEBUG_PRINTF(L"before FTerminal->HandleException");
        SUSPEND_OPERATION (
          if (!FTerminal->HandleException(&E))
          {
            throw;
          }
        );
      }
    }
    ++Index;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPConfirmOverwrite(UnicodeString & FileName,
  const TCopyParamType * CopyParam, intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOverwriteMode & OverwriteMode, const TOverwriteFileParams * FileParams)
{
  bool CanAppend = (FVersion < 4) || !OperationProgress->AsciiTransfer;
  bool CanResume =
    (FileParams != nullptr) &&
    (FileParams->DestSize < FileParams->SourceSize);
  uintptr_t Answer = 0;
  SUSPEND_OPERATION
  (
    // abort = "append"
    // retry = "resume"
    // all = "yes to newer"
    // ignore = "rename"
    uintptr_t Answers = qaYes | qaNo | qaCancel | qaYesToAll | qaNoToAll | qaAll | qaIgnore;

    // possibly we can allow alternate resume at least in some cases
    if (CanAppend)
    {
      Answers |= qaRetry;
    }
    TQueryButtonAlias Aliases[5];
    Aliases[0].Button = qaRetry;
    Aliases[0].Alias = LoadStr(APPEND_BUTTON);
    Aliases[0].GroupWith = qaNo;
    Aliases[0].GrouppedShiftState = TShiftState() << ssAlt;
    Aliases[1].Button = qaAll;
    Aliases[1].Alias = LoadStr(YES_TO_NEWER_BUTTON);
    Aliases[1].GroupWith = qaYes;
    Aliases[1].GrouppedShiftState = TShiftState() << ssCtrl;
    Aliases[2].Button = qaIgnore;
    Aliases[2].Alias = LoadStr(RENAME_BUTTON);
    Aliases[2].GroupWith = qaNo;
    Aliases[2].GrouppedShiftState = TShiftState() << ssCtrl;
    Aliases[3].Button = qaYesToAll;
    Aliases[3].GroupWith = qaYes;
    Aliases[3].GrouppedShiftState = TShiftState() << ssShift;
    Aliases[4].Button = qaNoToAll;
    Aliases[4].GroupWith = qaNo;
    Aliases[4].GrouppedShiftState = TShiftState() << ssShift;
    TQueryParams QueryParams(qpNeverAskAgainCheck);
    QueryParams.NoBatchAnswers = qaIgnore | qaAbort | qaRetry | qaAll;
    QueryParams.Aliases = Aliases;
    QueryParams.AliasesCount = LENOF(Aliases);
    Answer = FTerminal->ConfirmFileOverwrite(FileName, FileParams,
      Answers, &QueryParams,
      OperationProgress->Side == osLocal ? osRemote : osLocal,
      CopyParam, Params, OperationProgress);
  );

  if (CanAppend &&
      ((Answer == qaRetry) || (Answer == qaSkip)))
  {
    // duplicated in TTerminal::ConfirmFileOverwrite
    bool CanAlternateResume =
      FileParams ? (FileParams->DestSize < FileParams->SourceSize) && !OperationProgress->AsciiTransfer : false;
    TBatchOverwrite BatchOverwrite =
      FTerminal->EffectiveBatchOverwrite(CopyParam, Params, OperationProgress, true);
    // when mode is forced by batch, never query user
    if (BatchOverwrite == boAppend)
    {
      OverwriteMode = omAppend;
    }
    else if (CanAlternateResume && CanResume &&
             ((BatchOverwrite == boResume) || (BatchOverwrite == boAlternateResume)))
    {
      OverwriteMode = omResume;
    }
    // no other option, but append
    else if (!CanAlternateResume)
    {
      OverwriteMode = omAppend;
    }
    else
    {
      TQueryParams Params(0, HELP_APPEND_OR_RESUME);
      SUSPEND_OPERATION
      (
        Answer = FTerminal->QueryUser(FORMAT(LoadStr(APPEND_OR_RESUME2).c_str(), FileName.c_str()),
          nullptr, qaYes | qaNo | qaNoToAll | qaCancel, &Params);
      );

      switch (Answer)
      {
        case qaYes:
          OverwriteMode = omAppend;
          break;

        case qaNo:
          OverwriteMode = omResume;
          break;

        case qaNoToAll:
          OverwriteMode = omResume;
          OperationProgress->BatchOverwrite = boAlternateResume;
          break;

        default: assert(false); //fallthru
        case qaCancel:
          if (!OperationProgress->Cancel)
          {
            OperationProgress->Cancel = csCancel;
          }
          Abort();
          break;
      }
    }
  }
  else if (Answer == qaIgnore)
  {
    if (FTerminal->PromptUser(FTerminal->GetSessionData(), pkFileName, LoadStr(RENAME_TITLE), L"",
          LoadStr(RENAME_PROMPT2), true, 0, FileName))
    {
      OverwriteMode = omOverwrite;
    }
    else
    {
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      Abort();
    }
  }
  else
  {
    OverwriteMode = omOverwrite;
    switch (Answer)
    {
      case qaCancel:
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
        Abort();
        break;

      case qaNo:
        ThrowSkipFileNull();
    }
  }
}
//---------------------------------------------------------------------------
bool TSFTPFileSystem::SFTPConfirmResume(const UnicodeString & DestFileName,
  bool PartialBiggerThanSource, TFileOperationProgressType * OperationProgress)
{
  bool ResumeTransfer = false;
  assert(OperationProgress);
  uintptr_t Answer = 0;
  if (PartialBiggerThanSource)
  {
    SUSPEND_OPERATION
    (
      TQueryParams Params(qpAllowContinueOnError, HELP_PARTIAL_BIGGER_THAN_SOURCE);
      Answer = FTerminal->QueryUser(
        FMTLOAD(PARTIAL_BIGGER_THAN_SOURCE, DestFileName.c_str()), nullptr,
          qaOK | qaAbort, &Params, qtWarning);
    )

    if (Answer == qaAbort)
    {
      if (!OperationProgress->Cancel)
      {
        OperationProgress->Cancel = csCancel;
      }
      Abort();
    }
    ResumeTransfer = false;
  }
  else if (FTerminal->GetConfiguration()->GetConfirmResume())
  {
    uintptr_t Answer;
    SUSPEND_OPERATION
    (
      TQueryParams Params(qpAllowContinueOnError | qpNeverAskAgainCheck,
        HELP_RESUME_TRANSFER);
      // "abort" replaced with "cancel" to unify with "append/resume" query
      Answer = FTerminal->QueryUser(
        FMTLOAD(RESUME_TRANSFER2, DestFileName.c_str()), nullptr, qaYes | qaNo | qaCancel,
        &Params);
    );

    switch (Answer)
    {
      case qaNeverAskAgain:
        FTerminal->GetConfiguration()->SetConfirmResume(false);
      case qaYes:
        ResumeTransfer = true;
        break;

      case qaNo:
        ResumeTransfer = false;
        break;

      case qaCancel:
        if (!OperationProgress->Cancel)
        {
          OperationProgress->Cancel = csCancel;
        }
        Abort();
        break;
    }
  }
  else
  {
    ResumeTransfer = true;
  }
  return ResumeTransfer;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSourceRobust(const UnicodeString & FileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TFTPFileSystem
  bool Retry;

  TUploadSessionAction Action(FTerminal->GetActionLog());
  TOpenRemoteFileParams OpenParams;
  OpenParams.OverwriteMode = omOverwrite;
  TOverwriteFileParams FileParams;

  do
  {
    Retry = false;
    bool ChildError = false;
    try
    {
      SFTPSource(FileName, AFile, TargetDir, CopyParam, Params,
        OpenParams, FileParams,
        OperationProgress,
        Flags, Action, ChildError);
    }
    catch (Exception & E)
    {
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        if (!ChildError)
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (Retry)
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      // prevent overwrite and resume confirmations
      // (should not be set for directories!)
      Params |= cpNoConfirmation;
      // enable resume even if we are uploading into new directory
      Flags &= ~tfNewDirectory;
    }
  }
  while (Retry);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSource(const UnicodeString & FileName,
  const TRemoteFile * AFile,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
  TOpenRemoteFileParams & OpenParams,
  TOverwriteFileParams & FileParams,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TUploadSessionAction & Action, bool & ChildError)
{
  UnicodeString RealFileName = AFile ? AFile->GetFileName() : FileName;

  Action.FileName(ExpandUNCFileName(RealFileName));

  OperationProgress->SetFile(RealFileName, false);

  if (!FTerminal->AllowLocalFileTransfer(FileName, CopyParam))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", RealFileName.c_str()));
    ThrowSkipFileNull();
  }

  // TOpenRemoteFileParams OpenParams;
  // OpenParams.OverwriteMode = omOverwrite;

  HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
  __int64 MTime = 0, ATime = 0;
  __int64 Size = 0;

  FTerminal->OpenLocalFile(FileName, GENERIC_READ, &OpenParams.LocalFileAttrs,
    &LocalFileHandle, nullptr, &MTime, &ATime, &Size);

  bool Dir = FLAGSET(OpenParams.LocalFileAttrs, faDirectory);

  {
    SCOPE_EXIT
    {
      if (LocalFileHandle != INVALID_HANDLE_VALUE)
      {
        ::CloseHandle(LocalFileHandle);
      }
    };
    OperationProgress->SetFileInProgress();

    if (Dir)
    {
      Action.Cancel();
      SFTPDirectorySource(IncludeTrailingBackslash(FileName), TargetDir,
        OpenParams.LocalFileAttrs, CopyParam, Params, OperationProgress, Flags);
    }
    else
    {
      // File is regular file (not directory)
      assert(LocalFileHandle);

      UnicodeString DestFileName = CopyParam->ChangeFileName(ExtractFileName(RealFileName, false),
        osLocal, FLAGSET(Flags, tfFirstLevel));
      UnicodeString DestFullName = LocalCanonify(TargetDir + DestFileName);
      UnicodeString DestPartialFullName;
      bool ResumeAllowed;
      bool ResumeTransfer = false;
      bool DestFileExists = false;
      TRights DestRights;

      __int64 ResumeOffset = 0;

      FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to remote directory started.", RealFileName.c_str()));

      OperationProgress->SetLocalSize(Size);

      // Suppose same data size to transfer as to read
      // (not true with ASCII transfer)
      OperationProgress->SetTransferSize(OperationProgress->LocalSize);
      OperationProgress->TransferingFile = false;

      TDateTime Modification = ::UnixToDateTime(MTime, GetSessionData()->GetDSTMode());

      // Will we use ASCII of BINARY file transfer?
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Size;
      MaskParams.Modification = Modification;
      OperationProgress->SetAsciiTransfer(
        CopyParam->UseAsciiTransfer(RealFileName, osLocal, MaskParams));
      FTerminal->LogEvent(
        UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
          L" transfer mode selected.");

      // should we check for interrupted transfer?
      ResumeAllowed = !OperationProgress->AsciiTransfer &&
        CopyParam->AllowResume(OperationProgress->LocalSize) &&
        IsCapable(fcRename);
      OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

      // TOverwriteFileParams FileParams;
      FileParams.SourceSize = OperationProgress->LocalSize;
      FileParams.SourceTimestamp = Modification;

      if (ResumeAllowed)
      {
        DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();

        if (FLAGCLEAR(Flags, tfNewDirectory))
        {
          FTerminal->LogEvent(L"Checking existence of file.");
          TRemoteFile * File = nullptr;
          DestFileExists = RemoteFileExists(DestFullName, &File);
          if (DestFileExists)
          {
            OpenParams.DestFileSize = File->GetSize();
            FileParams.DestSize = OpenParams.DestFileSize;
            FileParams.DestTimestamp = File->GetModification();
            DestRights = *File->GetRights();
            // if destination file is symlink, never do resumable transfer,
            // as it would delete the symlink.
            // also bit of heuristics to detect symlink on SFTP-3 and older
            // (which does not indicate symlink in SSH_FXP_ATTRS).
            // if file has all permissions and is small, then it is likely symlink.
            // also it is not likely that such a small file (if it is not symlink)
            // gets overwritten by large file (that would trigger resumable transfer).
            if (File->GetIsSymLink() ||
                ((FVersion < 4) &&
                 ((*File->GetRights() & static_cast<unsigned short>(TRights::rfAll)) == static_cast<unsigned short>(TRights::rfAll)) &&
                 (File->GetSize() < 100)))
            {
              ResumeAllowed = false;
              OperationProgress->SetResumeStatus(rsDisabled);
            }

            SAFE_DESTROY(File);
          }

          if (ResumeAllowed)
          {
            FTerminal->LogEvent(L"Checking existence of partially transfered file.");
            if (RemoteFileExists(DestPartialFullName, &File))
            {
              ResumeOffset = File->GetSize();
              SAFE_DESTROY(File);

              bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->LocalSize);
              if (FLAGCLEAR(Params, cpNoConfirmation) &&
                  FLAGCLEAR(Params, cpResume))
              {
                ResumeTransfer = SFTPConfirmResume(DestFileName,
                  PartialBiggerThanSource, OperationProgress);
              }
              else
              {
                ResumeTransfer = !PartialBiggerThanSource;
              }

              if (!ResumeTransfer)
              {
                DoDeleteFile(DestPartialFullName, SSH_FXP_REMOVE);
              }
              else
              {
                FTerminal->LogEvent(L"Resuming file transfer.");
              }
            }
            else
            {
              // partial upload file does not exists, check for full file
              if (DestFileExists)
              {
                UnicodeString PrevDestFileName = DestFileName;
                SFTPConfirmOverwrite(DestFileName,
                  CopyParam, Params, OperationProgress, OpenParams.OverwriteMode, &FileParams);
                if (PrevDestFileName != DestFileName)
                {
                  // update paths in case user changes the file name
                  DestFullName = LocalCanonify(TargetDir + DestFileName);
                  DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
                  FTerminal->LogEvent(L"Checking existence of new file.");
                  DestFileExists = RemoteFileExists(DestFullName, nullptr);
                }
              }
            }
          }
        }
      }

      // will the transfer be resumable?
      bool DoResume = (ResumeAllowed && (OpenParams.OverwriteMode == omOverwrite));

      UnicodeString RemoteFileName = DoResume ? DestPartialFullName : DestFullName;
      OpenParams.RemoteFileName = RemoteFileName;
      OpenParams.Resume = DoResume;
      OpenParams.Resuming = ResumeTransfer;
      OpenParams.OperationProgress = OperationProgress;
      OpenParams.CopyParam = CopyParam;
      OpenParams.Params = Params;
      OpenParams.FileParams = &FileParams;
      OpenParams.Confirmed = false;

      FTerminal->LogEvent(L"Opening remote file.");
      FTerminal->FileOperationLoop(MAKE_CALLBACK(TSFTPFileSystem::SFTPOpenRemote, this), OperationProgress, true,
        FMTLOAD(SFTP_CREATE_FILE_ERROR, OpenParams.RemoteFileName.c_str()),
        &OpenParams);

      if (OpenParams.RemoteFileName != RemoteFileName)
      {
        assert(!DoResume);
        assert(::UnixExtractFilePath(OpenParams.RemoteFileName) == ::UnixExtractFilePath(RemoteFileName));
        DestFullName = OpenParams.RemoteFileName;
        UnicodeString NewFileName = ::UnixExtractFileName(DestFullName);
        assert(DestFileName != NewFileName);
        DestFileName = NewFileName;
      }

      Action.Destination(DestFullName);

      bool TransferFinished = false;
      TSFTPPacket CloseRequest(FCodePage);
      bool SetRights = ((DoResume && DestFileExists) || CopyParam->GetPreserveRights());
      bool SetProperties = (CopyParam->GetPreserveTime() || SetRights);
      TSFTPPacket PropertiesRequest(SSH_FXP_SETSTAT, FCodePage);
      TSFTPPacket PropertiesResponse(FCodePage);
      TRights Rights;
      if (SetProperties)
      {
        PropertiesRequest.AddPathString(DestFullName, FUtfStrings);
        if (CopyParam->GetPreserveRights())
        {
          Rights = CopyParam->RemoteFileRights(OpenParams.LocalFileAttrs);
        }
        else if (DoResume && DestFileExists)
        {
          Rights = DestRights;
        }
        else
        {
          assert(!SetRights);
        }

        unsigned short RightsNumber = Rights.GetNumberSet();
        PropertiesRequest.AddProperties(
          SetRights ? &RightsNumber : nullptr, nullptr, nullptr,
          CopyParam->GetPreserveTime() ? &MTime : nullptr,
          CopyParam->GetPreserveTime() ? &ATime : nullptr,
          nullptr, false, FVersion, FUtfStrings);
      }

      SCOPE_EXIT
      {
        if (FTerminal->GetActive())
        {
          // if file transfer was finished, the close request was already sent
          if (!OpenParams.RemoteFileHandle.IsEmpty())
          {
            SFTPCloseRemote(OpenParams.RemoteFileHandle, DestFileName,
              OperationProgress, TransferFinished, true, &CloseRequest);
          }
          // wait for the response
          SFTPCloseRemote(OpenParams.RemoteFileHandle, DestFileName,
            OperationProgress, TransferFinished, false, &CloseRequest);

          // delete file if transfer was not completed, resuming was not allowed and
          // we were not appending (incl. alternate resume),
          // shortly after plain transfer completes (eq. !ResumeAllowed)
          if (!TransferFinished && !DoResume && (OpenParams.OverwriteMode == omOverwrite))
          {
            DoDeleteFile(OpenParams.RemoteFileName, SSH_FXP_REMOVE);
          }
        }
      };
      {
        __int64 DestWriteOffset = 0;
        if (OpenParams.OverwriteMode == omAppend)
        {
          FTerminal->LogEvent(L"Appending file.");
          DestWriteOffset = OpenParams.DestFileSize;
        }
        else if (ResumeTransfer || (OpenParams.OverwriteMode == omResume))
        {
          if (OpenParams.OverwriteMode == omResume)
          {
            FTerminal->LogEvent(L"Resuming file transfer (append style).");
            ResumeOffset = OpenParams.DestFileSize;
          }
          FileSeek(LocalFileHandle, ResumeOffset, 0);
          OperationProgress->AddResumed(ResumeOffset);
        }

        TSFTPUploadQueue Queue(this, FCodePage);
        {
          SCOPE_EXIT
          {
            Queue.DisposeSafe();
          };
          int ConvertParams =
            FLAGMASK(CopyParam->GetRemoveCtrlZ(), cpRemoveCtrlZ) |
            FLAGMASK(CopyParam->GetRemoveBOM(), cpRemoveBOM);
          Queue.Init(FileName, LocalFileHandle, OperationProgress,
            OpenParams.RemoteFileHandle,
            DestWriteOffset + OperationProgress->TransferedSize,
            ConvertParams);

          while (Queue.Continue())
          {
            if (OperationProgress->Cancel)
            {
              Abort();
            }
          }

          // send close request before waiting for pending read responses
          SFTPCloseRemote(OpenParams.RemoteFileHandle, DestFileName,
            OperationProgress, false, true, &CloseRequest);
          OpenParams.RemoteFileHandle = L"";

          // when resuming is disabled, we can send "set properties"
          // request before waiting for pending read/close responses
          if (SetProperties && !DoResume)
          {
            SendPacket(&PropertiesRequest);
            ReserveResponse(&PropertiesRequest, &PropertiesResponse);
          }
        }

        TransferFinished = true;
        // queue is discarded here
      }

      if (DoResume)
      {
        if (DestFileExists)
        {
          FILE_OPERATION_LOOP(FMTLOAD(DELETE_ON_RESUME_ERROR,
              ::UnixExtractFileName(DestFullName).c_str(), DestFullName.c_str()),

            if (GetSessionData()->GetOverwrittenToRecycleBin() &&
                !FTerminal->GetSessionData()->GetRecycleBinPath().IsEmpty())
            {
              FTerminal->RecycleFile(DestFullName, nullptr);
            }
            else
            {
              DoDeleteFile(DestFullName, SSH_FXP_REMOVE);
            }
          );
        }

        // originally this was before CLOSE (last __finally statement),
        // on VShell it failed
        FILE_OPERATION_LOOP_CUSTOM(FTerminal, true,
          FMTLOAD(RENAME_AFTER_RESUME_ERROR,
            ::UnixExtractFileName(OpenParams.RemoteFileName.c_str()).c_str(), DestFileName.c_str()),
          RenameFile(OpenParams.RemoteFileName, DestFileName),
          HELP_RENAME_AFTER_RESUME_ERROR
        );
      }

      if (SetProperties)
      {
        std::unique_ptr<TTouchSessionAction> TouchAction;
        if (CopyParam->GetPreserveTime())
        {
          TDateTime MDateTime = ::UnixToDateTime(MTime, FTerminal->GetSessionData()->GetDSTMode());
          FTerminal->LogEvent(FORMAT(L"Preserving timestamp [%s]",
            StandardTimestamp(MDateTime).c_str()));
          TouchAction.reset(new TTouchSessionAction(FTerminal->GetActionLog(), DestFullName,
            MDateTime));
        }
        std::unique_ptr<TChmodSessionAction> ChmodAction;
        // do record chmod only if it was explicitly requested,
        // not when it was implicitly performed to apply timestamp
        // of overwritten file to new file
        if (CopyParam->GetPreserveRights())
        {
          ChmodAction.reset(new TChmodSessionAction(FTerminal->GetActionLog(), DestFullName, Rights));
        }
        try
        {
          // when resuming is enabled, the set properties request was not sent yet
          if (DoResume)
          {
            SendPacket(&PropertiesRequest);
          }
          bool Resend = false;
          FILE_OPERATION_LOOP_CUSTOM(FTerminal, true, FMTLOAD(PRESERVE_TIME_PERM_ERROR3, DestFileName.c_str()),
            try
            {
              TSFTPPacket DummyResponse(FCodePage);
              TSFTPPacket * Response = &PropertiesResponse;
              if (Resend)
              {
                PropertiesRequest.Reuse();
                SendPacket(&PropertiesRequest);
                // ReceiveResponse currently cannot receive twice into same packet,
                // so DummyResponse is temporary workaround
                Response = &DummyResponse;
              }
              Resend = true;
              ReceiveResponse(&PropertiesRequest, Response, SSH_FXP_STATUS,
                asOK | FLAGMASK(CopyParam->GetIgnorePermErrors(), asPermDenied));
            }
            catch (...)
            {
              if (FTerminal->GetActive() &&
                  (!CopyParam->GetPreserveRights() && !CopyParam->GetPreserveTime()))
              {
                assert(DoResume);
                FTerminal->LogEvent(L"Ignoring error preserving permissions of overwritten file");
              }
              else
              {
                throw;
              }
            }
            ,
            HELP_PRESERVE_TIME_PERM_ERROR
          );
        }
        catch (Exception & E)
        {
          if (TouchAction.get() != nullptr)
          {
            TouchAction->Rollback(&E);
          }
          if (ChmodAction.get() != nullptr)
          {
            ChmodAction->Rollback(&E);
          }
          ChildError = true;
          throw;
        }
      }
    }
  }

  /* TODO : Delete also read-only files. */
  if (FLAGSET(Params, cpDelete))
  {
    if (!Dir)
    {
      FILE_OPERATION_LOOP (FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, FileName.c_str()),
        THROWOSIFFALSE(Sysutils::DeleteFile(FileName));
      )
    }
  }
  else if (CopyParam->GetClearArchive() && FLAGSET(OpenParams.LocalFileAttrs, faArchive))
  {
    FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, FileName.c_str()),
      THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(FileName, OpenParams.LocalFileAttrs & ~faArchive) == 0);
    )
  }
}
//---------------------------------------------------------------------------
RawByteString TSFTPFileSystem::SFTPOpenRemoteFile(
  const UnicodeString & FileName, uint32_t OpenType, __int64 Size)
{
  TSFTPPacket Packet(SSH_FXP_OPEN, FCodePage);

  Packet.AddPathString(FileName, FUtfStrings);
  if (FVersion < 5)
  {
    Packet.AddCardinal(OpenType);
  }
  else
  {
    uint32_t Access =
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_READ), ACE4_READ_DATA) |
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_WRITE), ACE4_WRITE_DATA | ACE4_APPEND_DATA);

    uint32_t Flags = 0;

    if (FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_EXCL))
    {
      Flags = SSH_FXF_CREATE_NEW;
    }
    else if (FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_TRUNC))
    {
      Flags = SSH_FXF_CREATE_TRUNCATE;
    }
    else if (FLAGSET(OpenType, SSH_FXF_CREAT))
    {
      Flags = SSH_FXF_OPEN_OR_CREATE;
    }
    else
    {
      Flags = SSH_FXF_OPEN_EXISTING;
    }

    Flags |=
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_APPEND), SSH_FXF_ACCESS_APPEND_DATA) |
      FLAGMASK(FLAGSET(OpenType, SSH_FXF_TEXT), SSH_FXF_ACCESS_TEXT_MODE);

    Packet.AddCardinal(Access);
    Packet.AddCardinal(Flags);
  }

  bool SendSize =
    (Size >= 0) &&
    FLAGSET(OpenType, SSH_FXF_CREAT | SSH_FXF_TRUNC);
  Packet.AddProperties(nullptr, nullptr, nullptr, nullptr, nullptr,
    SendSize ? &Size : nullptr, false, FVersion, FUtfStrings);

  SendPacketAndReceiveResponse(&Packet, &Packet, SSH_FXP_HANDLE);

  return Packet.GetFileHandle();
}
//---------------------------------------------------------------------------
int TSFTPFileSystem::SFTPOpenRemote(void * AOpenParams, void * /*Param2*/)
{
  TOpenRemoteFileParams * OpenParams = static_cast<TOpenRemoteFileParams *>(AOpenParams);
  assert(OpenParams);
  TFileOperationProgressType * OperationProgress = OpenParams->OperationProgress;

  int OpenType = 0;
  bool Success = false;
  bool ConfirmOverwriting = false;

  do
  {
    try
    {
      ConfirmOverwriting =
        !OpenParams->Confirmed && !OpenParams->Resume &&
        FTerminal->CheckRemoteFile(OpenParams->CopyParam, OpenParams->Params, OperationProgress);
      OpenType = SSH_FXF_WRITE | SSH_FXF_CREAT;
      // when we want to preserve overwritten files, we need to find out that
      // they exist first... even if overwrite confirmation is disabled.
      // but not when we already know we are not going to overwrite (but e.g. to append)
      if ((ConfirmOverwriting || GetSessionData()->GetOverwrittenToRecycleBin()) &&
          (OpenParams->OverwriteMode == omOverwrite))
      {
        OpenType |= SSH_FXF_EXCL;
      }
      if (!OpenParams->Resuming && (OpenParams->OverwriteMode == omOverwrite))
      {
        OpenType |= SSH_FXF_TRUNC;
      }
      if ((FVersion >= 4) && OpenParams->OperationProgress->AsciiTransfer)
      {
        OpenType |= SSH_FXF_TEXT;
      }

      OpenParams->RemoteFileHandle = SFTPOpenRemoteFile(
        OpenParams->RemoteFileName, OpenType, OperationProgress->LocalSize);

      Success = true;
    }
    catch (Exception & E)
    {
      if (!OpenParams->Confirmed && (OpenType & SSH_FXF_EXCL) && FTerminal->GetActive())
      {
        FTerminal->LogEvent(FORMAT(L"Cannot create new file \"%s\", checking if it exists already", OpenParams->RemoteFileName.c_str()));

        bool ThrowOriginal = false;

        // When exclusive opening of file fails, try to detect if file exists.
        // When file does not exist, failure was probably caused by 'permission denied'
        // or similar error. In this case throw original exception.
        try
        {
          UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
          TRemoteFile * File = nullptr;
          ReadFile(RealFileName, File);
          std::unique_ptr<TRemoteFile> FilePtr(File);
          assert(FilePtr.get());
          OpenParams->DestFileSize = FilePtr->GetSize();
          if (OpenParams->FileParams != nullptr)
          {
            OpenParams->FileParams->DestTimestamp = File->GetModification();
            OpenParams->FileParams->DestSize = OpenParams->DestFileSize;
          }
          // file exists (otherwise exception was thrown)
        }
        catch (...)
        {
          if (!FTerminal->GetActive())
          {
            throw;
          }
          else
          {
            ThrowOriginal = true;
          }
        }

        if (ThrowOriginal)
        {
          throw;
        }

        // we may get here even if confirmation is disabled,
        // when we have preserving of overwritten files enabled
        if (ConfirmOverwriting)
        {
          // confirmation duplicated in SFTPSource for resumable file transfers.
          UnicodeString RemoteFileNameOnly = ::UnixExtractFileName(OpenParams->RemoteFileName);
          SFTPConfirmOverwrite(RemoteFileNameOnly,
            OpenParams->CopyParam, OpenParams->Params, OperationProgress, OpenParams->OverwriteMode, OpenParams->FileParams);
          if (RemoteFileNameOnly != ::UnixExtractFileName(OpenParams->RemoteFileName))
          {
            OpenParams->RemoteFileName =
              ::UnixExtractFilePath(OpenParams->RemoteFileName) + RemoteFileNameOnly;
          }
          OpenParams->Confirmed = true;
        }
        else
        {
          assert(GetSessionData()->GetOverwrittenToRecycleBin());
        }

        if ((OpenParams->OverwriteMode == omOverwrite) &&
            GetSessionData()->GetOverwrittenToRecycleBin() &&
            !FTerminal->GetSessionData()->GetRecycleBinPath().IsEmpty())
        {
          FTerminal->RecycleFile(OpenParams->RemoteFileName, nullptr);
        }
      }
      else if (FTerminal->GetActive())
      {
        // if file overwriting was confirmed, it means that the file already exists,
        // if not, check now
        if (!OpenParams->Confirmed)
        {
          bool ThrowOriginal = false;

          // When file does not exist, failure was probably caused by 'permission denied'
          // or similar error. In this case throw original exception.
          try
          {
            TRemoteFile * File;
            UnicodeString RealFileName = LocalCanonify(OpenParams->RemoteFileName);
            ReadFile(RealFileName, File);
            SAFE_DESTROY(File);
          }
          catch (...)
          {
            if (!FTerminal->GetActive())
            {
              throw;
            }
            else
            {
              ThrowOriginal = true;
            }
          }

          if (ThrowOriginal)
          {
            throw;
          }
        }

        // now we know that the file exists

        if (FTerminal->FileOperationLoopQuery(E, OperationProgress,
              FMTLOAD(SFTP_OVERWRITE_FILE_ERROR, OpenParams->RemoteFileName.c_str()),
              true, LoadStr(SFTP_OVERWRITE_DELETE_BUTTON)))
        {
          intptr_t Params = dfNoRecursive;
          FTerminal->DeleteFile(OpenParams->RemoteFileName, nullptr, &Params);
        }
      }
      else
      {
        throw;
      }
    }
  }
  while (!Success);

  return 0;
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPCloseRemote(const RawByteString & Handle,
  const UnicodeString & FileName, TFileOperationProgressType * OperationProgress,
  bool TransferFinished, bool Request, TSFTPPacket * Packet)
{
  // Moving this out of SFTPSource() fixed external exception 0xC0000029 error
  FILE_OPERATION_LOOP(FMTLOAD(SFTP_CLOSE_FILE_ERROR, FileName.c_str()),
    try
    {
      TSFTPPacket CloseRequest(FCodePage);
      TSFTPPacket * P = (Packet == nullptr ? &CloseRequest : Packet);

      if (Request)
      {
        P->ChangeType(SSH_FXP_CLOSE);
        P->AddString(Handle);
        SendPacket(P);
        ReserveResponse(P, Packet);
      }
      else
      {
        assert(Packet != nullptr);
        ReceiveResponse(P, Packet, SSH_FXP_STATUS);
      }
    }
    catch (...)
    {
      if (!FTerminal->GetActive() || TransferFinished)
      {
        throw;
      }
    }
  );
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPDirectorySource(const UnicodeString & DirectoryName,
  const UnicodeString & TargetDir, uintptr_t LocalFileAttrs, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  UnicodeString DestDirectoryName = CopyParam->ChangeFileName(
    ExtractFileName(ExcludeTrailingBackslash(DirectoryName), false), osLocal,
    FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = ::UnixIncludeTrailingBackslash(TargetDir + DestDirectoryName);

  OperationProgress->SetFile(DirectoryName);

  bool CreateDir = false;
  try
  {
    TryOpenDirectory(DestFullName);
  }
  catch (...)
  {
    if (FTerminal->GetActive())
    {
      // opening directory failed, it probably does not exists, try to
      // create it
      CreateDir = true;
    }
    else
    {
      throw;
    }
  }

  if (CreateDir)
  {
    TRemoteProperties Properties;
    if (CopyParam->GetPreserveRights())
    {
      Properties.Valid = TValidProperties() << vpRights;
      Properties.Rights = CopyParam->RemoteFileRights(LocalFileAttrs);
    }
    FTerminal->CreateDirectory(DestFullName, &Properties);
    Flags |= tfNewDirectory;
  }

  DWORD FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  TSearchRec SearchRec;
  bool FindOK = false;

  FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
    FindOK = (bool)(FindFirstChecked(DirectoryName + L"*.*",
      FindAttrs, SearchRec) == 0);
  );

  SCOPE_EXIT
  {
    FindClose(SearchRec);
  };
  {
    while (FindOK && !OperationProgress->Cancel)
    {
      UnicodeString FileName = DirectoryName + SearchRec.Name;
      try
      {
        if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        {
          SFTPSourceRobust(FileName, nullptr, DestFullName, CopyParam, Params, OperationProgress,
            Flags & ~tfFirstLevel);
        }
      }
      catch (EScpSkipFile &E)
      {
        // If ESkipFile occurs, just log it and continue with next file
        SUSPEND_OPERATION (
          // here a message to user was displayed, which was not appropriate
          // when user refused to overwrite the file in subdirectory.
          // hopefully it won't be missing in other situations.
          if (!FTerminal->HandleException(&E)) throw;
        );
      }

      FILE_OPERATION_LOOP (FMTLOAD(LIST_DIR_ERROR, DirectoryName.c_str()),
        FindOK = (FindNextChecked(SearchRec) == 0);
      );
    }
  }

  /* TODO : Delete also read-only directories. */
  /* TODO : Show error message on failure. */
  if (!OperationProgress->Cancel)
  {
    if (FLAGSET(Params, cpDelete))
    {
      FTerminal->RemoveLocalDirectory(DirectoryName);
    }
    else if (CopyParam->GetClearArchive() && FLAGSET(LocalFileAttrs, faArchive))
    {
      FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DirectoryName.c_str()),
        THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DirectoryName, LocalFileAttrs & ~faArchive) == 0);
      )
    }
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::CopyToLocal(TStrings * AFilesToCopy,
  const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
  intptr_t Params, TFileOperationProgressType * OperationProgress,
  TOnceDoneOperation & OnceDoneOperation)
{
  assert(AFilesToCopy && OperationProgress);

  UnicodeString FileName;
  UnicodeString FullTargetDir = IncludeTrailingBackslash(TargetDir);
  intptr_t Index = 0;
  while (Index < AFilesToCopy->GetCount() && !OperationProgress->Cancel)
  {
    bool Success = false;
    FileName = AFilesToCopy->GetString(Index);
    const TRemoteFile * File = static_cast<TRemoteFile *>(AFilesToCopy->GetObject(Index));

    assert(!FAvoidBusy);
    FAvoidBusy = true;

    SCOPE_EXIT
    {
      FAvoidBusy = false;
      OperationProgress->Finish(FileName, Success, OnceDoneOperation);
    };
    {
      UnicodeString TargetDirectory = FullTargetDir;
      UnicodeString FileNamePath = ::ExtractFilePath(File->GetFileName());
      if (!FileNamePath.IsEmpty())
      {
        TargetDirectory = ::IncludeTrailingBackslash(TargetDirectory + FileNamePath);
        ::ForceDirectories(TargetDirectory);
      }
      try
      {
        SFTPSinkRobust(LocalCanonify(FileName), File, TargetDirectory, CopyParam,
          Params, OperationProgress, tfFirstLevel);
        Success = true;
      }
      catch (EScpSkipFile & E)
      {
        SUSPEND_OPERATION (
          if (!FTerminal->HandleException(&E)) throw;
        );
      }
      catch (...)
      {
        // TODO: remove the block?
        throw;
      }
    }
    ++Index;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSinkRobust(const UnicodeString & FileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags)
{
  // the same in TFTPFileSystem
  bool Retry;

  TDownloadSessionAction Action(FTerminal->GetActionLog());

  do
  {
    Retry = false;
    bool ChildError = false;
    try
    {
      SFTPSink(FileName, AFile, TargetDir, CopyParam, Params, OperationProgress,
        Flags, Action, ChildError);
    }
    catch (Exception & E)
    {
      Retry = true;
      if (FTerminal->GetActive() ||
          !FTerminal->QueryReopen(&E, ropNoReadDirectory, OperationProgress))
      {
        if (!ChildError)
        {
          FTerminal->RollbackAction(Action, OperationProgress, &E);
        }
        throw;
      }
    }

    if (Retry)
    {
      OperationProgress->RollbackTransfer();
      Action.Restart();
      assert(AFile != nullptr);
      if (!AFile->GetIsDirectory())
      {
        // prevent overwrite and resume confirmations
        Params |= cpNoConfirmation;
      }
    }
  }
  while (Retry);
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSink(const UnicodeString & FileName,
  const TRemoteFile * AFile, const UnicodeString & TargetDir,
  const TCopyParamType * CopyParam, intptr_t Params,
  TFileOperationProgressType * OperationProgress, uintptr_t Flags,
  TDownloadSessionAction & Action, bool & ChildError)
{

  Action.FileName(FileName);

  UnicodeString OnlyFileName = ::UnixExtractFileName(FileName);

  TFileMasks::TParams MaskParams;
  assert(AFile);
  MaskParams.Size = AFile->GetSize();
  MaskParams.Modification = AFile->GetModification();

  if (!CopyParam->AllowTransfer(FileName, osRemote, AFile->GetIsDirectory(), MaskParams))
  {
    FTerminal->LogEvent(FORMAT(L"File \"%s\" excluded from transfer", FileName.c_str()));
    ThrowSkipFileNull();
  }

  FTerminal->LogFileDetails(FileName, AFile->GetModification(), AFile->GetSize());

  OperationProgress->SetFile(OnlyFileName);

  UnicodeString DestFileName = CopyParam->ChangeFileName(UnixExtractFileName(AFile->GetFileName()),
    osRemote, FLAGSET(Flags, tfFirstLevel));
  UnicodeString DestFullName = TargetDir + DestFileName;

  if (AFile->GetIsDirectory())
  {
    Action.Cancel();
    if (!AFile->GetIsSymLink())
    {
      FILE_OPERATION_LOOP (FMTLOAD(NOT_DIRECTORY_ERROR, DestFullName.c_str()),
        DWORD LocalFileAttrs = FTerminal->GetLocalFileAttributes(DestFullName);
        if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && (LocalFileAttrs & faDirectory) == 0)
        {
          ThrowExtException();
        }
      );

      FILE_OPERATION_LOOP (FMTLOAD(CREATE_DIR_ERROR, DestFullName.c_str()),
        THROWOSIFFALSE(ForceDirectories(DestFullName));
      );

      TSinkFileParams SinkFileParams;
      SinkFileParams.TargetDir = IncludeTrailingBackslash(DestFullName);
      SinkFileParams.CopyParam = CopyParam;
      SinkFileParams.Params = Params;
      SinkFileParams.OperationProgress = OperationProgress;
      SinkFileParams.Skipped = false;
      SinkFileParams.Flags = Flags & ~tfFirstLevel;

      FTerminal->ProcessDirectory(FileName, MAKE_CALLBACK(TSFTPFileSystem::SFTPSinkFile, this), &SinkFileParams);

      // Do not delete directory if some of its files were skip.
      // Throw "skip file" for the directory to avoid attempt to deletion
      // of any parent directory
      if ((Params & cpDelete) && SinkFileParams.Skipped)
      {
        ThrowSkipFileNull();
      }
    }
    else
    {
      // file is symlink to directory, currently do nothing, but it should be
      // reported to user
    }
  }
  else
  {
    FTerminal->LogEvent(FORMAT(L"Copying \"%s\" to local directory started.", FileName.c_str()));

    UnicodeString DestPartialFullName;
    bool ResumeAllowed;
    bool ResumeTransfer = false;
    __int64 ResumeOffset;

    // Will we use ASCII of BINARY file transfer?
    OperationProgress->SetAsciiTransfer(
      CopyParam->UseAsciiTransfer(FileName, osRemote, MaskParams));
    FTerminal->LogEvent(UnicodeString((OperationProgress->AsciiTransfer ? L"Ascii" : L"Binary")) +
      L" transfer mode selected.");

    // Suppose same data size to transfer as to write
    // (not true with ASCII transfer)
    OperationProgress->SetTransferSize(AFile->GetSize());
    OperationProgress->SetLocalSize(OperationProgress->TransferSize);

    // resume has no sense for temporary downloads
    ResumeAllowed = ((Params & cpTemporary) == 0) &&
      !OperationProgress->AsciiTransfer &&
      CopyParam->AllowResume(OperationProgress->TransferSize);
    OperationProgress->SetResumeStatus(ResumeAllowed ? rsEnabled : rsDisabled);

    DWORD LocalFileAttrs = INVALID_FILE_ATTRIBUTES;
    FILE_OPERATION_LOOP (FMTLOAD(NOT_FILE_ERROR, DestFullName.c_str()),
      LocalFileAttrs = FTerminal->GetLocalFileAttributes(DestFullName);
      if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && (LocalFileAttrs & faDirectory))
      {
        ThrowExtException();
      }
    );

    OperationProgress->TransferingFile = false; // not set with SFTP protocol

    HANDLE LocalFileHandle = INVALID_HANDLE_VALUE;
    TStream * FileStream = nullptr;
    bool DeleteLocalFile = false;
    RawByteString RemoteHandle;
    UnicodeString LocalFileName = DestFullName;
    TOverwriteMode OverwriteMode = omOverwrite;
    {
      SCOPE_EXIT
      {
        if (LocalFileHandle != INVALID_HANDLE_VALUE)
        {
          ::CloseHandle(LocalFileHandle);
        }
        if (FileStream)
        {
          SAFE_DESTROY(FileStream);
        }
        if (DeleteLocalFile && (!ResumeAllowed || OperationProgress->LocallyUsed == 0) &&
            (OverwriteMode == omOverwrite))
        {
          FILE_OPERATION_LOOP (FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, LocalFileName.c_str()),
            THROWOSIFFALSE(Sysutils::DeleteFile(LocalFileName));
          )
        }

        // if the transfer was finished, the file is closed already
        if (FTerminal->GetActive() && !RemoteHandle.IsEmpty())
        {
          // do not wait for response
          SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress,
            true, true, nullptr);
        }
      };
      if (ResumeAllowed)
      {
        DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
        LocalFileName = DestPartialFullName;

        FTerminal->LogEvent(L"Checking existence of partially transfered file.");
        if (FileExists(DestPartialFullName))
        {
          FTerminal->LogEvent(L"Partially transfered file exists.");
          FTerminal->OpenLocalFile(DestPartialFullName, GENERIC_WRITE,
            nullptr, &LocalFileHandle, nullptr, nullptr, nullptr, &ResumeOffset);

          bool PartialBiggerThanSource = (ResumeOffset > OperationProgress->TransferSize);
          if (FLAGCLEAR(Params, cpNoConfirmation))
          {
            ResumeTransfer = SFTPConfirmResume(DestFileName,
              PartialBiggerThanSource, OperationProgress);
          }
          else
          {
            ResumeTransfer = !PartialBiggerThanSource;
            if (!ResumeTransfer)
            {
              FTerminal->LogEvent(L"Partially transfered file is bigger that original file.");
            }
          }

          if (!ResumeTransfer)
          {
            ::CloseHandle(LocalFileHandle);
            LocalFileHandle = INVALID_HANDLE_VALUE;
            FILE_OPERATION_LOOP (FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestPartialFullName.c_str()),
              THROWOSIFFALSE(Sysutils::DeleteFile(DestPartialFullName));
            )
          }
          else
          {
            FTerminal->LogEvent(L"Resuming file transfer.");
            FileSeek(LocalFileHandle, ResumeOffset, 0);
            OperationProgress->AddResumed(ResumeOffset);
          }
        }
      }

      // first open source file, not to loose the destination file,
      // if we cannot open the source one in the first place
      FTerminal->LogEvent(L"Opening remote file.");
      FILE_OPERATION_LOOP (FMTLOAD(SFTP_OPEN_FILE_ERROR, FileName.c_str()),
        int OpenType = SSH_FXF_READ;
        if ((FVersion >= 4) && OperationProgress->AsciiTransfer)
        {
          OpenType |= SSH_FXF_TEXT;
        }
        RemoteHandle = SFTPOpenRemoteFile(FileName, OpenType);
      );

      TDateTime Modification(0.0);
      FILETIME AcTime;
      ClearStruct(AcTime);
      FILETIME WrTime;
      ClearStruct(WrTime);

      TSFTPPacket RemoteFilePacket(SSH_FXP_FSTAT, FCodePage);
      RemoteFilePacket.AddString(RemoteHandle);
      SendCustomReadFile(&RemoteFilePacket, &RemoteFilePacket,
        SSH_FILEXFER_ATTR_MODIFYTIME);
      ReceiveResponse(&RemoteFilePacket, &RemoteFilePacket);

      const TRemoteFile * File = AFile;
      std::unique_ptr<const TRemoteFile> FilePtr(nullptr);
      // ignore errors
      if (RemoteFilePacket.GetType() == SSH_FXP_ATTRS)
      {
        // load file, avoid completion (resolving symlinks) as we do not need that
        File = LoadFile(&RemoteFilePacket, nullptr, ::UnixExtractFileName(FileName),
          nullptr, false);
        FilePtr.reset(File);
      }

      Modification = File->GetModification();
      AcTime = DateTimeToFileTime(File->GetLastAccess(),
        FTerminal->GetSessionData()->GetDSTMode());
      WrTime = DateTimeToFileTime(File->GetModification(),
        FTerminal->GetSessionData()->GetDSTMode());

      if ((LocalFileAttrs != INVALID_FILE_ATTRIBUTES) && !ResumeTransfer)
      {
        __int64 DestFileSize = 0;
        __int64 MTime = 0;
        FTerminal->OpenLocalFile(DestFullName, GENERIC_WRITE,
          nullptr, &LocalFileHandle, nullptr, &MTime, nullptr, &DestFileSize, false);

        FTerminal->LogEvent(L"Confirming overwriting of file.");
        TOverwriteFileParams FileParams;
        FileParams.SourceSize = OperationProgress->TransferSize;
        FileParams.SourceTimestamp = AFile->GetModification();
        FileParams.DestTimestamp = ::UnixToDateTime(MTime,
          GetSessionData()->GetDSTMode());
        FileParams.DestSize = DestFileSize;
        UnicodeString PrevDestFileName = DestFileName;
        SFTPConfirmOverwrite(DestFileName, CopyParam, Params, OperationProgress, OverwriteMode, &FileParams);
        if (PrevDestFileName != DestFileName)
        {
          DestFullName = TargetDir + DestFileName;
          DestPartialFullName = DestFullName + FTerminal->GetConfiguration()->GetPartialExt();
          if (ResumeAllowed)
          {
            if (FileExists(DestPartialFullName))
            {
              FILE_OPERATION_LOOP (FMTLOAD(CORE_DELETE_LOCAL_FILE_ERROR, DestPartialFullName.c_str()),
                THROWOSIFFALSE(Sysutils::DeleteFile(DestPartialFullName));
              )
            }
            LocalFileName = DestPartialFullName;
          }
          else
          {
            LocalFileName = DestFullName;
          }
        }

        if (OverwriteMode == omOverwrite)
        {
          // is nullptr when overwriting read-only file
          if (LocalFileHandle != INVALID_HANDLE_VALUE)
          {
            ::CloseHandle(LocalFileHandle);
            LocalFileHandle = INVALID_HANDLE_VALUE;
          }
        }
        else
        {
          // is nullptr when overwriting read-only file, so following will
          // probably fail anyway
          if (LocalFileHandle == INVALID_HANDLE_VALUE)
          {
            FTerminal->OpenLocalFile(DestFullName, GENERIC_WRITE,
              nullptr, &LocalFileHandle, nullptr, nullptr, nullptr, nullptr);
          }
          ResumeAllowed = false;
          FileSeek(LocalFileHandle, DestFileSize, 0);
          if (OverwriteMode == omAppend)
          {
            FTerminal->LogEvent(L"Appending to file.");
          }
          else
          {
            FTerminal->LogEvent(L"Resuming file transfer (append style).");
            assert(OverwriteMode == omResume);
            OperationProgress->AddResumed(DestFileSize);
          }
        }
      }

      Action.Destination(ExpandUNCFileName(DestFullName));

      // if not already opened (resume, append...), create new empty file
      if (LocalFileHandle == INVALID_HANDLE_VALUE)
      {
        if (!FTerminal->CreateLocalFile(LocalFileName, OperationProgress,
            &LocalFileHandle, FLAGSET(Params, cpNoConfirmation)))
        {
          ThrowSkipFileNull();
        }
      }
      assert(LocalFileHandle != INVALID_HANDLE_VALUE);

      DeleteLocalFile = true;

      FileStream = new TSafeHandleStream(LocalFileHandle);

      // at end of this block queue is discarded
      {
        TSFTPDownloadQueue Queue(this, FCodePage);
        SCOPE_EXIT
        {
          Queue.DisposeSafe();
        };
        {
          TSFTPPacket DataPacket(FCodePage);

          uintptr_t BlSize = DownloadBlockSize(OperationProgress);
          intptr_t QueueLen = intptr_t(AFile->GetSize() / (BlSize != 0 ? BlSize : 1)) + 1;
          if ((QueueLen > GetSessionData()->GetSFTPDownloadQueue()) ||
              (QueueLen < 0))
          {
            QueueLen = GetSessionData()->GetSFTPDownloadQueue();
          }
          if (QueueLen < 1)
          {
            QueueLen = 1;
          }
          Queue.Init(QueueLen, RemoteHandle, OperationProgress->TransferedSize,
            OperationProgress);

          bool Eof = false;
          bool PrevIncomplete = false;
          int32_t GapFillCount = 0;
          int32_t GapCount = 0;
          uint32_t Missing = 0;
          uint32_t DataLen = 0;
          uintptr_t BlockSize = 0;
          bool ConvertToken = false;

          while (!Eof)
          {
            if (Missing > 0)
            {
              Queue.InitFillGapRequest(OperationProgress->TransferedSize, Missing,
                &DataPacket);
              GapFillCount++;
              SendPacketAndReceiveResponse(&DataPacket, &DataPacket,
                SSH_FXP_DATA, asEOF);
            }
            else
            {
              Queue.ReceivePacket(&DataPacket, BlockSize);
            }

            if (DataPacket.GetType() == SSH_FXP_STATUS)
            {
              // must be SSH_FX_EOF, any other status packet would raise exception
              Eof = true;
              // close file right away, before waiting for pending responses
              SFTPCloseRemote(RemoteHandle, DestFileName, OperationProgress,
                true, true, nullptr);
              RemoteHandle = L""; // do not close file again in __finally block
            }

            if (!Eof)
            {
              if ((Missing == 0) && PrevIncomplete)
              {
                // This can happen only if last request returned less bytes
                // than expected, but exactly number of bytes missing to last
                // known file size, but actually EOF was not reached.
                // Can happen only when filesize has changed since directory
                // listing and server returns less bytes than requested and
                // file has some special file size.
                FTerminal->LogEvent(FORMAT(
                  L"Received incomplete data packet before end of file, "
                  L"offset: %s, size: %d, requested: %d",
                  Int64ToStr(OperationProgress->TransferedSize).c_str(), static_cast<int>(DataLen),
                  static_cast<int>(BlockSize)));
                FTerminal->TerminalError(nullptr, LoadStr(SFTP_INCOMPLETE_BEFORE_EOF));
              }

              // Buffer for one block of data
              TFileBuffer BlockBuf;

              DataLen = DataPacket.GetCardinal();

              PrevIncomplete = false;
              if (Missing > 0)
              {
                assert(DataLen <= Missing);
                Missing -= DataLen;
              }
              else if (DataLen < BlockSize)
              {
                if (OperationProgress->TransferedSize + static_cast<__int64>(DataLen) !=
                      OperationProgress->TransferSize)
                {
                  // with native text transfer mode (SFTP>=4), do not bother about
                  // getting less than requested, read offset is ignored anyway
                  if ((FVersion < 4) || !OperationProgress->AsciiTransfer)
                  {
                    GapCount++;
                    Missing = static_cast<uint32_t>(BlockSize - DataLen);
                  }
                }
                else
                {
                  PrevIncomplete = true;
                }
              }

              assert(DataLen <= BlockSize);
              BlockBuf.Insert(0, reinterpret_cast<const char *>(DataPacket.GetNextData(DataLen)), DataLen);
              OperationProgress->AddTransfered(DataLen);

              if (OperationProgress->AsciiTransfer)
              {
                assert(!ResumeTransfer && !ResumeAllowed);

                __int64 PrevBlockSize = BlockBuf.GetSize();
                BlockBuf.Convert(GetEOL(), FTerminal->GetConfiguration()->GetLocalEOLType(), 0, ConvertToken);
                OperationProgress->SetLocalSize(
                  OperationProgress->LocalSize - PrevBlockSize + BlockBuf.GetSize());
              }

              FILE_OPERATION_LOOP (FMTLOAD(WRITE_ERROR, LocalFileName.c_str()),
                BlockBuf.WriteToStream(FileStream, BlockBuf.GetSize());
              );

              OperationProgress->AddLocallyUsed(BlockBuf.GetSize());
            }

            if (OperationProgress->Cancel == csCancel)
            {
              Abort();
            }
          }

          if (GapCount > 0)
          {
            FTerminal->LogEvent(FORMAT(
              L"%d requests to fill %d data gaps were issued.",
              GapFillCount, GapCount));
          }
        }
        // queue is discarded here
      }

      if (CopyParam->GetPreserveTime())
      {
        FTerminal->LogEvent(FORMAT(L"Preserving timestamp [%s]",
          StandardTimestamp(Modification).c_str()));
        SetFileTime(LocalFileHandle, nullptr, &AcTime, &WrTime);
      }

      ::CloseHandle(LocalFileHandle);
      LocalFileHandle = INVALID_HANDLE_VALUE;

      if (ResumeAllowed)
      {
        FILE_OPERATION_LOOP (FMTLOAD(RENAME_AFTER_RESUME_ERROR,
            ExtractFileName(DestPartialFullName, true).c_str(), DestFileName.c_str()),

          if (FileExists(DestFullName))
          {
            DeleteFileChecked(DestFullName);
          }
          THROWOSIFFALSE(Sysutils::RenameFile(DestPartialFullName, DestFullName));
        );
      }

      DeleteLocalFile = false;

      if (LocalFileAttrs == INVALID_FILE_ATTRIBUTES)
      {
        LocalFileAttrs = faArchive;
      }
      DWORD NewAttrs = CopyParam->LocalFileAttrs(*AFile->GetRights());
      if ((NewAttrs & LocalFileAttrs) != NewAttrs)
      {
        FILE_OPERATION_LOOP (FMTLOAD(CANT_SET_ATTRS, DestFullName.c_str()),
          THROWOSIFFALSE(FTerminal->SetLocalFileAttributes(DestFullName, LocalFileAttrs | NewAttrs) == 0);
        );
      }
    }
  }

  if (Params & cpDelete)
  {
    ChildError = true;
    // If file is directory, do not delete it recursively, because it should be
    // empty already. If not, it should not be deleted (some files were
    // skipped or some new files were copied to it, while we were downloading)
    intptr_t Params2 = dfNoRecursive;
    FTerminal->DeleteFile(FileName, AFile, &Params2);
    ChildError = false;
  }
}
//---------------------------------------------------------------------------
void TSFTPFileSystem::SFTPSinkFile(const UnicodeString & FileName,
  const TRemoteFile * AFile, void * Param)
{
  TSinkFileParams * Params = static_cast<TSinkFileParams *>(Param);
  assert(Params->OperationProgress);
  try
  {
    SFTPSinkRobust(FileName, AFile, Params->TargetDir, Params->CopyParam,
      Params->Params, Params->OperationProgress, Params->Flags);
  }
  catch (EScpSkipFile & E)
  {
    TFileOperationProgressType * OperationProgress = Params->OperationProgress;

    Params->Skipped = true;

    SUSPEND_OPERATION (
      if (!FTerminal->HandleException(&E)) throw;
    );

    if (OperationProgress->Cancel)
    {
      Abort();
    }
  }
}
