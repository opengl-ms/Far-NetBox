//---------------------------------------------------------------------------
#ifndef InterfaceH
#define InterfaceH
//---------------------------------------------------------------------------
#include <CoreDefs.hpp>

#include "Configuration.h"
#include "SessionData.h"
#define HELP_NONE ""
//---------------------------------------------------------------------------
TConfiguration * CreateConfiguration();

void ShowExtendedException(Exception * E);

UnicodeString GetRegistryKey();
void * BusyStart();
void BusyEnd(void * Token);
UnicodeString AppNameString();
UnicodeString SshVersionString();
void CopyToClipboard(const UnicodeString & Text);
HANDLE StartThread(void * SecurityAttributes, DWORD StackSize,
  /* TThreadFunc ThreadFunc, */ void * Parameter, DWORD CreationFlags,
  TThreadID & ThreadId);

// Order of the values also define order of the buttons/answers on the prompts
// MessageDlg relies on these to be <= 0x0000FFFF
const unsigned int qaYes =      0x00000001;
// MessageDlg relies that answer do not conflict with mrCancel (=0x2)
const unsigned int qaNo =       0x00000004;
const unsigned int qaOK =       0x00000008;
const unsigned int qaCancel =   0x00000010;
const unsigned int qaYesToAll = 0x00000020;
const unsigned int qaNoToAll =  0x00000040;
const unsigned int qaAbort =    0x00000080;
const unsigned int qaRetry =    0x00000100;
const unsigned int qaIgnore =   0x00000200;
const unsigned int qaSkip =     0x00000400;
const unsigned int qaAll =      0x00000800;
const unsigned int qaHelp =     0x00001000;
const unsigned int qaReport =   0x00002000;

const unsigned int qaFirst = qaYes;
const unsigned int qaLast = qaReport;

const unsigned int qaNeverAskAgain = 0x00010000;

const int qpFatalAbort =           0x01;
const int qpNeverAskAgainCheck =   0x02;
const int qpAllowContinueOnError = 0x04;
const int qpIgnoreAbort =          0x08;

struct TQueryButtonAlias : public TObject
{
  TQueryButtonAlias();

  uintptr_t Button;
  UnicodeString Alias;
  TNotifyEvent OnClick;
  int GroupWith;
  TShiftState GrouppedShiftState;
};

DEFINE_CALLBACK_TYPE1(TQueryParamsTimerEvent, void,
  intptr_t & /* Result */);
enum TQueryType { qtConfirmation, qtWarning, qtError, qtInformation };

struct TQueryParams : public TObject
{
  explicit TQueryParams(uintptr_t AParams = 0, const UnicodeString & AHelpKeyword = HELP_NONE);
  explicit TQueryParams(const TQueryParams & Source);

  void Assign(const TQueryParams & Source);

  const TQueryButtonAlias * Aliases;
  uintptr_t AliasesCount;
  uintptr_t Params;
  uintptr_t Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  uintptr_t TimerAnswers;
  TQueryType TimerQueryType;
  uintptr_t Timeout;
  uintptr_t TimeoutAnswer;
  uintptr_t NoBatchAnswers;
  UnicodeString HelpKeyword;

private:
  // NB_DISABLE_COPY(TQueryParams)
};

enum TPromptKind
{
  pkPrompt,
  pkFileName,
  pkUserName,
  pkPassphrase,
  pkTIS,
  pkCryptoCard,
  pkKeybInteractive,
  pkPassword,
  pkNewPassword
};

enum TPromptUserParam { pupEcho = 0x01, pupRemember = 0x02 };

bool IsAuthenticationPrompt(TPromptKind Kind);
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE4(TFileFoundEvent, void,
  TTerminal * /* Terminal */, const UnicodeString & /* FileName */, const TRemoteFile * /* File */,
  bool & /* Cancel */);
DEFINE_CALLBACK_TYPE3(TFindingFileEvent, void,
  TTerminal * /* Terminal */, const UnicodeString & /* Directory */, bool & /* Cancel */);
//---------------------------------------------------------------------------
class TOperationVisualizer
{
public:
  TOperationVisualizer(bool UseBusyCursor = true);
  ~TOperationVisualizer();

private:
  bool FUseBusyCursor;
  void * FToken;
};
//---------------------------------------------------------------------------
class TInstantOperationVisualizer : public TOperationVisualizer
{
public:
  TInstantOperationVisualizer();
  ~TInstantOperationVisualizer();

private:
  TDateTime FStart;
};
//---------------------------------------------------------------------------
#endif
