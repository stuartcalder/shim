#include "term.h"

#if    defined (SHIM_OS_UNIXLIKE)
#	define NEWLINE_	"\n"
#	ifdef __NetBSD__
#		include <ncurses/ncurses.h>
#	else
#		include <ncurses.h>
#	endif
#elif  defined (SHIM_OS_WINDOWS)
#	define NEWLINE_	"\n\r"
#	include <windows.h>
#else
#	error "Unsupported OS."
#endif

#define OS_PROMPT_ "> " NEWLINE_

#if (SHIM_TERM_BUFFER_SIZE < 2)
#	error "Minimum buffer size of 2 bytes."
#endif

#ifdef SHIM_OS_UNIXLIKE
/* On Windows these functions are inlined. */
void SHIM_PUBLIC
shim_term_init () {
	initscr();
	clear();
}
void SHIM_PUBLIC
shim_term_end () {
	endwin();
}
#endif /* ~ SHIM_OS_UNIXLIKE */

int SHIM_PUBLIC
shim_term_get_secret_string (uint8_t *    SHIM_RESTRICT buffer,
			     char const * SHIM_RESTRICT prompt)
#if    defined (SHIM_OS_UNIXLIKE)
{ /* Unixlike impl */
	cbreak();
	noecho();
	keypad( stdscr, TRUE );
	int index = 0;
	#if 0
	WINDOW * w = newwin( 5, MAX_PASSWORD_SIZE_ + 10, 0, 0 );
	#endif
	WINDOW * w = newwin( 5, SHIM_TERM_MAX_PW_SIZE + 10, 0, 0 );
	keypad( w, TRUE );
	bool outer, inner;
	outer = true;
	while( outer ) {
		memset( buffer, 0, SHIM_TERM_BUFFER_SIZE );
		wclear( w );
		wmove( w, 1, 0 );
		waddstr( w, prompt );
		inner = true;
		while( inner ) {
			int ch = wgetch( w );
			switch( ch ) {
				/* Delete */
				case 127:
				case KEY_DC:
				case KEY_LEFT:
				case KEY_BACKSPACE: {
					if( index > 0 ) {
						int y, x;
						getyx( w, y, x );
						wdelch( w );
						wmove( w, y, x - 1 );
						wrefresh( w );
						buffer[ --index ] = UINT8_C (0);
					}
				} break;
				/* Return */
				case '\n':
				case KEY_ENTER: {
					inner = false;
				} break;
				default: {
					#if 0
					if( index < MAX_PASSWORD_SIZE_ ) {
					#endif
					if( index < SHIM_TERM_MAX_PW_SIZE ) {
						waddch( w, '*' );
						wrefresh( w );
						buffer[ index++ ] = (uint8_t)ch;
					}
				} break;
			} /* ~ switch(ch) */
		} /* ~ while(inner) */
		outer = false;
	} /* ~ while(outer) */
	int const pw_size = strlen( (char *)buffer );
	delwin( w );
	return pw_size;
} /* ~ Unixlike impl */
#elif  defined (SHIM_OS_WINDOWS)
{ /* Windows impl */
	int index = 0;
	bool repeat_ui, repeat_input;
	repeat_ui = true;
	while( repeat_ui ) {
		memset( buffer, 0, SHIM_TERM_BUFFER_SIZE );
		system( "cls" );
		if( _cputs( prompt ) != 0 )
			SHIM_ERRX ("Failed to _cputs()!\n");
		repeat_input = true;
		while( repeat_input ) {
			int ch = _getch();
			switch( ch ) {
				case '\b': {
					if( index > 0 ) {
						if( _cputs( "\b \b" ) != 0 )
							SHIM_ERRX ("Failed to _cputs()!\n");
						buffer[ --index ] = UINT8_C (0);
					}
				} break;
				case '\r': {
					repeat_input = false;
				} break;
				default: {
					if( (index < SHIM_TERM_BUFFER_SIZE) && (ch >= 32) && (ch <= 126) ) {
						if( _putch( "*" ) == EOF )
							SHIM_ERRX ("Failed to _putch()!\n");
						buffer[ index++ ] = (uint8_t)ch;
					}
				} break;
			} /* ~ switch(ch) */
		} /* ~ while(repeat_input) */
		repeat_ui = false;
	} /* ~ while(repeat_ui) */
	int const pw_size = strlen( (char *)buffer );
	system( "cls" );
	return pw_size;
} /* ~ Windows impl */
#else
#	error "Unsupported OS."
#endif

int SHIM_PUBLIC
shim_term_obtain_password (uint8_t *    SHIM_RESTRICT password_buf,
			   char const * SHIM_RESTRICT entry_prompt,
			   int const                  min_pw_size,
			   int const                  max_pw_size)
{
	int size;
	while( 1 ) {
		size = shim_term_get_secret_string( password_buf, entry_prompt );
		if( size < min_pw_size ) {
			shim_term_notify( "Password is not long enough." NEWLINE_ );
		} else if( size > max_pw_size ) {
			shim_term_notify( "Password is too long." NEWLINE_ );
		} else {
			break;
		}
	}
	return size;
}

int SHIM_PUBLIC
shim_term_obtain_password_checked (uint8_t *    SHIM_RESTRICT password_buf,
				   uint8_t *    SHIM_RESTRICT check_buf,
				   char const * SHIM_RESTRICT entry_prompt,
				   char const * SHIM_RESTRICT reentry_prompt,
				   int const                  min_pw_size,
				   int const                  max_pw_size)
{
	int size;
	while( 1 ) {
		size = shim_term_get_secret_string( password_buf, entry_prompt );
		if( size < min_pw_size ) {
			shim_term_notify( "Password is not long enough." NEWLINE_ );
			continue;
		} else if( size > max_pw_size ) {
			shim_term_notify( "Password is too long." NEWLINE_ );
			continue;
		} else if( shim_term_get_secret_string( check_buf, reentry_prompt ) != size ) {
			shim_term_notify( "Second password not the same size as the first." NEWLINE_ );
			continue;
		}
		if( shim_ctime_memcmp( password_buf, check_buf, SHIM_TERM_BUFFER_SIZE ) == 0 )
			break;
		shim_term_notify( "Passwords do not match." NEWLINE_ );
	}
	return size;
}

void SHIM_PUBLIC
shim_term_notify (char const * notice) {
#if    defined (SHIM_OS_UNIXLIKE)
	WINDOW * w = newwin( 1, strlen( notice ) + 1, 0, 0 );
	wclear( w );
	wmove( w, 0, 0 );
	waddstr( w, notice );
	wrefresh( w );
	wgetch( w );
	delwin( w );
#elif  defined (SHIM_OS_WINDOWS)
	system( "cls" );
	if( _cputs( notice ) != 0 )
		SHIM_ERRX ("Error: Failed to _cputs()\n");
	system( "pause" );
	system( "cls" );
#else
#	error "Unsupported OS."
#endif
}





