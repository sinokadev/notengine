.PHONY: docs format build cloc

docs:
	cd docs && doxygen Doxyfile

format:
	find demo src include/knot shaders -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.frag" -o -name "*.vert" \) -exec clang-format -i {} +

build:
	cmake --build --preset ninja-release-all

cloc:
	cloc --include-ext=cpp,h,frag,vert --force-lang="C++",frag --force-lang="C++",vert ./src ./include/knot ./include/knot/utility ./shaders