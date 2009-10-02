CONFIG += qt console
debug {
  CONFIG += debug
}
SOURCES += main.cpp

HEADERS += pstdint.h Assert.h Config.h Real.h MathTools.h RefCounted.h CopyOnWrite.h
HEADERS += Color.h

HEADERS += Image.h
SOURCES += Image.cpp

HEADERS += Profiler.h
SOURCES += Profiler.cpp

HEADERS += ThreadingQt.h Parallel.h
SOURCES += Parallel.cpp

HEADERS += GaussianPyramid.h
SOURCES += GaussianPyramid.cpp