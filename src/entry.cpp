/***********************************************************************************

    Copyright (C) 2007-2011 Ahmet Öztürk (aoz_2@yahoo.com)

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


#include "entry.hpp"
#include "diary.hpp"
#include "lifeograph.hpp"
#include "strings.hpp"


using namespace LIFEO;


// STATIC MEMBERS
ElementShower< Entry >* Entry::shower( NULL );

Entry::Entry( Diary* const d, const Date::date_t date, const Ustring& text,
              bool favored )
:   DiaryElement( d, "", favored ? ES::ENTRY_DEFAULT_FAV : ES::ENTRY_DEFAULT ), m_date( date ),
    m_date_created( time( NULL ) ), m_date_changed( m_date_created ),
    m_text( text ), m_ptr2theme_tag( NULL ),
    m_option_lang( LANG_INHERIT_DIARY )
{
    calculate_title( text );
}

Entry::Entry( Diary* const d, const Date::date_t date, bool favored )
:   DiaryElement( d, _( STRING::EMPTY_ENTRY_TITLE ),
                  favored ?  ES::ENTRY_DEFAULT_FAV : ES::ENTRY_DEFAULT ),
    m_date( date ),
    m_date_created( time( NULL ) ), m_date_changed( m_date_created ),
    m_text( "" ), m_ptr2theme_tag( NULL ),
    m_option_lang( LANG_INHERIT_DIARY )
{
}

const Icon&
Entry::get_icon() const
{
    switch( get_todo_status() )
    {
        case ES::TODO:
            return Lifeograph::icons->todo_open_16;
        case ES::PROGRESSED:
            return Lifeograph::icons->todo_progressed_16;
        case ES::DONE:
            return Lifeograph::icons->todo_done_16;
        case ES::CANCELED:
            return Lifeograph::icons->todo_canceled_16;
        default:    // 0
            return Lifeograph::icons->entry_16;
    }
}
const Icon&
Entry::get_icon32() const
{
    switch( get_todo_status() )
    {
        case ES::TODO:
            return Lifeograph::icons->todo_open_32;
        case ES::PROGRESSED:
            return Lifeograph::icons->todo_progressed_32;
        case ES::DONE:
            return Lifeograph::icons->todo_done_32;
        case ES::CANCELED:
            return Lifeograph::icons->todo_canceled_32;
        default:    // 0
            return Lifeograph::icons->entry_32;
    }
}

Ustring
Entry::get_list_str() const
{
#ifndef LIFEO_WINDOZE
    static const Ustring tpl[] = { "%2", "%1 -  %2", "<s>%2</s>", "<s>%1 -  %2</s>" };

    int i( m_date.is_hidden() ? 0 : 1 );
    if( m_status & ES::CANCELED ) i += 2;

    Ustring tpl2 = is_trashed() ?
            Glib::ustring::compose( "<span color=\"%1\">%2</span>",
                                    Lifeograph::s_color_insensitive, tpl[ i ] ) :
            tpl[ i ];

    return Glib::ustring::compose( tpl2, m_date.format_string(),
                                   Glib::Markup::escape_text( m_name ) );
#else
    return( m_date.is_hidden() ? m_name : STR::compose( m_date.format_string(), " - ", m_name ) );
#endif
}

void
Entry::calculate_title( const Ustring& text )
{
    if( text.size() < 1 )
    {
        m_name = _( STRING::EMPTY_ENTRY_TITLE );
        return;
    }
    unsigned int l_pos = text.find( '\n', 0 );
    if( l_pos == std::string::npos )
        m_name = text;
    else
        m_name = text.substr( 0, l_pos );
}

Ustring
Entry::get_date_created_str() const
{
    return Date::format_string_dt( m_date_created );
}

Ustring
Entry::get_date_changed_str() const
{
    return Date::format_string_dt( m_date_changed );
}

std::string
Entry::get_lang_final() const
{
    return m_option_lang == LANG_INHERIT_DIARY ?
            m_ptr2diary->get_lang() : m_option_lang;
}

void
Entry::set_tags( const Tagset& tagset )
{
    m_tags = tagset;
}

const Tagset&
Entry::get_tags() const
{
    return m_tags;
}

Tagset&
Entry::get_tags()
{
    return m_tags;
}

bool
Entry::add_tag( Tag* tag )
{
    if( tag->get_type() == ET_UNTAGGED )
    {
        return clear_tags();
    }
    else if( m_tags.add( tag ) )
    {
        tag->insert( this );
        m_ptr2diary->get_untagged()->erase( this );

        if( m_ptr2theme_tag == NULL && tag->get_has_own_theme() )
            m_ptr2theme_tag = tag;

        return true;
    }
    else
        return false;
}

bool
Entry::remove_tag( Tag* tag )
{
    if( tag == NULL )
        return false;

    if( m_tags.find( tag ) == m_tags.end() )
        return false;

    m_tags.erase( tag );
    tag->erase( this );

    if( m_tags.empty() )
        m_ptr2diary->get_untagged()->insert( this );

    // if this tag was the theme tag, re-adjust the theme tag
    if( m_ptr2theme_tag == tag )
    {
        for( Tagset::iterator iter = m_tags.begin(); iter != m_tags.end(); ++iter )
        {
            if( ( *iter )->get_has_own_theme() )
            {
                m_ptr2theme_tag = *iter;
                return true;
            }
        }

        m_ptr2theme_tag = NULL;
    }

    return true;
}

bool
Entry::clear_tags()
{
    if( m_tags.empty() )
        return false;

    for( Tagset::iterator iter = m_tags.begin(); iter != m_tags.end(); ++iter )
        ( *iter )->erase( this );

    m_tags.clear();
    m_ptr2theme_tag = NULL;

    return true;
}

const Theme*
Entry::get_theme() const
{
    return( m_ptr2theme_tag ?
            m_ptr2theme_tag->get_theme() : m_ptr2diary->get_untagged()->get_theme() );
}

void
Entry::set_theme_tag( const Tag* tag )
{
    // theme tag must be in the tag set
    if( m_tags.find( const_cast< Tag* >( tag ) ) != m_tags.end() )
        m_ptr2theme_tag = tag;
}

void
Entry::update_theme()
{
    if( m_ptr2theme_tag ) // if there already was a theme tag set
    {
        if( m_ptr2theme_tag->get_has_own_theme() == false ) // if it is no longer a theme tag
            m_ptr2theme_tag = NULL;
    }

    if( m_ptr2theme_tag == NULL )
    {
        // check if a tag has its own theme now and set it
        for( Tagset::iterator iter = m_tags.begin(); iter != m_tags.end(); ++iter )
        {
            if( ( *iter )->get_has_own_theme() )
            {
                m_ptr2theme_tag = *iter;
                break;
            }
        }
    }
}

bool
Entry::get_filtered_out()
{
    const Filter* filter = m_ptr2diary->get_filter();
    ElemStatus fs = filter->get_status();

    bool flag_filteredout( m_status & ES::FILTERED_OUT );

    while( fs & ES::FILTER_OUTSTANDING )  // this loop is meant for a single iteration
                                          // loop used instead of if to be able to break out
    {
        flag_filteredout = !( fs & ES::FILTER_TRASHED & m_status );

        // no need to continue if already filtered out
        if( flag_filteredout )
            break;

        flag_filteredout = !( fs & ES::FILTER_FAVORED & m_status );
        if( flag_filteredout )
            break;

        flag_filteredout = !( fs & ES::FILTER_TODO & m_status );
        if( flag_filteredout )
            break;

        if( fs & ES::FILTER_DATE_BEGIN )
            if( m_date.m_date < filter->get_date_begin() )
            {
                flag_filteredout = true;
                break;
            }

        if( fs & ES::FILTER_DATE_END )
            if( m_date.m_date > filter->get_date_end() )
            {
                flag_filteredout = true;
                break;
            }

        if( fs & ES::FILTER_TAG )
        {
            if( filter->get_tag()->get_type() == DiaryElement::ET_TAG )
            {
                if( ! m_tags.checkfor_member( filter->get_tag() ) )
                {
                    flag_filteredout = true;
                    break;
                }
            }
            else // untagged
            {
                if( ! m_tags.empty() )
                {
                    flag_filteredout = true;
                    break;
                }
            }
        }

        if( fs & ES::FILTER_INDIVIDUAL )
        {
            if( filter->is_entry_filtered( this ) )
            {
                flag_filteredout = true;
                break;
            }
        }

        if( m_ptr2diary->is_search_active() )
#ifndef LIFEO_WINDOZE
            flag_filteredout = ( m_text.lowercase().find( m_ptr2diary->get_search_text() )
                    == std::string::npos );
#else
            flag_filteredout = ( m_text.find( m_ptr2diary->get_search_text() )
                    == std::string::npos );
#endif

        break;
    }

    if( fs & ES::FILTER_OUTSTANDING )
        set_filtered_out( flag_filteredout );

    return flag_filteredout;
}

void
Entry::show()
{
    if( shower != NULL )
        shower->show( *this );
    else
        PRINT_DEBUG( "Entry has no graphical data!" );
}

void
Entry::prepare_for_hiding()
{
    if( shower != NULL )
        shower->prepare_for_hiding( *this );
    else
        PRINT_DEBUG( "Entry has no graphical data!" );
}

// ENTRY SET =======================================================================================
PoolEntries::~PoolEntries()
{
    for( iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;
}

void
PoolEntries::clear()
{
    for( iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;

    std::map< Date::date_t, Entry*, FuncCompareDates >::clear();
}
