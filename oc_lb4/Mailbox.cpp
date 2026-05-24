#include "Mailbox.h"
#include <iostream>

using namespace std;

Mailbox::Mailbox(LPCTSTR directory, LPCTSTR file)
{
    _tcscpy_s(directoryName, directory);
    _tcscpy_s(fileName, file);

    _stprintf_s(fullPath, MAX_PATH, _T("%s\\%s"), directoryName, fileName);
}

BOOL Mailbox::Create(DWORD maxSize)
{
    CreateDirectory(directoryName, NULL);

    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_EXISTS)
            _tprintf(_T("A mailbox already exists.\n"));
        else
            _tprintf(_T("Error creating mailbox file.\n"));

        return FALSE;
    }


    MailboxHeader header;
    header.messageCount = 0;
    header.totalMessagesSize = 0;
    header.maxMailboxSize = maxSize;

    DWORD bytesWritten;

    BOOL result = WriteFile(
        hFile,
        &header,
        sizeof(MailboxHeader),
        &bytesWritten,
        NULL
    );

    CloseHandle(hFile);

    if (!result || bytesWritten != sizeof(MailboxHeader))
    {
        _tprintf(_T("Error writing mailbox header.\n"));
        return FALSE;
    }

    if (!UpdateChecksum())
    {
        _tprintf(_T("Error writing checksum.\n"));
        return FALSE;
    }

    _tprintf(_T("Mailbox was created successfully.\n"));
    return TRUE;
}

BOOL Mailbox::ReadHeader(MailboxHeader* header)
{
    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD bytesRead;

    BOOL result = ReadFile(
        hFile,
        header,
        sizeof(MailboxHeader),
        &bytesRead,
        NULL
    );

    CloseHandle(hFile);

    return result && bytesRead == sizeof(MailboxHeader);
}

BOOL Mailbox::WriteHeader(const MailboxHeader* header)
{
    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD bytesWritten;

    BOOL result = WriteFile(
        hFile,
        header,
        sizeof(MailboxHeader),
        &bytesWritten,
        NULL
    );

    CloseHandle(hFile);

    return result && bytesWritten == sizeof(MailboxHeader);
}

DWORD Mailbox::CalculateChecksum(DWORD bytesToRead)
{
    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    BYTE* buffer = new BYTE[bytesToRead];
    DWORD bytesRead;

    BOOL result = ReadFile(
        hFile,
        buffer,
        bytesToRead,
        &bytesRead,
        NULL
    );

    CloseHandle(hFile);

    if (!result || bytesRead != bytesToRead)
    {
        delete[] buffer;
        return 0;
    }

    DWORD checksum = 0;

    for (DWORD i = 0; i < bytesToRead; i++)
    {
        checksum += buffer[i];
    }

    delete[] buffer;
    return checksum;
}

BOOL Mailbox::UpdateChecksum()
{
    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD fileSize = GetFileSize(hFile, NULL);
    CloseHandle(hFile);

    DWORD checksum = CalculateChecksum(fileSize);

    hFile = CreateFile(
        fullPath,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    SetFilePointer(hFile, 0, NULL, FILE_END);

    DWORD bytesWritten;

    BOOL result = WriteFile(
        hFile,
        &checksum,
        sizeof(DWORD),
        &bytesWritten,
        NULL
    );

    CloseHandle(hFile);

    return result && bytesWritten == sizeof(DWORD);
}

BOOL Mailbox::VerifyChecksum()
{
    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD fileSize = GetFileSize(hFile, NULL);

    if (fileSize <= sizeof(DWORD))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    SetFilePointer(hFile, fileSize - sizeof(DWORD), NULL, FILE_BEGIN);

    DWORD savedChecksum;
    DWORD bytesRead;

    BOOL result = ReadFile(
        hFile,
        &savedChecksum,
        sizeof(DWORD),
        &bytesRead,
        NULL
    );

    CloseHandle(hFile);

    if (!result || bytesRead != sizeof(DWORD))
        return FALSE;

    DWORD calculatedChecksum = CalculateChecksum(fileSize - sizeof(DWORD));

    if (savedChecksum == calculatedChecksum)
    {
        _tprintf(_T("Mailbox integrity check passed.\n"));
        return TRUE;
    }
    else
    {
        _tprintf(_T("Mailbox integrity violation detected.\n"));
        return FALSE;
    }
}

BOOL Mailbox::CheckIntegrityWithChoice()
{
    if (VerifyChecksum())
    {
        return TRUE;
    }

    int choice;

    _tprintf(_T("\nIntegrity violation detected.\n"));
    _tprintf(_T("1 - Block access to mailbox\n"));
    _tprintf(_T("2 - Continue working anyway\n"));
    _tprintf(_T("Your choice: "));

    _tscanf_s(_T("%d"), &choice);

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}

    if (choice == 1)
    {
        _tprintf(_T("Access to mailbox is blocked.\n"));
        return FALSE;
    }

    if (choice == 2)
    {
        _tprintf(
            _T("Warning: mailbox integrity is corrupted.\n")
            _T("You chose to continue working with damaged mailbox.\n")
            _T("Reading, deleting and adding messages is still allowed,\n")
            _T("but mailbox data may be incorrect or partially damaged.\n")
        );

        return TRUE;
    }

    _tprintf(_T("Invalid choice. Access is blocked.\n"));
    return FALSE;
}

