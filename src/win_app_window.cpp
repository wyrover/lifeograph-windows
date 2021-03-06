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
#include <richedit.h>
#include <string>
#include <cstdlib>
#include <cassert>

#ifndef RICHEDIT_CLASS
    #ifdef UNICODE
        #define RICHEDIT_CLASS "RichEdit20W"
    #else
        #define RICHEDIT_CLASS "RichEdit20A"
    #endif
#endif

#include "../rc/resource.h"
#include "strings.hpp"
#include "lifeograph.hpp"
#include "win_app_window.hpp"
#include "win_dialog_password.hpp"
#include "win_dialog_tags.hpp"


using namespace LIFEO;

// NON-MEMBER PROCEDURES
inline static LRESULT CALLBACK
app_window_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return WinAppWindow::p->proc( hwnd, msg, wParam, lParam );
}

int CALLBACK
list_compare_func( LPARAM lp1, LPARAM lp2, LPARAM lParamSort )
{
    // SORT BY DATE (ONLY DESCENDINGLY FOR NOW)
    DiaryElement* item1 = Diary::d->get_element( lp1 );
    DiaryElement* item2 = Diary::d->get_element( lp2 );
    if( !( item1 && item2 ) )
        return 0;
    else
    if( item1->get_type() == DiaryElement::ET_DIARY )
    return -1;

    int direction( ( item1->get_date().is_ordinal() && item2->get_date().is_ordinal() ) ? -1 : 1 );

    if( item1->get_date() > item2->get_date() )
        return -direction;
    else
    if( item1->get_date() < item2->get_date() )
        return direction;
    else
        return 0;
}

LRESULT CALLBACK
calendar_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
               UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    switch( msg )
    {
        case WM_LBUTTONDBLCLK:
            WinAppWindow::p->handle_calendar_doubleclick();
            return TRUE;
        // TODO: add a menu for chapters..
        //case WM_RBUTTONUP:
            //return TRUE;
    }
    
    return DefSubclassProc( hwnd, msg, wparam, lparam );
}

WinAppWindow* WinAppWindow::p = NULL;

// CONSTRUCTOR
WinAppWindow::WinAppWindow()
:    m_entry_view( NULL ), m_seconds_remaining( LOGOUT_COUNTDOWN + 1 ), m_auto_logout_status( 1 )
{
    p = this;

    Cipher::init();

    Diary::d = new Diary;
    m_login = new WinLogin;
    m_entry_view = new EntryView;
    m_diary_view = new DiaryView;
}

WinAppWindow::~WinAppWindow()
{
    if( Diary::d )
        delete Diary::d;
}

