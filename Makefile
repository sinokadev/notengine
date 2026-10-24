.PHONY: docs format

docs:
	cd docs && doxygen -g Doxyfile

format:
	clang-format -i demo/*.cpp
	clang-format -i src/*.cpp
	clang-format -i include/knot/*.h
	clang-format -i include/knot/utility/*.h
	clang-format -i shaders/*

build:
	cmake --build --preset ninja-release-all

cloc:
	cloc --include-ext=cpp,h,frag,vert --force-lang="C++",frag --force-lang="C++",vert ./src ./include/knot ./include/knot/utility ./shaders