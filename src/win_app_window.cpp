/***********************************************************************************

    Copyright (C) 2014-2015 Ahmet �zt�rk (aoz_2@yahoo.com)

    This file is part of Lifeograph.

    Lifeograph is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lifeograph is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lifeograph.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
//#define _WIN32_IE 0x0400
#include <commctrl.h>
#include <string>
#include <cstdlib>
#include <cassert>

#include "../rc/resource.h"
#include "strings.hpp"
#include "lifeograph.hpp"
#include "win_app_window.hpp"

// TEMPORARY
#define IDRT_MAIN       123338
#define IDLB_MAIN       123339
#define IDCAL_MAIN      123340


using namespace LIFEO;

inline static LRESULT CALLBACK app_window_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return WinAppWindow::p->proc( hwnd, msg, wParam, lParam );
}

WinAppWindow* WinAppWindow::p = NULL;

// CONSTRUCTOR
WinAppWindow::WinAppWindow()
:   m_seconds_remaining( LOGOUT_COUNTDOWN + 1 ), m_auto_logout_status( 1 )
{
    p = this;

    Cipher::init();

    Diary::d = new Diary;
    m_login = new WinLogin;
}

WinAppWindow::~WinAppWindow()
{
    if( Diary::d )
        delete Diary::d;
}

int
WinAppWindow::run( HINSTANCE hInstance )
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof( WNDCLASSEX );
    wc.style         = 0;
    wc.lpfnWndProc   = app_window_proc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    wc.lpszMenuName  = MAKEINTRESOURCE( IDM_MAIN );
    wc.lpszClassName = "WindowClass";
    wc.hIconSm       = LoadIcon( hInstance, "A" ); /* A is name used by project icons */

    if( !RegisterClassEx( &wc ) )
    {
        MessageBox( 0, "Window Registration Failed!", "Error!",
                    MB_ICONEXCLAMATION|MB_OK|MB_SYSTEMMODAL );
        return 0;
    }

    hwnd = CreateWindowEx( WS_EX_CLIENTEDGE,
                           "WindowClass",
                           "Lifeograph",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           640, 480,
                           NULL, NULL, hInstance, NULL );

    if( hwnd == NULL )
    {
        MessageBox( 0, "Window Creation Failed!", "Error!",
                    MB_ICONEXCLAMATION|MB_OK|MB_SYSTEMMODAL );
        return 0;
    }

    ShowWindow( hwnd, 1 );
    UpdateWindow( hwnd );

    while( GetMessage( &Msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &Msg );
        DispatchMessage( &Msg );
    }
    return Msg.wParam;
}

LRESULT
WinAppWindow::proc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
    switch( Message )
    {
        case WM_CREATE:
            m_hwnd = hwnd;
            handle_create();
            break;
        case WM_SIZE:
            if( wParam != SIZE_MINIMIZED )
                handle_resize( LOWORD( lParam ), HIWORD( lParam ) );
            break;
        case WM_SETFOCUS:
            SetFocus( GetDlgItem( hwnd, IDRT_MAIN ) );
            break;
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDMI_OPEN_DIARY:
                    m_login->add_existing_diary();
                    break;
                case IDMI_EXPORT:
                    // TODO
                    break;
                case IDMI_QUIT_WO_SAVE:
                    PostMessage( hwnd, WM_CLOSE, 0, true );
                    break;
                case IDMI_QUIT:
                    PostMessage( hwnd, WM_CLOSE, 0, 0 );
                    break;
                case IDMI_ABOUT:
                    MessageBox( NULL, "Lifeograph 0.0.0", "About...", 0 );
                    break;
            }
            break;
        case WM_CLOSE:
            if( Lifeograph::p->loginstatus == Lifeograph::LOGGED_IN )
                if( ! finish_editing( ! lParam ) )
                    break;
            DestroyWindow( hwnd );
            break;
        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
        default:
            return DefWindowProc( hwnd, Message, wParam, lParam );
    }
    return 0;
}

void
WinAppWindow::handle_create()
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof( INITCOMMONCONTROLSEX );
    iccx.dwICC  = ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES;
    InitCommonControlsEx( &iccx );
    
    m_richedit =
            CreateWindow( "EDIT", "",
                          WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|ES_MULTILINE|ES_WANTRETURN,
                          CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                          m_hwnd, ( HMENU ) IDRT_MAIN, GetModuleHandle( NULL ), NULL );
    SendMessage( m_richedit, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );

    m_list =
            CreateWindowEx( 0, WC_LISTVIEW, "",
                            WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDLB_MAIN, GetModuleHandle( NULL ), NULL );
    SendMessage( m_list, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );

    m_calendar =
            CreateWindowEx( 0, "SysMonthCal32", "",
                            WS_CHILD|WS_VISIBLE|WS_BORDER|MCS_DAYSTATE|MCS_NOTODAY,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDCAL_MAIN, Lifeograph::p->hInst, NULL );
    SendMessage( m_calendar, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
                 
    ListView_SetExtendedListViewStyle( m_list, LVS_EX_FULLROWSELECT );

    SYSTEMTIME lt;
    GetLocalTime( &lt );
    MonthCal_SetToday( m_calendar, &lt );
    
    m_login->handle_start();
}

const static float EDITOR_RATIO = 0.6;