BOOL Mailbox::AddMessage(LPCTSTR message)
{
    MailboxHeader header;

    if (!ReadHeader(&header))
    {
        _tprintf(_T("Cannot read mailbox header.\n"));
        return FALSE;
    }

    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        _tprintf(_T("Cannot open mailbox file.\n"));
        return FALSE;
    }

    if (_tcslen(message) == 0)
    {
        _tprintf(_T("Empty message is not allowed.\n"));
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD fileSizeBefore = GetFileSize(hFile, NULL);

    DWORD messageSize =
        (DWORD)((_tcslen(message) + 1) * sizeof(TCHAR));

    DWORD newDataSize =
        sizeof(MessageHeader) + messageSize;

    DWORD currentFileSize =
        GetFileSize(hFile, NULL);

    if (currentFileSize - sizeof(DWORD) + newDataSize > header.maxMailboxSize)
    {
        _tprintf(_T("\nMessage was not added. Maximum mailbox size exceeded.\n"));
        CloseHandle(hFile);
        return FALSE;
    }

    SetFilePointer(
        hFile,
        fileSizeBefore - sizeof(DWORD),
        NULL,
        FILE_BEGIN
    );

    SetEndOfFile(hFile);
    SetFilePointer(hFile, 0, NULL, FILE_END);

    MessageHeader msgHeader;
    msgHeader.messageSize = messageSize;

    DWORD bytesWritten;

    BOOL result = WriteFile(
        hFile,
        &msgHeader,
        sizeof(MessageHeader),
        &bytesWritten,
        NULL
    );

    if (result && bytesWritten == sizeof(MessageHeader))
    {
        result = WriteFile(
            hFile,
            message,
            messageSize,
            &bytesWritten,
            NULL
        );
    }

    CloseHandle(hFile);

    if (!result || bytesWritten != messageSize)
    {
        _tprintf(_T("Error writing message.\n"));
        return FALSE;
    }

    header.messageCount++;
    header.totalMessagesSize += newDataSize;

    if (!WriteHeader(&header))
    {
        _tprintf(_T("Error updating mailbox header.\n"));
        return FALSE;
    }

    if (!UpdateChecksum())
    {
        _tprintf(_T("Error updating checksum.\n"));
        return FALSE;
    }

    _tprintf(_T("Message was added successfully.\n"));
    return TRUE;
}

BOOL Mailbox::ReadMessage(DWORD number, BOOL deleteAfterRead)
{
    MailboxHeader header;

    if (!ReadHeader(&header))
    {
        _tprintf(_T("Cannot read mailbox header.\n"));
        return FALSE;
    }

    if (number < 1 || number > header.messageCount)
    {
        _tprintf(_T("Invalid message number.\n"));
        return FALSE;
    }

    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    SetFilePointer(hFile, sizeof(MailboxHeader), NULL, FILE_BEGIN);

    MessageHeader msgHeader;
    DWORD bytesRead;

    for (DWORD i = 1; i <= header.messageCount; i++)
    {
        BOOL result = ReadFile(
            hFile,
            &msgHeader,
            sizeof(MessageHeader),
            &bytesRead,
            NULL
        );

        if (!result || bytesRead != sizeof(MessageHeader))
        {
            CloseHandle(hFile);
            return FALSE;
        }

        if (i == number)
        {
            TCHAR* buffer = new TCHAR[msgHeader.messageSize / sizeof(TCHAR)];

            result = ReadFile(
                hFile,
                buffer,
                msgHeader.messageSize,
                &bytesRead,
                NULL
            );

            if (result && bytesRead == msgHeader.messageSize)
            {
                _tprintf(_T("Message %lu: %s\n"), number, buffer);
            }
            else
            {
                _tprintf(_T("Error reading message body.\n"));
                delete[] buffer;
                CloseHandle(hFile);
                return FALSE;
            }

            delete[] buffer;
            CloseHandle(hFile);

            if (deleteAfterRead)
            {
                return DeleteMessage(number);
            }

            return TRUE;
        }

        SetFilePointer(hFile, msgHeader.messageSize, NULL, FILE_CURRENT);
    }

    CloseHandle(hFile);
    return FALSE;
}

