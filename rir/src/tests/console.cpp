#include <cassert>
#include "console.h"

namespace console {
	namespace {

		char const * CSI = "\033[";

		void colorCode(std::ostream & s, color c, unsigned val) {
			switch (c) {
			case color::black:
			case color::darkGray:
				val += 0;
				break;
			case color::red:
			case color::darkRed:
				val += 1;
				break;
			case color::green:
			case color::darkGreen:
				val += 2;
				break;
			case color::yellow:
			case color::darkYellow:
				val += 3;
				break;
			case color::blue:
			case color::darkBlue:
				val += 4;
				break;
			case color::magenta:
			case color::darkMagenta:
				val += 5;
				break;
			case color::cyan:
			case color::darkCyan:
				val += 6;
				break;
			case color::white:
			case color::gray:
				val += 7;
				break;
			default:
				assert(false && "unreachable");
			}
			switch (c) {
			case color::black:
			case color::darkRed:
			case color::darkGreen:
			case color::darkYellow:
			case color::darkBlue:
			case color::darkMagenta:
			case color::darkCyan:
			case color::gray:
				if (val >= 40) // background colors should not change foreground effect
					s << CSI << val << "m";
				else
					s << CSI << "0;" << val << "m";
				break;
			default:
				assert((val < 40) && "Intense colors can only be used for foreground");
				s << CSI << "1;" << val << "m";
			}
		}
	}

	/** Resets the terminal. */
	std::ostream & reset(std::ostream & s) {
		s << CSI << "0m";
		return s;
	}

	/** Erase display */
	std::ostream & erase(std::ostream & s) {
		s << CSI << "2J";
		return s;
	}

	/** Shows the cursor */
	std::ostream & show(std::ostream & s) {
		s << CSI << "?25h";
		return s;
	}

	/** Hides the cursor. */
	std::ostream & hide(std::ostream & s) {
		s << CSI << "?25l";
		return s;
	}

	/** Moves the cursor up by given number of rows. */
	std::function<std::ostream &(std::ostream &)> up(unsigned by) {
		return [by](std::ostream & s) -> std::ostream & {
			s << CSI << by << "A";
			return s;
		};
	}

	/** Moves the cursor down by given number of rows. */
	std::function<std::ostream &(std::ostream &)> down(unsigned by) {
		return [by](std::ostream & s) -> std::ostream & {
			s << CSI << by << "B";
			return s;
		};
	}

	/** Moves the cursor left by given number of characters. */
	std::function<std::ostream &(std::ostream &)> left(unsigned by) {
		return [by](std::ostream & s) -> std::ostream & {
			s << CSI << by << "D";
			return s;
		};
	}

	/** Moves the cursor right by given number of characters. */
	std::function<std::ostream &(std::ostream &)> right(unsigned by) {
		return [by](std::ostream & s) -> std::ostream & {
			s << CSI << by << "C";
			return s;
		};

	}

	/** Moves the cursor to the beginning of given number of lines above. */
	std::function<std::ostream &(std::ostream &)> nextLine(unsigned by) {
		return [by](std::ostream & s) -> std::ostream & {
			s << CSI << by << "E";
			return s;
		};
	}

	/** Moves the cursor to the beginning of given number of lines below. */
	std::function<std::ostream &(std::ostream &)> prevLine(unsigned by) {
		return [by](std::ostream & s) -> std::ostream & {
			s << CSI << by << "F";
			return s;
		};
	}

	/** Set cursor position to given X and Y coordinates */
	std::function<std::ostream &(std::ostream &)> set(unsigned x, unsigned y) {
		return [x, y](std::ostream & s) -> std::ostream & {
			s << CSI << x << ";" << y << "H";
			return s;
		};
	}

	/** Sets the foreground color to given value. */
	std::function<std::ostream &(std::ostream &)> fg(color c) {
		return [c](std::ostream & s) -> std::ostream & {
			colorCode(s, c, 30);
			return s;
		};
	}

	/** Sets the background color to given value. */
	std::function<std::ostream &(std::ostream &)> bg(color c) {
		return [c](std::ostream & s) -> std::ostream & {
			colorCode(s, c, 40);
			return s;
		};
	}

}


::std::ostream & operator << (::std::ostream & s, ::color c) {
	console::colorCode(s, c, 30);
	return s;
}