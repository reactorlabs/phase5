#include <cstdlib>
#include <vector>
#include <iostream>

#include "tests.h"
#include "console.h"

namespace {
	typedef std::vector<Test *> Tests;

	Tests & getTests() {
		static Tests tests;
		return tests;
	}

}

std::ostream & operator << (std::ostream & stream, TestError const & e) {
	stream << e.file_ << "(" << e.line_ << "):" << std::endl;
	stream << "    " << e.what();
	return stream;
}

int Test::RunAll() {
	bool reportErrors = true;
	bool reportSuccess = true;
	unsigned successful = 0;
	std::cout << "I RUN TESTZ NOW, OK?" << std::endl;
	for (Test * t : getTests()) {
		try {
			if (reportSuccess)
				std::cout << t->suite() << "." << t->name() << "..." << std::flush;
			t->execute();
			if (reportSuccess)
				std::cout << color::green << " :)" << console::reset << std::endl;
			++successful;
		} catch (TestError const & e) {
			if (!reportSuccess)
				std::cout << t->suite() << "." << t->name() << "...";
			std::cout << color::red << " FAILZ" << console::reset << std::endl;
			if (reportErrors)
				std::cout << "    " << e << std::endl;
		} catch (std::exception const & e) {
			if (!reportSuccess)
				std::cout << t->suite() << "." << t->name() << "...";
			std::cout << color::red << " FAILZ" << console::reset << std::endl;
			if (reportErrors)
				std::cout << "    " << e.what() << std::endl;
		} catch (...) {
			if (! reportSuccess)
				std::cout << t->suite() << "." << t->name() << "...";
			std::cout << color::red << " FAILZ" << console::reset << std::endl;
			if (reportErrors)
				std::cout << "    Undefined error" << std::endl;
		}
	}

	size_t total = getTests().size();

	std::cout << std::endl;

	std::cout << "Total tests:    " << total << std::endl;
	std::cout << "Passed:         " << successful << std::endl;
	std::cout << "Failed:         " << (total - successful) << std::endl;
	if (successful != total) {
		std::cout << color::red << "SRSLY?!" << console::reset << std::endl;
		return EXIT_FAILURE;
	} else {
		std::cout << color::green << "AWSHUM!" << console::reset << std::endl;
		return EXIT_SUCCESS;
	}
}


Test * Test::RegisterTestClass(Test * test) {
	getTests().push_back(test);
	return test;
}