BOOL Mailbox::DeleteMessage(DWORD number)
{
    MailboxHeader header;

    if (!ReadHeader(&header))
    {
        _tprintf(_T("Cannot read mailbox header.\n"));
        return FALSE;
    }

    if (number < 1 || number > header.messageCount)
    {
        _tprintf(_T("Invalid message number.\n"));
        return FALSE;
    }

    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    SetFilePointer(hFile, sizeof(MailboxHeader), NULL, FILE_BEGIN);

    DWORD deleteStart = 0;
    DWORD deleteEnd = 0;
    DWORD currentPos = sizeof(MailboxHeader);

    MessageHeader msgHeader;
    DWORD bytesRead;

    for (DWORD i = 1; i <= header.messageCount; i++)
    {
        BOOL result = ReadFile(
            hFile,
            &msgHeader,
            sizeof(MessageHeader),
            &bytesRead,
            NULL
        );

        if (!result || bytesRead != sizeof(MessageHeader))
        {
            CloseHandle(hFile);
            return FALSE;
        }

        if (i == number)
        {
            deleteStart = currentPos;
            deleteEnd = currentPos + sizeof(MessageHeader) + msgHeader.messageSize;
            break;
        }

        currentPos += sizeof(MessageHeader) + msgHeader.messageSize;
        SetFilePointer(hFile, msgHeader.messageSize, NULL, FILE_CURRENT);
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    DWORD dataSizeWithoutChecksum = fileSize - sizeof(DWORD);
    DWORD tailSize = dataSizeWithoutChecksum - deleteEnd;

    if (tailSize > 0)
    {
        BYTE* tailBuffer = new BYTE[tailSize];

        SetFilePointer(hFile, deleteEnd, NULL, FILE_BEGIN);

        BOOL readResult = ReadFile(
            hFile,
            tailBuffer,
            tailSize,
            &bytesRead,
            NULL
        );

        if (!readResult || bytesRead != tailSize)
        {
            delete[] tailBuffer;
            CloseHandle(hFile);
            return FALSE;
        }

        SetFilePointer(hFile, deleteStart, NULL, FILE_BEGIN);

        DWORD bytesWritten;

        BOOL writeResult = WriteFile(
            hFile,
            tailBuffer,
            tailSize,
            &bytesWritten,
            NULL
        );

        delete[] tailBuffer;

        if (!writeResult || bytesWritten != tailSize)
        {
            CloseHandle(hFile);
            return FALSE;
        }
    }

    SetFilePointer(hFile, deleteStart + tailSize, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    CloseHandle(hFile);

    header.messageCount--;
    header.totalMessagesSize -= deleteEnd - deleteStart;

    if (!WriteHeader(&header))
    {
        _tprintf(_T("Error updating mailbox header.\n"));
        return FALSE;
    }

    if (!UpdateChecksum())
    {
        _tprintf(_T("Error updating checksum.\n"));
        return FALSE;
    }
    _tprintf(_T("Message was deleted successfully.\n"));
    return TRUE;
}

BOOL Mailbox::DeleteAllMessages()
{
    MailboxHeader header;

    if (!ReadHeader(&header))
    {
        _tprintf(_T("Cannot read mailbox header.\n"));
        return FALSE;
    }

    header.messageCount = 0;
    header.totalMessagesSize = 0;

    HANDLE hFile = CreateFile(
        fullPath,
        GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD bytesWritten;

    BOOL result = WriteFile(
        hFile,
        &header,
        sizeof(MailboxHeader),
        &bytesWritten,
        NULL
    );

    if (!result || bytesWritten != sizeof(MailboxHeader))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    SetFilePointer(hFile, sizeof(MailboxHeader), NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    CloseHandle(hFile);

    if (!UpdateChecksum())
    {
        _tprintf(_T("Error updating checksum.\n"));
        return FALSE;
    }

    _tprintf(_T("All messages were deleted.\n"));
    return TRUE;
}

DWORD Mailbox::GetMessageCount()
{
    MailboxHeader header;

    if (!ReadHeader(&header))
        return 0;

    return header.messageCount;
}

DWORD Mailbox::GetMailboxCount(LPCTSTR directory)
{
    TCHAR searchPath[MAX_PATH];

    _stprintf_s(searchPath, MAX_PATH, _T("%s\\*.mbx"), directory);

    WIN32_FIND_DATA findData;

    HANDLE hFind = FindFirstFile(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    DWORD count = 0;

    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            count++;
        }

    } while (FindNextFile(hFind, &findData));

    FindClose(hFind);

    return count;
}