void
WinAppWindow::handle_resize( short width, short height )
{
    RECT rc;
    MonthCal_GetMinReqRect( m_calendar, &rc );
    
    const int EDITOR_WIDTH = width * EDITOR_RATIO;
    
    MoveWindow( GetDlgItem( m_hwnd, IDRT_MAIN ), 0, 0, EDITOR_WIDTH, height, TRUE );
    MoveWindow( m_list,
                EDITOR_WIDTH, 0, width - EDITOR_WIDTH, height - rc.bottom,
                TRUE );
    MoveWindow( m_calendar,
                EDITOR_WIDTH, height - rc.bottom, width - EDITOR_WIDTH, rc.bottom,
                TRUE );
}

// LOG OUT
bool
WinAppWindow::finish_editing( bool opt_save )
{
    // files added to recent list here if not already there
    if( ! Diary::d->get_path().empty() )
        if( Lifeograph::stock_diaries.find( Diary::d->get_path() ) ==
                Lifeograph::stock_diaries.end() )
            Lifeograph::settings.recentfiles.insert( Diary::d->get_path() );

    if( ! Diary::d->is_read_only() )
    {
        sync_entry();
        // TODO Diary::d->set_last_elem( panel_main->get_cur_elem() );

        if( opt_save )
        {
            if( Diary::d->write() != SUCCESS )
            {
                MessageBox( m_hwnd,
                            STRING::CANNOT_WRITE_SUB,
                            STRING::CANNOT_WRITE,
                            MB_OK|MB_ICONERROR );
                return false;
            }
        }
        else
        {
            if( MessageBox( m_hwnd,
                            "Your changes will be backed up .~unsaved~.."
                            "If you exit normally, your diary is saved automatically.",
                            "Are you sure you want to log out without saving?",
                            MB_YESNO|MB_ICONQUESTION ) != IDYES )
                return false;
            // back up changes
            Diary::d->write( Diary::d->get_path() + ".~unsaved~" );
        }
    }

    // CLEARING
    // TODO: m_loginstatus = LOGGING_OUT_IN_PROGRESS;

    Lifeograph::m_internaloperation++;
//    panel_main->handle_logout();
//    panel_diary->handle_logout();
//    m_entry_view->get_buffer()->handle_logout();
//    panel_extra->handle_logout();

    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
        Lifeograph::loginstatus = Lifeograph::LOGGED_OUT;
    Diary::d->clear();

    Lifeograph::m_internaloperation--;
    
    return true;
}

void
WinAppWindow::logout( bool opt_save )
{
    Lifeograph::p->m_flag_open_directly = false;   // should be reset to prevent logging in again
    if( finish_editing( opt_save ) )
        m_login->handle_logout();

    m_auto_logout_status = 1;
}

void
WinAppWindow::update_title()
{
    Ustring title( PROGRAM_NAME );

    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
    {
        title += " - ";
        title += Diary::d->get_name();

        if( Diary::d->is_read_only() )
            title += " <Read Only>";
    }

    SetWindowText( m_hwnd, title.c_str() );
}

void
WinAppWindow::login()
{
    // LOGIN
    // the following must come before handle_login()s
    m_auto_logout_status = ( Lifeograph::settings.autologout && Diary::d->is_encrypted() ) ? 0 : 1;

    Lifeograph::m_internaloperation++;

//    panel_main->handle_login(); // must come before m_diary_view->handle_login() for header bar order
//    m_view_login->handle_login();
//    m_tag_view->handle_login();
//    m_filter_view->handle_login();
//    m_entry_view->handle_login();
//    m_diary_view->handle_login();
//    m_chapter_view->handle_login();
//    panel_diary->handle_login();
//    panel_extra->handle_login();

    populate();

    Lifeograph::m_internaloperation--;

    Lifeograph::loginstatus = Lifeograph::LOGGED_IN;

    update_title();
}

BOOL
InitListView( HWND hWndListView )
{
    // COLUMNS
    LV_COLUMN lvc;

    lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    lvc.pszText  = ( char* ) "Entries";
    lvc.cx       = 100;          // width of column in pixels

    ListView_InsertColumn( hWndListView, 0, &lvc );
    
    // IMAGE LISTS
    HICON hIconItem;     // Icon for list-view items.
    HIMAGELIST hLarge;   // Image list for icon view.
    HIMAGELIST hSmall;   // Image list for other views.

    hSmall = ImageList_Create( GetSystemMetrics( SM_CXSMICON ),
                               GetSystemMetrics( SM_CYSMICON ),
                               ILC_MASK, 1, 1 );

    for( int index = 0; index < 2; index++ )
    {
        hIconItem = LoadIcon( Lifeograph::p->hInst, MAKEINTRESOURCE( IDI_ENTRY16 + index ) );
        ImageList_AddIcon( hSmall, hIconItem );
        DestroyIcon( hIconItem );
    }

    // Assign the image lists to the list-view control.
    ListView_SetImageList( hWndListView, hSmall, LVSIL_SMALL );

    return TRUE;
}

void
WinAppWindow::populate()
{
    InitListView( m_list );
    
    int i = 0;
    LVITEM lvi;
    lvi.mask      = LVIF_TEXT | LVIF_IMAGE |LVIF_STATE;
    lvi.stateMask = 0;
    lvi.iSubItem  = 0;
    lvi.state     = 0;
    lvi.iImage    = 0;

    for( auto& kv : Diary::d->get_entries() )
    {
        lvi.iItem     = i++;
        lvi.pszText   = ( char* ) kv.second->get_list_str().c_str();

        ListView_InsertItem( m_list, &lvi );
    }
}

void
WinAppWindow::sync_entry()
{
    // TODO
}
