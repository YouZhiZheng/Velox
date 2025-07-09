#include <iostream>

int main(int argc, char* argv[])
{
  std::cout << "- INFO - Hello Sym-CTS!" << std::endl;

  std::cout << "You passed " << argc << " arguments:" << std::endl;
  for (int i = 0; i < argc; ++i)
  {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::cout << "  argv[" << i << "] = " << argv[i] << std::endl;
  }

  return 0;
}