#include <windows.h>
#include <tchar.h>
#include <iostream>
#include "Mailbox.h"

using namespace std;

void ClearInput()
{
    int ch;
    while ((ch = _gettchar()) != _T('\n') && ch != EOF) {}
}

void PrintMenu()
{
    _tprintf(_T("\n===== MAILBOX MENU =====\n"));
    _tprintf(_T("1. Create mailbox\n"));
    _tprintf(_T("2. Open mailbox\n"));
    _tprintf(_T("3. Add message\n"));
    _tprintf(_T("4. Read message\n"));
    _tprintf(_T("5. Read message and delete\n"));
    _tprintf(_T("6. Delete message\n"));
    _tprintf(_T("7. Delete all messages\n"));
    _tprintf(_T("8. Show message count\n"));
    _tprintf(_T("9. Show mailbox count\n"));
    _tprintf(_T("10. Check mailbox integrity\n"));
    _tprintf(_T("m. Show menu again\n"));
    _tprintf(_T("0. Exit\n"));
}

void MakeMailboxFileName(TCHAR* result, DWORD size)  //форм імя пошт скриньки
{
    TCHAR name[MAX_PATH];

    _tprintf(_T("Enter mailbox name: "));
    _getts_s(name, MAX_PATH);

    BOOL onlySpaces = TRUE;

    for (int i = 0; name[i] != _T('\0'); i++)
    {
        if (name[i] != _T(' ') && name[i] != _T('\t'))
        {
            onlySpaces = FALSE;
            break;
        }
    }

    if (_tcslen(name) == 0 || onlySpaces)
    {
        _tprintf(_T("Mailbox name cannot be empty or contain only spaces.\n"));
        result[0] = _T('\0');
        return;
    }

    _stprintf_s(result, size, _T("%s.mbx"), name);
}

BOOL FileExists(LPCTSTR directory, LPCTSTR fileName)  //чи існ файл
{
    TCHAR fullPath[MAX_PATH];

    _stprintf_s(fullPath, MAX_PATH, _T("%s\\%s"), directory, fileName);

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

    CloseHandle(hFile);
    return TRUE;
}

int _tmain()
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    const TCHAR directory[] = _T("Mailboxes");

    Mailbox* mailbox = NULL;
    BOOL mailboxOpened = FALSE; //чи відкр скринька
    BOOL mailboxBlocked = FALSE; //чи забл скринька

    TCHAR mailboxFile[MAX_PATH];
    TCHAR message[500];

    TCHAR choice[20];
    DWORD maxSize;
    DWORD number;

    PrintMenu();

    do
    {
        _tprintf(_T("\nYour choice: "));
        _getts_s(choice, 20);

        if (_tcscmp(choice, _T("m")) == 0 || _tcscmp(choice, _T("M")) == 0)
        {
            PrintMenu();
            continue;
        }

        int command = _ttoi(choice);

        switch (command)
        {
        case 1:                                                     //створення скриньки
            MakeMailboxFileName(mailboxFile, MAX_PATH);
            if (_tcslen(mailboxFile) == 0)
            {
                break;
            }

            _tprintf(_T("Enter maximum mailbox size in bytes: "));
            _tscanf_s(_T("%lu"), &maxSize);
            ClearInput();

            if (mailbox != NULL)
            {
                delete mailbox;
                mailbox = NULL;
            }

            mailbox = new Mailbox(directory, mailboxFile);

            if (mailbox->Create(maxSize))
            {
                mailboxOpened = TRUE;
                mailboxBlocked = FALSE;
                _tprintf(_T("Active mailbox: %s\n"), mailboxFile);
            }

            break;

        case 2:                                                     //відкриття скриньки 
            MakeMailboxFileName(mailboxFile, MAX_PATH);
            if (_tcslen(mailboxFile) == 0)
            {
                break;
            }

            if (!FileExists(directory, mailboxFile))
            {
                _tprintf(_T("Mailbox does not exist. Create it first.\n"));
                break;
            }

            if (mailbox != NULL)
            {
                delete mailbox;
                mailbox = NULL;
            }

            mailbox = new Mailbox(directory, mailboxFile);
            mailboxOpened = TRUE;
            mailboxBlocked = FALSE;

            _tprintf(_T("Mailbox opened: %s\n"), mailboxFile);

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
            }

            break;

        case 3:                                                 //додавання повідомлення
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (mailboxBlocked)
            {
                _tprintf(_T("Mailbox access is blocked.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
                break;
            }

            _tprintf(_T("Enter message: "));
            _getts_s(message, 500);

            mailbox->AddMessage(message);
            break;

        case 4:                                             //читання повідомлення без видалення
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (mailboxBlocked)
            {
                _tprintf(_T("Mailbox access is blocked.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
                break;
            }

            _tprintf(_T("Enter message number: "));
            _tscanf_s(_T("%lu"), &number);
            ClearInput();

            mailbox->ReadMessage(number, FALSE);
            break;

        case 5:                                     //читання повідомлення з видаленням
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (mailboxBlocked)
            {
                _tprintf(_T("Mailbox access is blocked.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
                break;
            }

            _tprintf(_T("Enter message number: "));
            _tscanf_s(_T("%lu"), &number);
            ClearInput();

            mailbox->ReadMessage(number, TRUE);
            break;

        case 6:                                     //видалееея конкретного повідомлення
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (mailboxBlocked)
            {
                _tprintf(_T("Mailbox access is blocked.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
                break;
            }

            _tprintf(_T("Enter message number: "));
            _tscanf_s(_T("%lu"), &number);
            ClearInput();

            mailbox->DeleteMessage(number);
            break;

        case 7:                                         //видалення всіх повідомлень
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (mailboxBlocked)
            {
                _tprintf(_T("Mailbox access is blocked.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
                break;
            }

            mailbox->DeleteAllMessages();
            break;

        case 8:                                         //показати кількість повідомлень
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (mailboxBlocked)
            {
                _tprintf(_T("Mailbox access is blocked.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
                break;
            }

            _tprintf(_T("Message count: %lu\n"), mailbox->GetMessageCount());
            break;

        case 9:                                 //кількість скриньок
            _tprintf(
                _T("Total mailboxes in directory: %lu\n"),
                Mailbox::GetMailboxCount(directory)
            );
            break;

        case 10:                                //пеервірка цілісності
            if (!mailboxOpened || mailbox == NULL)
            {
                _tprintf(_T("Open or create mailbox first.\n"));
                break;
            }

            if (!mailbox->CheckIntegrityWithChoice())
            {
                mailboxBlocked = TRUE;
            }

            break;

        case 0:
            _tprintf(_T("Exit.\n"));
            break;

        default:
            _tprintf(_T("Invalid choice. Press m to show menu.\n"));
        }

    } while (_ttoi(choice) != 0);

    if (mailbox != NULL)
        delete mailbox;

    system("pause");
    return 0;
}