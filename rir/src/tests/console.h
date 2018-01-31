#pragma once

#include <iostream>
#include <functional>

/** Enum for terminal colors. 
*/
enum class color {
	black,
	darkRed,
	darkGreen,
	darkYellow,
	darkBlue,
	darkMagenta,
	darkCyan,
	gray,
	darkGray,
	red,
	green,
	yellow,
	blue,
	magenta,
	cyan,
	white,
};

/** Provides support for basic output to ANSI terminals, including cursor movements, colors, etc.
 */
namespace console {

    /** Resets the terminal.
     */
    std::ostream & reset(std::ostream & s);

    /** Shows the cursor.
     */
    std::ostream & show(std::ostream & s);

    /** Erase display.
     */
    std::ostream & erase(std::ostream & s);

    /** Hides the cursor.
     */
    std::ostream & hide(std::ostream & s);

    /** Moves the cursor up by given number of rows.
     */
    std::function<std::ostream &(std::ostream &)> up(unsigned by);

    /** Moves the cursor down by given number of rows.
     */
    std::function<std::ostream &(std::ostream &)> down(unsigned by);

    /** Moves the cursor left by given number of characters.
     */
    std::function<std::ostream &(std::ostream &)> left(unsigned by);

    /** Moves the cursor right by given number of characters.
     */
    std::function<std::ostream &(std::ostream &)> right(unsigned by);

    /** Moves the cursor to the beginning of given number of lines above.
     */
    std::function<std::ostream &(std::ostream &)> nextLine(unsigned by);

    /** Moves the cursor to the beginning of given number of lines below.
     */
    std::function<std::ostream &(std::ostream &)> prevLine(unsigned by);

    /** Set cursor position to given X and Y coordinates.
     */
    std::function<std::ostream &(std::ostream &)> set(unsigned x, unsigned y);

    /** Sets the foreground color to given value.
     */
    std::function<std::ostream &(std::ostream &)> fg(color c);

    /** Sets the background color to given value.
     */
    std::function<std::ostream &(std::ostream &)> bg(color c);
}

/** Support for writing colors to output streams.
 */
::std::ostream & operator << (::std::ostream & s, ::color c);