int
WinAppWindow::run()
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof( WNDCLASSEX );
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = app_window_proc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = Lifeograph::hInst;
    wc.hIcon         = LoadIcon( Lifeograph::hInst, L"A" );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    wc.lpszMenuName  = MAKEINTRESOURCE( IDM_MAIN );
    wc.lpszClassName = L"WindowClass";
    wc.hIconSm       = LoadIcon( Lifeograph::hInst, L"A" ); // A is name used by project icons
    
    if( !RegisterClassEx( &wc ) )
    {
        MessageBoxA( 0, "Window Registration Failed!", "Error!",
                     MB_ICONEXCLAMATION|MB_OK|MB_SYSTEMMODAL );
        return 0;
    }

    hwnd = CreateWindowEx( WS_EX_CLIENTEDGE,
                           L"WindowClass",
                           L"Lifeograph",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           640, 480,
                           NULL, NULL, Lifeograph::hInst, NULL );

    if( hwnd == NULL )
    {
        MessageBoxA( 0, "Window Creation Failed!", "Error!",
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
        case WM_ENTERMENULOOP:
            if( wParam == false )
                update_menu();
            break;
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDMI_DIARY_CREATE:
                    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
                        if( ! finish_editing() )
                            break;
                    m_login->create_new_diary();
                    break;
                case IDMI_DIARY_OPEN:
                    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
                        if( ! finish_editing() )
                            break;
                    m_login->add_existing_diary();
                    break;
                case IDMI_DIARY_CHANGE_PASSWORD:
                    if( !authorize() )
                        break;
                case IDMI_DIARY_ENCRYPT:
                    DialogPassword::launch( m_hwnd, Diary::d, DialogPassword::PD_NEW );
                    break;
                case IDMI_EXPORT:
                    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
                        m_diary_view->export_diary();
                    break;
                case IDMI_QUIT_WO_SAVE:
                    PostMessage( hwnd, WM_CLOSE, 0, true );
                    break;
                case IDMI_QUIT:
                    PostMessage( hwnd, WM_CLOSE, 0, 0 );
                    break;
                case IDMI_ENTRY_DISMISS:
                    m_entry_view->dismiss_entry();
                    break;
                case IDMI_ABOUT:
                    MessageBoxA( NULL, "Lifeograph for Windows  0.1.1\n\n"
                                       "Copyright (C) 2014 Ahmet �zt�rk\n"
                                       "Lifeograph is licensed under GNU Public License v3\n\n"
                                       "http://lifeograph.sourceforge.net",
                                 "About...", MB_OK );
                    break;
                case IDRT_MAIN:
                    if( HIWORD( wParam ) == EN_CHANGE )
                        m_entry_view->m_richedit->handle_change();
            }
            break;
        case WM_NOTIFY:
            handle_notify( ( int ) wParam, lParam );
            break;
        case WM_MOUSEMOVE:
            if( !wParam )
                m_entry_view->m_tag_widget->handle_mouse_move( LOWORD( lParam ), HIWORD( lParam ) );
            break;
        case WM_LBUTTONDBLCLK:
            if( wParam == MK_LBUTTON )
                m_entry_view->m_tag_widget->handle_click( LOWORD( lParam ), HIWORD( lParam ) );
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
        case WM_PAINT:
            if( GetUpdateRect( m_hwnd, NULL, false ) )
                m_entry_view->m_tag_widget->handle_draw();
            // BEWARE: no break, WM_PAINT must be processed by the system or bad things happen
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

    // try to load latest version of rich edit control
    static HINSTANCE hlib = LoadLibrary( L"RICHED20.DLL" );
    if( !hlib )
    {
        MessageBoxA( NULL, "Failed to load Rich Edit", "Error", MB_OK | MB_ICONERROR );
        return;
    }

    // RICH EDIT
    m_entry_view->m_richedit->m_hwnd =
            CreateWindowEx( 0, //WS_EX_CLIENTEDGE,
                            RICHEDIT_CLASS, L"",
                    	    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN,
                            0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDRT_MAIN, GetModuleHandle( NULL ), NULL );

    SendMessage( m_entry_view->m_richedit->m_hwnd, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
    SendMessage( m_entry_view->m_richedit->m_hwnd, EM_SETEVENTMASK, 0, ( LPARAM ) ENM_CHANGE );

    // LIST VIEW
    m_list =
            CreateWindowExW( 0, WC_LISTVIEWW, L"",
                             WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT |
                             LVS_NOCOLUMNHEADER | LVS_SINGLESEL,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             m_hwnd, ( HMENU ) IDLV_MAIN, GetModuleHandle( NULL ), NULL );
    init_list();

    // CALENDAR
    m_calendar =
            CreateWindowEx( 0, L"SysMonthCal32", L"",
                            WS_CHILD|WS_VISIBLE | MCS_DAYSTATE|MCS_NOTODAY,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDCAL_MAIN, Lifeograph::hInst, NULL );
    SendMessage( m_calendar, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
                 
    SetWindowSubclass( m_calendar, calendar_proc, 0, 0 );
    SetClassLongPtr( m_calendar, GCL_STYLE, CS_DBLCLKS ); // make it accept double clicks

    SYSTEMTIME lt;
    GetLocalTime( &lt );
    MonthCal_SetToday( m_calendar, &lt );
    
    m_login->handle_start();
}

void
WinAppWindow::handle_resize( short width, short height )
{
    RECT rc;
    MonthCal_GetMinReqRect( m_calendar, &rc );
    
    const int editor_width = width * EDITOR_RATIO;

    m_entry_view->m_tag_widget->handle_resize( editor_width, height );

    const int editor_height = height - m_entry_view->m_tag_widget->get_height();
    
    MoveWindow( m_entry_view->m_richedit->m_hwnd, 0, 0, editor_width, editor_height, TRUE );

    MoveWindow( m_list,
                editor_width, 0, width - editor_width, height - rc.bottom,
                TRUE );

    MoveWindow( m_calendar,
                editor_width, height - rc.bottom, width - editor_width, rc.bottom,
                TRUE );

    ListView_SetColumnWidth( m_list, 0, width - editor_width - GetSystemMetrics( SM_CXVSCROLL ) );
    
    rc.left = 0;
    rc.top = editor_height;
    rc.right = editor_width;
    rc.bottom = height;
    InvalidateRect( m_hwnd, &rc, true );
}

void
WinAppWindow::handle_notify( int id, LPARAM lparam )
{
    switch( id )
    {
        case IDLV_MAIN:
            if( ( ( LPNMHDR ) lparam )->code == NM_CLICK )
            {
                int iSelect = SendMessage( m_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED ); // return item selected

                if( iSelect != -1 )
                {
                    LVITEM lvi;
                    lvi.mask        = LVIF_PARAM;
                    lvi.iItem       = iSelect;
                    if( ListView_GetItem( m_list, &lvi ) )
                    {
                        DiaryElement* elem = Diary::d->get_element( lvi.lParam );
                        if( elem )
                            elem->show();
                    }
                }
            }
            PRINT_DEBUG( "....." );
            break;
        case IDCAL_MAIN:
            if( ( ( LPNMHDR ) lparam )->code == MCN_GETDAYSTATE )
            {
                NMDAYSTATE* ds( ( NMDAYSTATE* ) lparam );
                MONTHDAYSTATE mds[ 12 ] = { 0 };

                fill_monthdaystate( ds->stStart.wYear,
                                    ds->stStart.wMonth,
                                    mds,
                                    ds->cDayState );

                ds->prgDayState = mds;
            }
            else if( ( ( LPNMHDR ) lparam )->code == MCN_SELECT )
            {
                if( Lifeograph::loginstatus != Lifeograph::LOGGED_IN )
                    return;
                //if( Lifeograph::m_internaloperation ) return;

                NMSELCHANGE* sc = ( LPNMSELCHANGE ) lparam;
                Date date( sc->stSelStart.wYear, sc->stSelStart.wMonth, sc->stSelStart.wDay );
                Entry* entry;

                if( m_entry_view->get_element() &&
                    date.m_date == ( m_entry_view->get_element()->get_date().get_pure() ) )
                {
                    entry = Diary::d->get_entry_next_in_day(
                            m_entry_view->get_element()->get_date() );
                }
                else
                {
                    entry = Diary::d->get_entry( date.m_date + 1 ); // 1 increment to fix order
                }

                if( entry )
                    entry->show();
            }
            break;
    }
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
        m_entry_view->sync();

        if( m_entry_view->get_element() )
            Diary::d->set_last_elem( m_entry_view->get_element() );

        if( opt_save )
        {
            if( Diary::d->write() != SUCCESS )
            {
                MessageBoxA( m_hwnd,
                             STRING::CANNOT_WRITE_SUB,
                             STRING::CANNOT_WRITE,
                             MB_OK|MB_ICONERROR );
                return false;
            }
        }
        else
        {
            if( MessageBoxA( m_hwnd,
                             "Your changes will be backed up in .~unsaved~."
                             "If you exit normally, your diary is saved automatically.",
                             "Are you sure you want to log out without saving?",
                             MB_YESNO | MB_ICONQUESTION ) != IDYES )
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
    m_entry_view->clear();
    update_entry_list();
    update_calendar();
    update_title();

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

    update_entry_list();
    update_calendar();

    Lifeograph::m_internaloperation--;

    Lifeograph::loginstatus = Lifeograph::LOGGED_IN;

    update_title();

    DiaryElement* startup_elem = Diary::d->get_startup_elem();
    
    if( startup_elem->get_type() == DiaryElement::ET_ENTRY )
        startup_elem->show();
}

BOOL
WinAppWindow::init_list()
{
    SendMessage( m_list, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
    ListView_SetExtendedListViewStyle( m_list, LVS_EX_FULLROWSELECT );
    ListView_SetUnicodeFormat( m_list, true );

    // COLUMNS
    LV_COLUMN lvc;

    lvc.mask     = LVCF_FMT | LVCF_TEXT;
    lvc.fmt      = LVCFMT_LEFT | LVCFMT_IMAGE;
    lvc.pszText  = ( wchar_t* ) L"Entries";

    ListView_InsertColumn( m_list, 0, &lvc );
    
    // IMAGE LISTS
    /*HIMAGELIST himagelist = ImageList_Create( 16, 16, ILC_MASK, 1, 1 );

    for( int i = 0; i < 5; i++ )
    {
        HICON hicon = LoadIcon( Lifeograph::p->hInst, MAKEINTRESOURCE( IDI_ENTRY16 + i ) );
        ImageList_AddIcon( himagelist, hicon );
        //DestroyIcon( hicon );
    }*/
    
    HIMAGELIST himagelist = ImageList_Create( 16, 16, ILC_COLOR24, 0, 5 );
    HBITMAP hbitmap = ( HBITMAP ) LoadImage( Lifeograph::p->hInst,
                                             MAKEINTRESOURCE( IDBM_ENTRY16 ),
                                             IMAGE_BITMAP, 0, 0,
                                             LR_LOADTRANSPARENT );

    ImageList_Add( himagelist, hbitmap, NULL );

    DeleteObject( hbitmap );

    ListView_SetImageList( m_list, himagelist, LVSIL_SMALL );

    return TRUE;
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

    SetWindowTextA( m_hwnd, title.c_str() );
}

void
WinAppWindow::update_menu()
{
    HMENU hmenu = GetMenu( m_hwnd );
    bool logged_in = Lifeograph::loginstatus == Lifeograph::LOGGED_IN;
    bool encrypted = Diary::d->is_encrypted();
    
    EnableMenuItem( hmenu, IDMI_DIARY_ENCRYPT,
                    MF_BYCOMMAND | ( logged_in && !encrypted ? MF_ENABLED : MF_GRAYED ) );
                    
    EnableMenuItem( hmenu, IDMI_DIARY_CHANGE_PASSWORD,
                    MF_BYCOMMAND | ( logged_in && encrypted ? MF_ENABLED : MF_GRAYED ) );

    EnableMenuItem( hmenu, IDMI_EXPORT,
                    MF_BYCOMMAND | ( logged_in ? MF_ENABLED : MF_GRAYED ) );
    EnableMenuItem( hmenu, IDMI_QUIT_WO_SAVE,
                    MF_BYCOMMAND | ( logged_in ? MF_ENABLED : MF_GRAYED ) );
                            
    EnableMenuItem( hmenu, IDMI_ENTRY_DISMISS,
                    MF_BYCOMMAND | ( m_entry_view->get_element() ? MF_ENABLED : MF_GRAYED ) );
                            
    //CheckMenuItem( hmenu, IDMI_, MF_BYCOMMAND | ( ? MF_CHECKED : MF_UNCHECKED ) );
}

bool
WinAppWindow::select_list_elem( const DiaryElement* elem )
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = ( LPARAM ) elem->get_id();
    
    int index = SendMessage( m_list, LVM_FINDITEM, -1, ( LPARAM ) &lvfi );
    
    if( index > -1 )
    {
        LVITEM lvi;
        lvi.stateMask = LVIS_SELECTED;
        lvi.state = LVIS_SELECTED;

        SendMessage( m_list, LVM_SETITEMSTATE, index, ( LPARAM ) &lvi );
        SendMessage( m_list, LVM_ENSUREVISIBLE, index, TRUE );
        
        return true;
    }
    
    return false;
}

bool
WinAppWindow::update_list_elem( const DiaryElement* elem )
{
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = ( LPARAM ) elem->get_id();

    int index = SendMessage( m_list, LVM_FINDITEM, -1, ( LPARAM ) &lvfi );

    if( index > -1 )
    {
        LVITEM lvi;
        lvi.iItem       = index;
        lvi.iSubItem    = 0;
        lvi.mask        = LVIF_TEXT | LVIF_IMAGE;
        lvi.pszText     = HELPERS::convert_utf8_to_16( elem->get_list_str() );

        switch( elem->get_todo_status() )
        {
            case ES::TODO:
                lvi.iImage    = 1;
                break;
            case ES::PROGRESSED:
                lvi.iImage    = 2;
                break;
            case ES::DONE:
                lvi.iImage    = 3;
                break;
            case ES::CANCELED:
                lvi.iImage    = 4;
                break;
            default:    // 0
                lvi.iImage    = 0;
                break;
        }

        SendMessage( m_list, LVM_SETITEM, 0, ( LPARAM ) &lvi );
        SendMessage( m_list, LVM_ENSUREVISIBLE, index, TRUE );

        return true;
    }

    return false;
}

void
WinAppWindow::update_entry_list()
{
    SendMessage( m_list, LVM_DELETEALLITEMS, 0, 0 );
    
    int i = 0;
    LVITEM lvi;
    lvi.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    lvi.stateMask = LVIS_SELECTED;
    lvi.iSubItem  = 0;

    for( auto& kv : Diary::d->get_entries() )
    {
        lvi.iItem   = i++;
        lvi.pszText = HELPERS::convert_utf8_to_16( kv.second->get_list_str() );
        lvi.lParam  = ( LPARAM ) kv.second->get_id();
        lvi.state   = ( m_entry_view->get_element() &&
                        m_entry_view->get_element()->get_id() == kv.second->get_id() ) ?
                        LVIS_SELECTED : 0;

        switch( kv.second->get_todo_status() )
        {
            case ES::TODO:
                lvi.iImage    = 1;
                break;
            case ES::PROGRESSED:
                lvi.iImage    = 2;
                break;
            case ES::DONE:
                lvi.iImage    = 3;
                break;
            case ES::CANCELED:
                lvi.iImage    = 4;
                break;
            default:    // 0
                lvi.iImage    = 0;
                break;
        }

        SendMessage( m_list, LVM_INSERTITEM, 0, ( LPARAM ) &lvi );
    }
    
    SendMessage( m_list, LVM_SORTITEMS, 0, ( WPARAM ) list_compare_func );
}

#define BOLDDAY(ds, iDay)  if(iDay > 0 && iDay < 32) (ds) |= (0x00000001 << (iDay - 1))

void
WinAppWindow::fill_monthdaystate( int year, int month, MONTHDAYSTATE mds[], int size )
{
    for( int i = 0; i < size; i++ )
    {
        for( auto& kv_entry : Diary::d->get_entries() )
        {
            Entry* entry = kv_entry.second;

            if( /*entry->get_filtered_out() ||*/ entry->get_date().is_ordinal() )
                continue;

            if( entry->get_date().get_year() == year )
            {
                if( entry->get_date().get_month() == month )
                    BOLDDAY( mds[ i ], entry->get_date().get_day() );
                else if( entry->get_date().get_month() < month )
                    break;
            }
            else
            if( entry->get_date().get_year() < year )
                break;
        }

        // increase month and, if needed, year
        if( ++month > 12 )
        {
            month = 1;
            year++;
        }
    }
}

void
WinAppWindow::update_calendar()
{
    SYSTEMTIME st[ 2 ];
    MONTHDAYSTATE mds[ 12 ] = { 0 };
    int size = SendMessage( m_calendar, MCM_GETMONTHRANGE, GMR_DAYSTATE, ( LPARAM ) st );

    fill_monthdaystate( st[ 0 ].wYear, st[ 0 ].wMonth, mds, size );

    SendMessage( m_calendar, MCM_SETDAYSTATE, size, ( LPARAM ) &mds );

    RECT rect;
    
    MonthCal_GetMinReqRect( m_calendar, &rect );
    const int height = rect.bottom - rect.top;
    GetClientRect( m_hwnd, &rect );
    
    rect.left = rect.right * EDITOR_RATIO;
    rect.top = rect.bottom  - height;
    
    InvalidateRect( m_hwnd, &rect, TRUE );
}

void
WinAppWindow::handle_calendar_doubleclick()
{
    if( Lifeograph::loginstatus != Lifeograph::LOGGED_IN || Diary::d->is_read_only() )
        return;

    SYSTEMTIME st;
    if( ! SendMessage( m_calendar, MCM_GETCURSEL, 0, ( LPARAM ) &st ) )
        return;

    Date date( st.wYear, st.wMonth, st.wDay );
    Entry* entry( Diary::d->create_entry( date.m_date ) );

    update_entry_list();
    update_calendar();

    entry->show();
}

void
WinAppWindow::start_tag_dialog( const Wstring& name )
{
    if( DialogTags::launch( m_hwnd, Diary::d, m_entry_view->get_element(), name ) == SUCCESS )
    {
        m_entry_view->update_theme();
        m_entry_view->m_tag_widget->update_full();
    }
}

bool
WinAppWindow::authorize()
{
    int i = 0;
    Result res;
    
    while( res = DialogPassword::launch( m_hwnd, Diary::d, DialogPassword::PD_AUTHORIZE, i ) )
    {
        switch( res )
        {
            case SUCCESS:
                return true;
            case INVALID:
                i++;
                break;
            case ABORTED:
                return false;
        }
    }
}

bool
WinAppWindow::confirm_dismiss_element( const DiaryElement* elem )
{
    return(
        MessageBox( m_hwnd,
                    convert_utf8_to_16(
                            STR::compose( "Are you sure, you want to dismiss ",
                                          elem->get_name() ) ),
                    L"Confirm Dismiss",
                    MB_YESNO | MB_ICONWARNING ) == IDYES );
}

