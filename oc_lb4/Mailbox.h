#pragma once

#include <windows.h>
#include <tchar.h>

struct MailboxHeader
{
    DWORD messageCount;
    DWORD totalMessagesSize;
    DWORD maxMailboxSize;
};

struct MessageHeader
{
    DWORD messageSize;
};

class Mailbox
{
private:
    TCHAR directoryName[MAX_PATH];
    TCHAR fileName[MAX_PATH];
    TCHAR fullPath[MAX_PATH];

    BOOL ReadHeader(MailboxHeader* header);
    BOOL WriteHeader(const MailboxHeader* header);

    DWORD CalculateChecksum(DWORD bytesToRead);
    BOOL UpdateChecksum();

public:
    Mailbox(LPCTSTR directory, LPCTSTR file);

    BOOL Create(DWORD maxSize);

    BOOL AddMessage(LPCTSTR message);
    BOOL ReadMessage(DWORD number, BOOL deleteAfterRead);
    BOOL DeleteMessage(DWORD number);
    BOOL DeleteAllMessages();

    BOOL VerifyChecksum();
    BOOL CheckIntegrityWithChoice();

    DWORD GetMessageCount();
    static DWORD GetMailboxCount(LPCTSTR directory);
};