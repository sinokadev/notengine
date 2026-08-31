.PHONY: docs format

docs:
	cd docs && doxygen -g Doxyfile

format:
	clang-format -i demo/*.cpp
	clang-format -i src/*.cpp
	clang-format -i include/knot/*.h
	clang-format -i include/knot/utility/*.h