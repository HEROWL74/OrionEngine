// editor/PixWrapper.hpp
#pragma once
#ifdef _WIN32
#  include <Windows.h>
#  include <d3d12.h>
#  ifndef USE_PIX
#    define USE_PIX
#  endif
#  include <pix3.h>
#endif